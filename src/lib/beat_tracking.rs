// Code in this file is based on and ported from the madmom project:
// @inproceedings{madmom,
//    Title = {{madmom: a new Python Audio and Music Signal Processing Library}},
//    Author = {B{\"o}ck, Sebastian and Korzeniowski, Filip and Schl{\"u}ter, Jan and Krebs, Florian and Widmer, Gerhard},
//    Booktitle = {Proceedings of the 24th ACM International Conference on
//    Multimedia},
//    Month = {10},
//    Year = {2016},
//    Pages = {1174--1178},
//    Address = {Amsterdam, The Netherlands},
//    Doi = {10.1145/2964284.2973795}
// }
//
// Copyright (c) 2022 Eric Van Albert
// Copyright (c) 2012-2014 Department of Computational Perception,
// Johannes Kepler University, Linz, Austria and Austrian Research Institute for
// Artificial Intelligence (OFAI), Vienna, Austria.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use std::sync::Arc;
use rustfft::{FftPlanner, num_complex::{Complex, ComplexFloat}, Fft};
use nalgebra::{SVector, SMatrix, Vector};

const SAMPLE_RATE: usize = 44100;
// Hop size of 441 with sample rate of 44100 Hz gives an output frame rate of 100 Hz
const HOP_SIZE: usize = 441;
const FRAME_SIZE: usize = 2048;
const SPECTROGRAM_SIZE: usize = FRAME_SIZE / 2;

// MIDI notes corresponding to the low and high filters
// Span a range from 30 Hz to 17000 Hz
const FILTER_MIN_NOTE: i32 = 23; // 30.87 Hz
const FILTER_MAX_NOTE: i32 = 132; // 16744 Hz
// Filtering parameters:
const N_FILTERS: usize = 81;

struct FramedSignalProcessor {
    buffer: [i16; FRAME_SIZE * 2], // Circular buffer using double-write strategy to ensure contiguous readout
    write_pointer: usize,
    hop_counter: i32,
}

impl FramedSignalProcessor {
    pub fn new() -> Self {
        Self {
            buffer: [0_i16; FRAME_SIZE * 2],
            write_pointer: 0,
            hop_counter: (HOP_SIZE as i32) - (FRAME_SIZE as i32),
        }
    }

    pub fn process(&mut self, samples: &[i16]) -> Vec<SVector<i16, FRAME_SIZE>> {
        let mut result = Vec::<SVector<i16, FRAME_SIZE>>::new();
        for sample in samples {
            self.buffer[self.write_pointer] = *sample;
            self.buffer[self.write_pointer + FRAME_SIZE] = *sample;
            self.write_pointer += 1;
            assert!(self.write_pointer <= FRAME_SIZE);
            if self.write_pointer == FRAME_SIZE {
                self.write_pointer = 0;
            }
            self.hop_counter += 1;
            assert!(self.hop_counter <= HOP_SIZE as i32);
            if self.hop_counter == HOP_SIZE as i32 {
                self.hop_counter = 0;
                let mut chunk = [0_i16; FRAME_SIZE];
                chunk.copy_from_slice(&self.buffer[self.write_pointer..self.write_pointer + FRAME_SIZE]);
                result.push(SVector::from(chunk));
            }
        }
        result
    }
}

struct ShortTimeFourierTransformProcessor {
    window: SVector<f32, FRAME_SIZE>,
    fft: Arc<dyn Fft<f32>>,
}

fn hann(n: usize, m: usize) -> f32 {
    0.5 - 0.5 * (std::f32::consts::TAU * n as f32 / (m as f32 - 1.)).cos()
}

impl ShortTimeFourierTransformProcessor {
    pub fn new() -> Self {
        // Generate a hann window that also normalizes i16 audio data to the range -1 to 1
        let mut window = SVector::from([0_f32; FRAME_SIZE]);
        for i in 0..FRAME_SIZE {
            window[i] = hann(i, FRAME_SIZE) * (1_f32 / (i16::MAX as f32));
        }

        // Plan the FFT
        let mut planner = FftPlanner::<f32>::new();
        let fft = planner.plan_fft_forward(FRAME_SIZE);

        Self {
            window,
            fft,
        }
    }

    pub fn process(&mut self, frame: &SVector<i16, FRAME_SIZE>) -> SVector<f32, SPECTROGRAM_SIZE> {
        let mut buffer = [Complex {re: 0_f32, im: 0_f32}; FRAME_SIZE];
        for i in 0..FRAME_SIZE {
            buffer[i].re = (frame[i] as f32) * self.window[i];
        }
        self.fft.process(&mut buffer);

        // Convert FFT to spectrogram by taking magnitude of each element
        let mut result = [0_f32; SPECTROGRAM_SIZE];
        for i in 0..SPECTROGRAM_SIZE {
            result[i] = buffer[i].abs();
        }
        SVector::from(result)
    }
}

fn triangle_filter(start: i32, center: i32, stop: i32) -> SVector<f32, SPECTROGRAM_SIZE> {
    assert!(start < center);
    assert!(center < stop);
    assert!(stop <= SPECTROGRAM_SIZE as i32);

    let mut result = [0_f32; SPECTROGRAM_SIZE];
    let mut sum = 0_f32;
    for i in start + 1..=center {
        let x = (i as f32 - start as f32) / (center as f32 - start as f32);
        if i >= 0 && i < SPECTROGRAM_SIZE as i32 {
            result[i as usize] = x;
        }
        sum += x;
    }
    for i in center + 1..stop {
        let x = (i as f32 - stop as f32) / (center as f32 - stop as f32);
        if i >= 0 && i < SPECTROGRAM_SIZE as i32 {
            result[i as usize] = x;
        }
        sum += x;
    }

    // Normalize
    for i in start + 1..stop {
        if i >= 0 && i < SPECTROGRAM_SIZE as i32 {
            result[i as usize] /= sum;
        }
    }

    SVector::from(result)
}

// MIDI note to frequency in Hz
fn note2freq(note: i32) -> f32 {
    2_f32.powf((note as f32 - 69.) / 12.) * 440.
}

// Returns the frequency corresponding to each entry in the spectrogram
fn spectrogram_frequencies() -> SVector<f32, SPECTROGRAM_SIZE> {
    let mut result = [0_f32; SPECTROGRAM_SIZE];
    for i in 0..SPECTROGRAM_SIZE {
        result[i] = i as f32 * SAMPLE_RATE as f32 / (SPECTROGRAM_SIZE * 2) as f32
    }
    SVector::from(result)
}

// Returns the index of the closest entry in the spectrogram to the given frequency in Hz
fn freq2bin(spectrogram_frequencies: SVector<f32, SPECTROGRAM_SIZE>, freq: f32) -> usize {
    let mut index = SPECTROGRAM_SIZE - 1;
    for i in 1..SPECTROGRAM_SIZE {
        if freq < spectrogram_frequencies[i] {
            let left = spectrogram_frequencies[i - 1];
            let right = spectrogram_frequencies[i];
            index = if (freq - left).abs() < (freq - right).abs() {
                i - 1
            } else {
                i
            };
            break;
        }
    }
    index
}

// Returns a filter bank according to the given constants
pub fn gen_filterbank() -> SMatrix<f32, N_FILTERS, SPECTROGRAM_SIZE> {
    let freqs = spectrogram_frequencies();

    let filterbank = [[0_f32; N_FILTERS]; SPECTROGRAM_SIZE];
    let mut filterbank: SMatrix<f32, N_FILTERS, SPECTROGRAM_SIZE> = SMatrix::from(filterbank);

    // Generate a set of triangle filters
    let mut filter_index = 0_usize;
    let mut previous_center = -1_i32;
    for note in (FILTER_MIN_NOTE + 1)..=(FILTER_MAX_NOTE - 1) {
        let center = freq2bin(freqs, note2freq(note)) as i32;
        // Skip duplicate filters
        if center == previous_center {
            continue;
        }
        // Expand filter to include at least one spectrogram entry
        let mut start = freq2bin(freqs, note2freq(note - 1)) as i32;
        let mut stop = freq2bin(freqs, note2freq(note + 1)) as i32;
        if stop - start < 2 {
            start = center - 1;
            stop = center + 1;
        }
        filterbank.set_row(filter_index, &triangle_filter(start, center, stop).transpose());
        filter_index += 1;
        previous_center = center;
    }

    // Check that N_FILTERS constant was set appropriately
    assert_eq!(filter_index, N_FILTERS);

    filterbank
}


struct FilteredSpectrogramProcessor {
    filterbank: SMatrix<f32, N_FILTERS, SPECTROGRAM_SIZE>
}

impl FilteredSpectrogramProcessor {
    pub fn new() -> Self {
        Self {
            filterbank: gen_filterbank(),
        }
    }

    pub fn process(&mut self, spectrogram: &SVector<f32, SPECTROGRAM_SIZE>) -> SVector<f32, N_FILTERS> {
        self.filterbank * spectrogram
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_framed_signal_processor() {
        let audio_signal: Vec<i16> = (0_i16..3072).collect();
        let mut framed_signal_processor = FramedSignalProcessor::new();
        let frames = framed_signal_processor.process(&audio_signal[0..1024]);
        assert_eq!(frames.len(), 0); // No frames should be returned yet
        let frames = framed_signal_processor.process(&audio_signal[1024..2048]);
        assert_eq!(frames.len(), 1);
        assert_eq!(frames[0][0], 0);
        assert_eq!(frames[0][1], 1);
        assert_eq!(frames[0][2047], 2047);
        let frames = framed_signal_processor.process(&audio_signal[2048..3072]);
        assert_eq!(frames.len(), 2);
        assert_eq!(frames[0][0], 441);
        assert_eq!(frames[0][2047], 2488);
        assert_eq!(frames[1][0], 882);
        assert_eq!(frames[1][2047], 2929);
    }

    #[test]
    fn test_hann() {
        assert_eq!(hann(0, 7), 0.);
        assert_eq!(hann(1, 7), 0.25);
        assert_eq!(hann(2, 7), 0.75);
        assert_eq!(hann(3, 7), 1.);
        assert_eq!(hann(4, 7), 0.74999994);
        assert_eq!(hann(5, 7), 0.24999982);
        assert_eq!(hann(6, 7), 0.);
    }

    #[test]
    fn test_stft_processor() {
        let mut audio_frame = [0_i16; 2048];
        audio_frame.copy_from_slice(&(0_i16..2048).collect::<Vec<_>>()[..]);
        let audio_frame = SVector::from(audio_frame);
        let mut stft_processor = ShortTimeFourierTransformProcessor::new();
        let result = stft_processor.process(&audio_frame);

        assert_eq!(result[0], 31.96973);
        assert_eq!(result[1], 17.724249);
        assert_eq!(result[2], 1.7021317);
        assert_eq!(result[1023], 0.);
    }

    #[test]
    fn test_triangle_filter() {
        let filt = triangle_filter(5, 7, 15);
        assert_eq!(filt[5], 0.);
        assert_eq!(filt[6], 0.1);
        assert_eq!(filt[7], 0.2);
        assert_eq!(filt[8], 0.175);
        assert_eq!(filt[9], 0.15);
        assert_eq!(filt[10], 0.125);
        assert_eq!(filt[11], 0.1);
        assert_eq!(filt[12], 0.075);
        assert_eq!(filt[13], 0.05);
        assert_eq!(filt[14], 0.025);
        assert_eq!(filt[15], 0.);
    }

    #[test]
    fn test_note2freq() {
        assert_eq!(note2freq(69), 440.);
        assert_eq!(note2freq(57), 220.);
        assert_eq!(note2freq(60), 261.62555);
    }

    #[test]
    fn test_spectrogram_frequencies() {
        let freqs = spectrogram_frequencies();
        assert_eq!(freqs[0], 0.);
        assert_eq!(freqs[1], 21.533203);
        assert_eq!(freqs[1022], 22006.934);
        assert_eq!(freqs[1023], 22028.467);
    }

    #[test]
    fn test_freq2bin() {
        let freqs = spectrogram_frequencies();
        assert_eq!(freq2bin(freqs, 0.), 0);
        assert_eq!(freq2bin(freqs, 24000.), 1023);
        assert_eq!(freq2bin(freqs, 440.), 20);
    }

    #[test]
    fn test_gen_filterbank() {
        let filterbank = gen_filterbank();

        // Check triangle filter peaks
        assert_eq!(filterbank[(0,2)], 1.);
        assert_eq!(filterbank[(1,3)], 1.);
        assert_eq!(filterbank[(2,4)], 1.);
        assert_eq!(filterbank[(78,654)], 0.02631579);
        assert_eq!(filterbank[(79,693)], 0.025);
        assert_eq!(filterbank[(80,734)], 0.023529412);
    }
}
