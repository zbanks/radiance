use anyhow::*;
use std::fs::{read_to_string, write};
use std::path::PathBuf;

struct ShaderData {
    src: String,
    src_path: PathBuf,
    spv_path: PathBuf,
    kind: shaderc::ShaderKind,
}

impl ShaderData {
    pub fn load(src_path: &PathBuf, kind: shaderc::ShaderKind) -> Result<Self> {
        let src = read_to_string(src_path.clone())?;
        let spv_filename = src_path.with_extension("spv");
        let spv_filename = spv_filename.file_name().unwrap();

        //cargo-generated dir for build script to output to
        let mut spv_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());
        spv_path.push(spv_filename);

        Ok(Self {
            src,
            src_path: src_path.clone(),
            spv_path,
            kind,
        })
    }
}

fn main() -> Result<()> {
    // Collect shaders from /src
    let shader_paths = [
        (PathBuf::from("src/effect_vertex.glsl"), shaderc::ShaderKind::Vertex),
    ];

    // This could be parallelized
    let shaders = shader_paths
        .iter()
        .map(|(path, kind)| ShaderData::load(path, *kind))
        .collect::<Vec<Result<_>>>()
        .into_iter()
        .collect::<Result<Vec<_>>>()?;

    let mut compiler = shaderc::Compiler::new().context("Unable to create shader compiler")?;

    // This can't be parallelized. The [shaderc::Compiler] is not
    // thread safe. Also, it creates a lot of resources. You could
    // spawn multiple processes to handle this, but it would probably
    // be better just to only compile shaders that have been changed
    // recently.
    for shader in shaders {
        // This tells cargo to rerun this script if something in /src/ changes.
        println!("cargo:rerun-if-changed={}", shader.src_path.as_os_str().to_str().unwrap());
        
        let compiled = compiler.compile_into_spirv(
            &shader.src,
            shader.kind,
            &shader.src_path.to_str().unwrap(),
            "main",
            None,
        )?;
        write(shader.spv_path, compiled.as_binary_u8())?;
    }

    Ok(())
}
