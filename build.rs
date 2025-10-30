use std::env;
use std::fs;
use std::io::Write;
use std::path;
use std::path::Path;

fn main() {
    check_builtin_shaders();
    //check_library_shaders();
    embed_default_library();
}

fn check_builtin_shaders() {
    // Builtin shader files to validate
    let shader_files = vec![
        "src/lib/image_shader.wgsl",
        "src/bin/winit_output/output.wgsl",
        "src/bin/winit_output/projection_map.wgsl",
        "src/bin/ui/spectrum_widget.wgsl",
        "src/bin/ui/beat_widget.wgsl",
        "src/bin/ui/video_node_tile.wgsl",
        "src/bin/ui/waveform_widget.wgsl",
    ];

    let mut shader_sources = vec![];
    for shader_path in shader_files {
        println!("cargo:rerun-if-changed={}", shader_path);
        shader_sources.push((
            shader_path,
            fs::read_to_string(Path::new(shader_path)).unwrap(),
        ));
    }

    println!("cargo:rerun-if-changed=src/lib/effect_header.wgsl");
    println!("cargo:rerun-if-changed=src/lib/effect_footer.wgsl");
    let effect_header = fs::read_to_string(Path::new("src/lib/effect_header.wgsl")).unwrap();
    let effect_footer = fs::read_to_string(Path::new("src/lib/effect_footer.wgsl")).unwrap();
    let effect_noop = "fn main(uv: vec2<f32>) -> vec4<f32> { return vec4<f32>(0., 0., 0., 0.); }";

    shader_sources.push((
        "src/lib/effect_header+footer.wgsl",
        format!("{}\n{}\n{}\n", effect_header, effect_noop, effect_footer),
    ));

    let mut had_errors = false;

    for (label, shader_source) in shader_sources {
        // Parse and validate the WGSL shader using naga
        match naga::front::wgsl::parse_str(&shader_source) {
            Ok(module) => {
                // Validate the module
                let mut validator = naga::valid::Validator::new(
                    naga::valid::ValidationFlags::all(),
                    naga::valid::Capabilities::all(),
                );

                match validator.validate(&module) {
                    Ok(_) => {}
                    Err(e) => {
                        eprintln!("{}", e.emit_to_string_with_path(&shader_source, label));
                        had_errors = true;
                    }
                }
            }
            Err(e) => {
                eprintln!("{}", e.emit_to_string_with_path(&shader_source, label));
                had_errors = true;
            }
        }
    }
    if had_errors {
        panic!("Shader validation failed!");
    }
}

/*
fn check_library_shaders() {
    let mut had_errors = false;

    // Validate all .wgsl files in library/ directory
    let library_dir = Path::new("library");
    println!("cargo:rerun-if-changed=library/");

    if library_dir.exists() && library_dir.is_dir() {
        let library_shaders = fs::read_dir(library_dir)
            .expect("Failed to read library directory")
            .filter_map(|entry| {
                let entry = entry.ok()?;
                let path = entry.path();
                if path.extension()?.to_str()? == "wgsl" {
                    Some(path)
                } else {
                    None
                }
            });

        let header_source = match fs::read_to_string("src/lib/effect_header.wgsl") {
            Ok(source) => Some(source),
            Err(e) => {
                eprintln!("Failed to read shader file {}: {}", shader_path, e);
                had_errors = true;
                None
            }
        };

        let footer_source = match fs::read_to_string("src/lib/effect_footer.wgsl") {
            Ok(source) => Some(source),
            Err(e) => {
                eprintln!("Failed to read shader file {}: {}", shader_path, e);
                had_errors = true;
                None
            }
        };

        if let (Some(header_source), Some(footer_source)) = (header_source, footer_source) {
            for shader_path in library_shaders {
                println!("cargo:rerun-if-changed={}", shader_path.display());

                let shader_source = match fs::read_to_string(&shader_path) {
                    Ok(source) => source,
                    Err(e) => {
                        eprintln!(
                            "Failed to read shader file {}: {}",
                            shader_path.display(),
                            e
                        );
                        had_errors = true;
                        continue;
                    }
                };

                // Parse and validate the WGSL shader using naga
                match naga::front::wgsl::parse_str(&format!(
                    "{}\n{}\n{}\n",
                    header_source, shader_source, footer_source
                )) {
                    Ok(module) => {
                        // Validate the module
                        let mut validator = naga::valid::Validator::new(
                            naga::valid::ValidationFlags::all(),
                            naga::valid::Capabilities::all(),
                        );

                        match validator.validate(&module) {
                            Ok(_) => {
                                println!(
                                    "cargo:warning=✓ Validated shader: {}",
                                    shader_path.display()
                                );
                            }
                            Err(e) => {
                                eprintln!("Validation error in {}: {}", shader_path.display(), e);
                                had_errors = true;
                            }
                        }
                    }
                    Err(e) => {
                        eprintln!("Parse error in {}: {}", shader_path.display(), e);
                        had_errors = true;
                    }
                }
            }
        }
    } else {
        println!("cargo:warning=Library directory 'library/' not found, skipping library shaders");
    }

    if had_errors {
        panic!("Shader validation failed!");
    }
}
*/

fn embed_default_library() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("embedded_library.rs");
    let mut f = fs::File::create(&dest_path).unwrap();

    // Read all files from library/ directory
    let library_dir = Path::new("library");

    if !library_dir.exists() {
        panic!("Could not find library/ directory");
    }

    // Generate the code
    writeln!(f, "pub const EMBEDDED_LIBRARY: &[(&str, &[u8])] = &[").unwrap();

    for entry in fs::read_dir(library_dir).unwrap().flatten() {
        if !entry.file_type().unwrap().is_file() {
            continue;
        }
        let filename = entry.file_name().display().to_string();
        let abs_path = path::absolute(entry.path())
            .unwrap()
            .to_string_lossy()
            .to_string();

        writeln!(
            f,
            "    (\"{}\", include_bytes!(\"{}\")),",
            filename.replace('\\', "/"),    // Normalize path separators
            abs_path.replace('\\', "\\\\")  // Escape backslashes for Windows
        )
        .unwrap();

        // Tell Cargo to rerun build.rs if this file changes
        println!("cargo:rerun-if-changed={}", abs_path);
    }

    writeln!(f, "];").unwrap();

    // Tell Cargo to rerun if library directory structure changes
    println!("cargo:rerun-if-changed=library");
}
