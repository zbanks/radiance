extern crate nalgebra as na;

use na::{Vector2, Matrix4};
use radiance::ArcTextureViewSampler;
use std::sync::Arc;
use std::collections::{HashMap, HashSet};
use std::hash::Hash;

struct InstanceDescriptor {
    texture: ArcTextureViewSampler,
    pos_min: Vector2<f32>,
    pos_max: Vector2<f32>,
}

struct InstanceResources {
}

pub struct VideoNodePreviewRenderer<ID: Clone + Eq + Hash> {
    device: Arc<wgpu::Device>,
    queue: Arc<wgpu::Queue>,
    view: Matrix4<f32>,
    instance_list: Vec<(ID, InstanceDescriptor)>,
    instance_cache: HashMap<ID, InstanceResources>,
}

impl<ID: Clone + Eq + Hash> VideoNodePreviewRenderer<ID> {
    pub fn new(device: Arc<wgpu::Device>, queue: Arc<wgpu::Queue>) -> Self {
        Self {
            device,
            queue,
            view: Matrix4::identity(),
            instance_cache: Default::default(),
            instance_list: Default::default(),
        }
    }

    pub fn set_view(&mut self, view: &Matrix4<f32>) {
        self.view = *view;
    }

    pub fn push_instance(&mut self, id: &ID, texture: &ArcTextureViewSampler, pos_min: &Vector2<f32>, pos_max: &Vector2<f32>) {
        self.instance_list.push((
            id.clone(),
            InstanceDescriptor {
                texture: texture.clone(),
                pos_min: pos_min.clone(),
                pos_max: pos_max.clone(),
            },
        ));
    }

    pub fn paint(&mut self, encoder: &mut wgpu::CommandEncoder) {
        // Paint all of the pushed instances
        let mut visited = HashSet::<ID>::new();
        for (id, instance) in self.instance_list.iter() {
            visited.insert(id.clone());
            // TODO create cache entries if necessary
            // TODO paint
        }

        // Delete any cache entries that are not present in the instance list
        self.instance_cache.retain(|id, _| visited.contains(id));

        // Clear the list
        self.instance_list.clear();
    }
}
