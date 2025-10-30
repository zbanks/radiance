use std::{fs, path::Path};

// Include the generated code from build.rs
include!(concat!(env!("OUT_DIR"), "/embedded_library.rs"));

/// Extracts embedded files into the resource directory.
/// Only writes files that don't already exist.
pub fn load_default_library(resource_dir: &Path) {
    let library_dir = resource_dir.join("library");
    if !library_dir.exists() {
        fs::create_dir(&library_dir).unwrap();
    }
    for (filename, content) in EMBEDDED_LIBRARY.iter() {
        let file_path = library_dir.join(filename);

        // Skip if file already exists
        if file_path.exists() {
            continue;
        }

        // Write the file
        match fs::write(&file_path, content) {
            Ok(()) => println!("Copied built-in effect: {}", filename),
            Err(e) => println!("Could not write {}: {}", file_path.display(), e),
        }
    }
}
