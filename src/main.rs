use libradiance::Context;
use futures::executor::block_on;

fn main() {
    let adapter = block_on(wgpu::Instance::new(wgpu::BackendBit::PRIMARY).request_adapter(
        &wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::Default,
            compatible_surface: None,
        }
    )).unwrap();
    let (device, queue) = block_on(adapter.request_device(&Default::default(), None)).unwrap();

    let ctx = Context::new(&device, &queue);
    println!("Hello, world!");
}
