use crate::graphics::{Fbo, RenderChain};
use crate::video_node::{VideoNode, VideoNodeId, VideoNodeKind, VideoNodeKindMut};

use std::rc::Rc;
use web_sys::HtmlVideoElement;

/// TODO: Add support for HtmlImageElement
pub struct MediaNode {
    id: VideoNodeId,
    video: HtmlVideoElement,
}

impl MediaNode {
    pub fn new(video: HtmlVideoElement) -> MediaNode {
        MediaNode {
            id: VideoNodeId::new(),
            video,
        }
    }
}

impl VideoNode for MediaNode {
    fn id(&self) -> VideoNodeId {
        self.id
    }

    fn name(&self) -> &str {
        "Media"
    }

    fn n_buffers(&self) -> usize {
        1
    }

    fn render<'a>(
        &'a self,
        _chain: &'a RenderChain,
        input_fbos: &[Option<Rc<Fbo>>],
        buffer_fbos: &mut [Rc<Fbo>],
    ) -> Option<Rc<Fbo>> {
        assert!(input_fbos.len() == self.n_inputs());
        assert!(buffer_fbos.len() == self.n_buffers());

        if self.video.ready_state() == 4 {
            buffer_fbos[0]
                .image_video_element(&self.video)
                .ok()
                .map(|_| Rc::clone(&buffer_fbos[0]))
        } else {
            None
        }
    }

    fn downcast(&self) -> Option<VideoNodeKind> {
        Some(VideoNodeKind::Media(self))
    }
    fn downcast_mut(&mut self) -> Option<VideoNodeKindMut> {
        Some(VideoNodeKindMut::Media(self))
    }
}
