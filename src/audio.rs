use crate::err::Result;

use log::*;
use wasm_bindgen::closure::Closure;
use wasm_bindgen::{JsCast, JsValue};
use web_sys::{AnalyserNode, AudioContext};

pub struct Audio {
    // Need to keep the `context` variable around to prevent it from being GC'd
    #[allow(dead_code)]
    context: AudioContext,
    analyser: AnalyserNode,
    #[allow(clippy::type_complexity)]
    source_promise: Option<Box<Closure<dyn FnMut(JsValue)>>>,
}

const SPECTRUM_BINS: usize = 32;
const LOW_CUTOFF: f32 = 400.0; // in Hz
const HIGH_CUTOFF: f32 = 4000.0; // in Hz

#[derive(Debug)]
pub struct AudioAnalysis {
    pub low: f32,
    pub mid: f32,
    pub high: f32,
    pub level: f32,
    pub spectrum: [f32; SPECTRUM_BINS],
}

impl Audio {
    pub fn new() -> Result<Audio> {
        let context = AudioContext::new()?;
        let analyser = context.create_analyser()?;
        analyser.set_fft_size(2048);
        analyser.set_smoothing_time_constant(0.30);
        analyser.set_min_decibels(-300.);
        analyser.set_max_decibels(300.);

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
        // The AudioContext may have been prevented from starting if it loaded too quickly
        // It must be resumed (or created) after a user gesture on the page. https://goo.gl/7K7WLu
        // If we're suspended, forcibly try to resume
        if self.context.state() == web_sys::AudioContextState::Suspended {
            // Even if resume succeeds, we won't yet have data to analyze on this iteration
            self.context.resume();
        }

        let mut analysis: AudioAnalysis = Default::default();
        if self.context.state() == web_sys::AudioContextState::Running {
            let sample_rate = self.context.sample_rate();
            let len = self.analyser.frequency_bin_count();
            let mut data: Vec<f32> = vec![0.; len as usize];
            self.analyser.get_float_frequency_data(data.as_mut_slice());

            // Convert linearly-spaced logarithmic magnitudes (`data`) into logarithmicly-spaced linear
            // magnitudes (`spectrum`)
            let spectrum_bin_factor: f32 = SPECTRUM_BINS as f32 / f32::ln(len as f32);
            let mut spectrum_bin_count: [usize; SPECTRUM_BINS] = [0; SPECTRUM_BINS];
            let mut spectrum_bin_frequencies: [f32; SPECTRUM_BINS] = [0.0; SPECTRUM_BINS];
            let mut spectrum: [f32; SPECTRUM_BINS] = [0.0; SPECTRUM_BINS];
            for (i, freq_bin) in spectrum_bin_frequencies.iter_mut().enumerate() {
                *freq_bin = (i as f32 + 0.5) * sample_rate / (SPECTRUM_BINS as f32 * 2.0);
            }
            for (i, log_v) in data.iter().enumerate() {
                let spectrum_bin: usize = (f32::ln(i as f32) * spectrum_bin_factor) as usize;
                spectrum_bin_count[spectrum_bin] += 1;

                const TEN: f32 = 10.;
                let v: f32 = TEN.powf(*log_v / 20.);
                spectrum[spectrum_bin] += v;
            }
            for (s, c) in spectrum.iter_mut().zip(spectrum_bin_count.iter()) {
                if *c > 1 {
                    *s /= *c as f32;
                }
            }

            // Take average magnitude over sections to compute lows/mids/highs
            let mut low_count = 0;
            let mut mid_count = 0;
            let mut high_count = 0;
            for (freq, v) in spectrum_bin_frequencies.iter().zip(spectrum.iter()) {
                if *freq < LOW_CUTOFF {
                    low_count += 1;
                    analysis.low += *v;
                } else if *freq > HIGH_CUTOFF {
                    high_count += 1;
                    analysis.high += *v;
                } else {
                    mid_count += 1;
                    analysis.mid += *v;
                }
            }
            analysis.low /= low_count as f32;
            analysis.mid /= mid_count as f32;
            analysis.high /= high_count as f32;
            analysis.level = analysis.low + analysis.mid + analysis.high;

            // Convert all linear magnitudes into log magnitudes, with some arbitrary scale
            // to clamp between [0.0, 1.0]
            // XXX this SCALE function is arbitrary
            const SCALE: fn(f32) -> f32 =
                |x: f32| f32::max(0.0, f32::min(1.0, f32::log10(x) * 0.15 + 0.8));
            analysis.level = SCALE(analysis.level);
            analysis.low = SCALE(analysis.low);
            analysis.mid = SCALE(analysis.mid);
            analysis.high = SCALE(analysis.high);
            analysis
                .spectrum
                .iter_mut()
                .zip(spectrum.iter())
                .for_each(|(s, x)| {
                    *s = SCALE(*x);
                });
        }
        analysis
    }
}

impl Default for AudioAnalysis {
    fn default() -> AudioAnalysis {
        AudioAnalysis {
            low: 0.0,
            mid: 0.0,
            high: 0.0,
            level: 0.0,
            spectrum: [0.0; SPECTRUM_BINS],
        }
    }
}
