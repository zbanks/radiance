use crate::err::Result;

use wasm_bindgen::closure::Closure;
use wasm_bindgen::{JsCast, JsValue};
use web_sys::{AudioContext, AnalyserNode};

pub struct Audio {
    context: AudioContext,
    analyser: AnalyserNode,
    source_promise: Option<Box<Closure<dyn FnMut(JsValue)>>>,
}

#[derive(Debug)]
pub struct AudioAnalysis {
    pub lows: f32,
    pub mids: f32,
    pub highs: f32,
    pub envelope: f32,
    pub spectrum: [f32; 32],
}

impl Audio {
    pub fn new() -> Result<Audio> {
        let context = AudioContext::new()?;
        let analyser = context.create_analyser()?;
        analyser.set_fft_size(2048);
        analyser.set_smoothing_time_constant(0.85);
        analyser.set_min_decibels(-90.);
        analyser.set_max_decibels(10.);

        let context_clone = context.clone();
        let analyser_clone = analyser.clone();
        let source_promise = Some(Box::new(Closure::once(move |result: JsValue| {
            let media: web_sys::MediaStream = result.dyn_into().unwrap();
            let source = context_clone.create_media_stream_source(&media).unwrap();
            source.connect_with_audio_node(&analyser_clone).unwrap();
        })));

        let audio = Audio {
            context,
            analyser,
            source_promise,
        };

        let mut constraints = web_sys::MediaStreamConstraints::new();
        constraints.audio(&JsValue::TRUE).video(&JsValue::FALSE);
        web_sys::window()
            .unwrap()
            .navigator()
            .media_devices()
            .unwrap()
            .get_user_media_with_constraints(&constraints)
            .unwrap()
            .then(&*audio.source_promise.as_ref().unwrap());

        Ok(audio)
    }

    pub fn analyze(&self) -> AudioAnalysis {
        let len = self.analyser.frequency_bin_count();
        let mut data: Vec<f32> = vec![0.; len as usize];
        self.analyser.get_float_frequency_data(data.as_mut_slice());

        // XXX This is just a proof of concept
        let map = |x| f32::min(f32::max((x + 140.) / 160., 0.), 1.);
        AudioAnalysis {
            lows: map(data[10]),
            mids: map(data[40]),
            highs: map(data[100]),
            envelope: map(data[200]),
            spectrum: [0.0; 32],
        }
    }
}

impl Default for AudioAnalysis {
    fn default() -> AudioAnalysis {
        AudioAnalysis {
            lows: 0.0,
            mids: 0.0,
            highs: 0.0,
            envelope: 0.0,
            spectrum: [0.0; 32],
        }
    }
}
