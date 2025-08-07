use std::path::Path;

fn main() {
    let manifest = Path::new("screen-mask.exe.manifest")
        .canonicalize()
        .unwrap();
    println!("cargo:rustc-link-arg-bins=/MANIFEST:EMBED");
    println!(
        "cargo:rustc-link-arg-bins=/MANIFESTINPUT:{}",
        manifest.display()
    );
    println!("cargo:rerun-if-changed=screen-mask.exe.manifest");
    println!("cargo:rerun-if-changed=build.rs");
}
