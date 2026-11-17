// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief One export session: a Seamly2D run per material, the checks, and the archive.
//!
//! Workflow of `run_session`:
//!   1. `plan_session` names one output per material with one session stamp.
//!   2. Each material runs `seamly2d --layout-export ...` with its own argument list.
//!   3. Each run must exit 0 and leave a complete file of the format.
//!   4. Only when every material passed: write `<basename>_<stamp>.zip` and read it back.
//! Output files stay in the output folder; nothing is deleted.

use std::path::{Path, PathBuf};
use std::process::Command;

use crate::cli_command::{display_command, ExportRequest};
use crate::layout_report::LayoutReport;
use crate::output_check::check_export_file;
use crate::output_naming::{archive_path, material_output_path, unique_path};
use crate::zip_archive::{check_archive, write_archive};

/// @brief The command of one material, before it runs.
#[derive(Clone, Debug, PartialEq)]
pub struct PlannedRun {
    /// Material name from the report; empty when the handoff names none.
    pub material: String,
    pub output: PathBuf,
    pub arguments: Vec<String>,
    /// The command line to read and copy.
    pub command: String,
}

/// @brief Outcome of one material.
#[derive(Clone, Debug)]
pub struct MaterialResult {
    pub run: PlannedRun,
    /// Exit code; None when the process could not start or was stopped by a signal.
    pub exit_code: Option<i32>,
    pub stdout: String,
    pub stderr: String,
    /// File size, or the check failure.
    pub check: Result<u64, String>,
    /// Files the export wrote beside the output, such as a DXF-ASTM `.rul` grade rule table.
    pub companions: Vec<PathBuf>,
}

impl MaterialResult {
    /// @brief True when the run exited 0 and its file passed the check.
    pub fn succeeded(&self) -> bool {
        self.exit_code == Some(0) && self.check.is_ok()
    } // fn succeeded
}

/// @brief Outcome of a session.
#[derive(Clone, Debug)]
pub struct SessionResult {
    pub stamp: String,
    pub materials: Vec<MaterialResult>,
    /// None when a material failed (no archive is made); else the archive path or its error.
    pub archive: Option<Result<PathBuf, String>>,
}

impl SessionResult {
    /// @brief True when every material and the archive passed.
    pub fn succeeded(&self) -> bool {
        self.materials.iter().all(MaterialResult::succeeded) && matches!(self.archive, Some(Ok(_)))
    } // fn succeeded
}

/// @brief Name the output of each material and build its command.
pub fn plan_session(request: &ExportRequest, report: &LayoutReport, output_folder: &Path, basename: &str, stamp: &str)
    -> Vec<PlannedRun> {
    let mut taken: Vec<PathBuf> = Vec::new();
    report
        .materials
        .iter()
        .map(|material| {
            let path = material_output_path(output_folder, basename, &material.name, stamp, &request.format);
            let output = unique_path(&path, &taken);
            taken.push(output.clone());
            let arguments = request.arguments(&material.name, &output);
            PlannedRun {
                material: material.name.clone(),
                command: display_command(&request.seamly2d, &arguments),
                output,
                arguments,
            }
        })
        .collect()
} // fn plan_session

/// @brief Run one planned material export and check its file.
pub fn run_material(request: &ExportRequest, run: PlannedRun) -> MaterialResult {
    let started = Command::new(&request.seamly2d).args(&run.arguments).output();
    let (exit_code, stdout, stderr) = match started {
        Ok(output) => (
            output.status.code(),
            String::from_utf8_lossy(&output.stdout).to_string(),
            String::from_utf8_lossy(&output.stderr).to_string(),
        ),
        Err(error) => (None, String::new(), format!("{} could not be started: {error}", request.seamly2d.display())),
    }; // started
    let check = if exit_code == Some(0) {
        check_export_file(&run.output, &request.format)
    } else {
        Err(format!("exit code {}", exit_code.map_or("none".to_string(), |code| code.to_string())))
    }; // check
    let companions = ["rul"]
        .iter()
        .map(|extension| run.output.with_extension(extension))
        .filter(|path| path.exists())
        .collect();
    MaterialResult { run, exit_code, stdout, stderr, check, companions }
} // fn run_material

/// @brief Run every material, then archive the outputs when all passed.
pub fn run_session(request: &ExportRequest, report: &LayoutReport, output_folder: &Path, basename: &str, stamp: &str)
    -> SessionResult {
    let materials: Vec<MaterialResult> = plan_session(request, report, output_folder, basename, stamp)
        .into_iter()
        .map(|run| run_material(request, run))
        .collect();
    let archive = materials.iter().all(MaterialResult::succeeded).then(|| {
        let files: Vec<PathBuf> = materials
            .iter()
            .flat_map(|result| std::iter::once(result.run.output.clone()).chain(result.companions.iter().cloned()))
            .collect();
        let path = unique_path(&archive_path(output_folder, basename, stamp), &[]);
        write_archive(&path, &files).and_then(|_| check_archive(&path, &files)).map(|_| path)
    }); // archive
    SessionResult { stamp: stamp.to_string(), materials, archive }
} // fn run_session

#[cfg(test)]
mod tests {
    use super::*;

    // Two materials get two names with one stamp; each command names its material.
    #[test]
    fn plan_names_each_material() {
        let report = LayoutReport::parse(r#"{"labelText":"text","materials":[
            {"name":"Fabric","oversizedPieces":[],"whole":[{"formats":["svg"],"view":""}],"split":[]},
            {"name":"Lining","oversizedPieces":[],"whole":[{"formats":["svg"],"view":""}],"split":[]}],
            "multisize":false,"sizes":[],"svgTextModes":[],"tiledPaper":false}"#).unwrap();
        let request = ExportRequest {
            seamly2d: PathBuf::from("seamly2d"),
            pattern: PathBuf::from("p.sm2d"),
            format: "pdf-tiled".to_string(),
            several_materials: true,
            ..ExportRequest::default()
        };
        let folder = tempfile::tempdir().unwrap();
        let plan = plan_session(&request, &report, folder.path(), "coat", "2611151830");
        assert_eq!(plan.len(), 2);
        assert_eq!(plan[0].output, folder.path().join("coat_Fabric_2611151830.pdf"));
        assert_eq!(plan[1].output, folder.path().join("coat_Lining_2611151830.pdf"));
        assert!(plan[1].arguments.windows(2).any(|pair| pair == ["--layout-material", "Lining"]));
        assert!(plan[0].command.starts_with("seamly2d --layout-export"));
    } // plan_names_each_material

    // A program that cannot start fails the material, and no archive is made.
    #[test]
    fn failed_material_makes_no_archive() {
        let report = LayoutReport::parse(r#"{"labelText":"text","materials":[
            {"name":"","oversizedPieces":[],"whole":[{"formats":["svg"],"view":""}],"split":[]}],
            "multisize":false,"sizes":[],"svgTextModes":[],"tiledPaper":false}"#).unwrap();
        let request = ExportRequest {
            seamly2d: PathBuf::from("no_such_program_for_the_test"),
            pattern: PathBuf::from("p.sm2d"),
            format: "svg".to_string(),
            ..ExportRequest::default()
        };
        let folder = tempfile::tempdir().unwrap();
        let result = run_session(&request, &report, folder.path(), "shirt", "2611151830");
        assert!(!result.succeeded());
        assert!(result.archive.is_none());
        assert!(!folder.path().join("shirt_2611151830.zip").exists());
    } // failed_material_makes_no_archive
} // mod tests
