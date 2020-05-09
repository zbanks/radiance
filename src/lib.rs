#![recursion_limit = "512"]

mod app;
mod audio;
mod err;
mod graphics;
mod library;
mod model;
mod resources;
mod utils;
mod video_node;

use wasm_bindgen::prelude::*;

// When the `wee_alloc` feature is enabled, use `wee_alloc` as the global
// allocator.
#[cfg(feature = "wee_alloc")]
#[global_allocator]
static ALLOC: wee_alloc::WeeAlloc = wee_alloc::WeeAlloc::INIT;

pub use app::Context;

#[wasm_bindgen(start)]
pub fn setup_hooks() -> Result<(), JsValue> {
    utils::set_panic_hook();
    web_logger::init();
    Ok(())
}
