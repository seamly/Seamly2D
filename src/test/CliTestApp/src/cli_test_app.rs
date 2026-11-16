// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief Interactive tester of the Seamly2D command-line layout export.
//!
//! Screens: 1 Inputs → 2 Layout (multisize patterns only) → 3 Export format → Results.
//! The choices come from `seamly2d --layout-info`, so this tool repeats no layout rule.
//! `--batch` runs the same steps without a window, for scripted checks.

mod app_view;
mod cli_command;
mod export_run;
mod layout_report;
mod output_check;
mod output_naming;
mod preferences_layout_folder;
mod zip_archive;

use std::path::PathBuf;

use cli_command::{DxfVariant, ExportRequest};
use layout_report::query_report;

/// @brief Seamly2D next to this program, in the build tree, or on PATH.
pub fn default_seamly2d() -> PathBuf {
    let name = if cfg!(windows) { "seamly2d.exe" } else { "seamly2d" };
    if let Some(path) = std::env::var_os("SEAMLY2D_EXE").map(PathBuf::from).filter(|path| path.is_file()) {
        return path;
    } // if set in the environment
    let beside = std::env::current_exe().ok().and_then(|exe| exe.parent().map(|folder| folder.join(name)));
    if let Some(path) = beside.filter(|path| path.is_file()) {
        return path;
    } // if beside this program
    // Build tree: <repo>/src/test/CliTestApp/target/<profile>/ → <repo>/src/app/seamly2d/bin/.
    let in_tree = std::env::current_exe().ok().and_then(|exe| {
        exe.ancestors().find_map(|folder| {
            let candidate = folder.join("src").join("app").join("seamly2d").join("bin").join(name);
            candidate.is_file().then_some(candidate)
        })
    }); // in_tree
    in_tree.unwrap_or_else(|| PathBuf::from(name))
} // fn default_seamly2d

/// @brief Value after `--name` in the argument list.
pub fn option_value(arguments: &[String], name: &str) -> Option<String> {
    arguments.iter().position(|argument| argument == name).and_then(|index| arguments.get(index + 1)).cloned()
} // fn option_value

/// @brief `--batch`: query, choose as the GUI would, run every material, archive, and print the results.
/// @return Process exit code: 0 when every material and the archive passed, 1 on a failed run, 2 on bad input.
fn run_batch(arguments: &[String]) -> i32 {
    let Some(pattern) = option_value(arguments, "--pattern").map(PathBuf::from) else {
        eprintln!("--batch needs --pattern <file.sm2d>");
        return 2;
    }; // pattern
    let seamly2d = option_value(arguments, "--seamly2d").map(PathBuf::from).unwrap_or_else(default_seamly2d);
    let measurements = option_value(arguments, "--mfile").map(PathBuf::from);
    let settings = option_value(arguments, "--settings").map(PathBuf::from);
    let output_folder = option_value(arguments, "--output-dir")
        .map(PathBuf::from)
        .unwrap_or_else(preferences_layout_folder::layout_folder);
    let basename = option_value(arguments, "--basename").unwrap_or_else(|| {
        pattern.file_stem().map(|stem| stem.to_string_lossy().to_string()).unwrap_or_else(|| "layout".to_string())
    }); // basename

    let report = match query_report(&seamly2d, &pattern, measurements.as_deref(), settings.as_deref()) {
        Ok(report) => report,
        Err(error) => {
            eprintln!("{error}");
            return 1;
        } // Err
    }; // report
    let split = report.needs_split();
    let views = report.view_choices(split);
    let view = if report.multisize {
        let wanted = option_value(arguments, "--view");
        match wanted {
            Some(view) if views.contains(&view) => Some(view),
            Some(view) => {
                eprintln!("View '{view}' is not offered. Views: {}", views.join(", "));
                return 2;
            } // unknown view
            None => views.first().cloned(),
        } // match wanted view
    } else {
        None
    }; // view
    let formats = report.format_choices(view.as_deref().unwrap_or(""), split);
    let format = match option_value(arguments, "--format") {
        Some(format) if formats.contains(&format) => format,
        Some(format) => {
            let reason = report
                .unavailable_formats(view.as_deref().unwrap_or(""), split)
                .into_iter()
                .find(|item| item.name == format)
                .map_or("unknown format".to_string(), |item| item.reason);
            eprintln!("Format '{format}' is not offered: {reason}. Formats: {}", formats.join(", "));
            return 2;
        } // unavailable format
        None => match formats.first() {
            Some(format) => format.clone(),
            None => {
                eprintln!("The layout allows no export format.");
                return 2;
            } // no format
        }, // default format
    }; // format
    let dxf_variant = match option_value(arguments, "--dxf").as_deref() {
        Some("clo3d") => DxfVariant::Clo3d,
        Some("aama") => DxfVariant::AamaR11,
        _ => DxfVariant::AstmR13,
    }; // dxf_variant
    let export_options = (format == "dxf").then(|| app_view::write_dxf_options(dxf_variant)).transpose();
    let export_options = match export_options {
        Ok(options) => options,
        Err(error) => {
            eprintln!("{error}");
            return 1;
        } // Err
    }; // export_options
    if let Err(error) = std::fs::create_dir_all(&output_folder) {
        eprintln!("{} cannot be created: {error}", output_folder.display());
        return 2;
    } // if output folder cannot be made

    let request = ExportRequest {
        seamly2d,
        pattern,
        measurements,
        settings,
        view,
        split,
        format,
        svg_text_mode: option_value(arguments, "--svg-text-mode"),
        export_options,
        several_materials: report.materials.len() > 1,
    };
    let stamp = output_naming::session_stamp(&chrono::Local::now());
    println!("split pieces: {split}; materials: {}", report.materials.len());
    let result = export_run::run_session(&request, &report, &output_folder, &basename, &stamp);
    for material in &result.materials {
        println!("{}: {}", if material.succeeded() { "PASS" } else { "FAIL" }, material.run.command);
        match &material.check {
            Ok(bytes) => println!("      {} ({bytes} bytes) {}", material.run.output.display(), material.stdout.trim()),
            Err(problem) => println!("      {problem}\n      {}", material.stderr.trim()),
        } // match check
    } // for each material
    match &result.archive {
        Some(Ok(path)) => println!("ARCHIVE: {}", path.display()),
        Some(Err(error)) => println!("ARCHIVE FAILED: {error}"),
        None => println!("ARCHIVE: none (a material failed)"),
    } // match archive
    if result.succeeded() { 0 } else { 1 }
} // fn run_batch

/// @brief Close the console window that Windows made for this program alone.
///
/// The program is a console program, so `--batch` prints to the shell that started it.
/// A start from Explorer gets a console of its own; the window does not need it.
/// A console shared with a shell (more than one process) stays.
#[cfg(windows)]
fn release_own_console() {
    #[link(name = "kernel32")]
    extern "system" {
        fn GetConsoleProcessList(process_list: *mut u32, process_count: u32) -> u32;
        fn FreeConsole() -> i32;
    }
    let mut processes = [0u32; 4];
    // SAFETY: the buffer holds `processes.len()` entries, as the call is told.
    let count = unsafe { GetConsoleProcessList(processes.as_mut_ptr(), processes.len() as u32) };
    if count == 1 {
        // SAFETY: FreeConsole takes no arguments; failure leaves the console open, which is harmless.
        unsafe { FreeConsole() };
    } // if the console belongs to this program alone
} // fn release_own_console

#[cfg(not(windows))]
fn release_own_console() {}

fn main() {
    let arguments: Vec<String> = std::env::args().collect();
    if arguments.iter().any(|argument| argument == "--batch") {
        std::process::exit(run_batch(&arguments));
    } // if batch mode
    release_own_console();
    if let Err(error) = app_view::run(arguments) {
        eprintln!("{error}");
        std::process::exit(1);
    } // if the window failed
} // fn main
