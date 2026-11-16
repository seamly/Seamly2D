// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief The ZIP archive of one export session, and its check.

use std::fs::File;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

use zip::write::SimpleFileOptions;

/// @brief Write `files` into a new ZIP at `archive`, each under its file name.
///
/// The archive must not exist: an export session never replaces a file.
/// The files stay where they are.
/// @return The entry names, in order.
pub fn write_archive(archive: &Path, files: &[PathBuf]) -> Result<Vec<String>, String> {
    let file = File::options()
        .write(true)
        .create_new(true)
        .open(archive)
        .map_err(|error| format!("{} cannot be created: {error}", archive.display()))?;
    let mut writer = zip::ZipWriter::new(file);
    let options = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);
    let mut names = Vec::new();
    for path in files {
        let name = path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
        let bytes = std::fs::read(path).map_err(|error| format!("{} cannot be read: {error}", path.display()))?;
        writer.start_file(name.as_str(), options).map_err(|error| error.to_string())?;
        writer.write_all(&bytes).map_err(|error| error.to_string())?;
        names.push(name);
    } // for each file
    writer.finish().map_err(|error| error.to_string())?;
    Ok(names)
} // fn write_archive

/// @brief Read the archive back: every file must be an entry with the same bytes.
pub fn check_archive(archive: &Path, files: &[PathBuf]) -> Result<(), String> {
    let file = File::open(archive).map_err(|error| format!("{} cannot be read: {error}", archive.display()))?;
    let mut reader = zip::ZipArchive::new(file).map_err(|error| format!("{} is not a ZIP: {error}", archive.display()))?;
    if reader.len() != files.len() {
        return Err(format!("the archive has {} entries; expected {}", reader.len(), files.len()));
    } // if entry count differs
    for path in files {
        let name = path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
        let mut entry = reader.by_name(&name).map_err(|_| format!("the archive has no entry {name}"))?;
        let mut stored = Vec::new();
        entry.read_to_end(&mut stored).map_err(|error| error.to_string())?;
        let original = std::fs::read(path).map_err(|error| error.to_string())?;
        if stored != original {
            return Err(format!("the archive entry {name} differs from the file"));
        } // if bytes differ
    } // for each file
    Ok(())
} // fn check_archive

#[cfg(test)]
mod tests {
    use super::*;

    // The archive holds every file byte for byte; a second archive at the same path is refused.
    #[test]
    fn archive_round_trip() {
        let folder = tempfile::tempdir().unwrap();
        let first = folder.path().join("shirt_Fabric_2611151830.svg");
        let second = folder.path().join("shirt_Lining_2611151830.svg");
        std::fs::write(&first, "<svg/>").unwrap();
        std::fs::write(&second, "<svg><g/></svg>").unwrap();
        let archive = folder.path().join("shirt_2611151830.zip");
        let files = vec![first.clone(), second.clone()];
        assert_eq!(write_archive(&archive, &files).unwrap(), vec![
            "shirt_Fabric_2611151830.svg".to_string(),
            "shirt_Lining_2611151830.svg".to_string()
        ]);
        assert_eq!(check_archive(&archive, &files), Ok(()));
        assert!(first.exists() && second.exists());
        assert!(write_archive(&archive, &files).is_err());
        assert!(check_archive(&archive, &files[..1]).is_err());
    } // archive_round_trip
} // mod tests
