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
use nalgebra::{DVector, DMatrix};

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

const LOG_OFFSET: f32 = 1.;

struct FramedSignalProcessor {
    // Circular buffer using double-write strategy to ensure contiguous readout
    // (size = FRAME_SIZE * 2)
    buffer: Vec<i16>,
    write_pointer: usize,
    hop_counter: i32,
}

impl FramedSignalProcessor {
    pub fn new() -> Self {
        Self {
            buffer: vec![0_i16; FRAME_SIZE * 2],
            write_pointer: 0,
            hop_counter: (HOP_SIZE as i32) - ((FRAME_SIZE / 2) as i32),
        }
    }

    pub fn process(
        &mut self,
        samples: &[i16] // this can be any length
    ) -> Vec<DVector<i16>> { // each DVector has size FRAME_SIZE
        let mut result = Vec::<DVector<i16>>::new();
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
                result.push(DVector::from_column_slice(&self.buffer[self.write_pointer..self.write_pointer + FRAME_SIZE]));
            }
        }
        result
    }
}

struct ShortTimeFourierTransformProcessor {
    window: DVector<f32>, // size = FRAME_SIZE
    fft: Arc<dyn Fft<f32>>,
}

fn hann(n: usize, m: usize) -> f32 {
    0.5 - 0.5 * (std::f32::consts::TAU * n as f32 / (m as f32 - 1.)).cos()
}

impl ShortTimeFourierTransformProcessor {
    pub fn new() -> Self {
        // Generate a hann window that also normalizes i16 audio data to the range -1 to 1

        let window = DVector::from_fn(FRAME_SIZE, |i, _| hann(i, FRAME_SIZE) * (1_f32 / (i16::MAX as f32)));

        // Plan the FFT
        let mut planner = FftPlanner::<f32>::new();
        let fft = planner.plan_fft_forward(FRAME_SIZE);

        Self {
            window,
            fft,
        }
    }

    pub fn process(
        &mut self,
        frame: &DVector<i16> // size = FRAME_SIZE
    ) -> DVector<f32> { // size = SPECTROGRAM_SIZE
        let mut buffer = vec![Complex {re: 0_f32, im: 0_f32}; FRAME_SIZE];
        for i in 0..FRAME_SIZE {
            buffer[i].re = (frame[i] as f32) * self.window[i];
        }
        self.fft.process(&mut buffer);

        // Slight deviation from madmom: the ShortTimeFourierTransformProcessor
        // returns complex values in madmom. Here, it returns a spectrogram (FFT magnitude)

        // Convert FFT to spectrogram by taking magnitude of each element
        DVector::from_fn(SPECTROGRAM_SIZE, |i, _| buffer[i].abs())
    }
}

fn triangle_filter(
    start: i32, center: i32, stop: i32
) -> DVector<f32> { // size = SPECTROGRAM_SIZE
    assert!(start < center);
    assert!(center < stop);
    assert!(stop <= SPECTROGRAM_SIZE as i32);

    let mut result = DVector::<f32>::zeros(SPECTROGRAM_SIZE);
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

    result
}

// MIDI note to frequency in Hz
fn note2freq(note: i32) -> f32 {
    2_f32.powf((note as f32 - 69.) / 12.) * 440.
}

// Returns the frequency corresponding to each entry in the spectrogram
fn spectrogram_frequencies() -> DVector<f32> { // size = SPECTROGRAM_SIZE
    DVector::from_fn(SPECTROGRAM_SIZE, |i, _| i as f32 * SAMPLE_RATE as f32 / (SPECTROGRAM_SIZE * 2) as f32)
}

// Returns the index of the closest entry in the spectrogram to the given frequency in Hz
fn freq2bin(
    spectrogram_frequencies: &DVector<f32>, // size = SPECTROGRAM_SIZE
    freq: f32
) -> usize {
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
pub fn gen_filterbank() -> DMatrix<f32> { // rows = N_FILTERS, cols = SPECTROGRAM_SIZE
    let freqs = spectrogram_frequencies();

    let mut filterbank = DMatrix::<f32>::zeros(N_FILTERS, SPECTROGRAM_SIZE);

    // Generate a set of triangle filters
    let mut filter_index = 0_usize;
    let mut previous_center = -1_i32;
    for note in (FILTER_MIN_NOTE + 1)..=(FILTER_MAX_NOTE - 1) {
        let center = freq2bin(&freqs, note2freq(note)) as i32;
        // Skip duplicate filters
        if center == previous_center {
            continue;
        }
        // Expand filter to include at least one spectrogram entry
        let mut start = freq2bin(&freqs, note2freq(note - 1)) as i32;
        let mut stop = freq2bin(&freqs, note2freq(note + 1)) as i32;
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
    filterbank: DMatrix<f32> // rows = N_FILTERS, cols = SPECTROGRAM_SIZE
}

impl FilteredSpectrogramProcessor {
    pub fn new() -> Self {
        Self {
            filterbank: gen_filterbank(),
        }
    }

    pub fn process(
        &mut self,
        spectrogram: &DVector<f32>, // size = SPECTROGRAM_SIZE
    ) -> DVector<f32> { // size = N_FILTERS
        let filter_output = &self.filterbank * spectrogram;

        // Slight deviation from madmom: the output of the FilteredSpectrogramProcessor
        // is sent into a LogarithmicSpectrogramProcessor.
        // Instead, we just take the log here and skip defining a LogarithmicSpectrogramProcessor.
        // (It is strange they call it a *spectrogram* processor,
        // as the output of this step is not really a spectrogram)
        filter_output.map(|x| (x + LOG_OFFSET).log10())
    }
}

struct SpectrogramDifferenceProcessor {
    prev: Option<DVector<f32>>, // size = N_FILTERS
}

impl SpectrogramDifferenceProcessor {
    pub fn new() -> Self {
        Self {
            prev: None,
        }
    }

    pub fn reset(&mut self) {
        self.prev = None;
    }

    pub fn process(
        &mut self,
        filtered_data: &DVector<f32> // size = N_FILTERS
    ) -> DVector<f32> { // size = N_FILTERS * 2
        let prev = match &self.prev {
            None => filtered_data,
            Some(prev) => prev,
        };

        let diff = (filtered_data - prev).map(|x| 0_f32.max(x));

        self.prev = Some(filtered_data.clone());

        let mut result = DVector::<f32>::zeros(N_FILTERS * 2);
        result.rows_range_mut(0..N_FILTERS).copy_from_slice(filtered_data.as_slice());
        result.rows_range_mut(N_FILTERS..N_FILTERS * 2).copy_from_slice(diff.as_slice());
        result
    }
}

fn sigmoid(x: f32) -> f32 {
    0.5_f32 * (1_f32 + (0.5_f32 * x).tanh())
}

fn tanh(x: f32) -> f32 {
    x.tanh()
}

struct FeedForwardLayer {
    weights: DMatrix<f32>, // OUTPUT_SIZE rows x INPUT_SIZE cols
    bias: DVector<f32>, // OUTPUT_SIZE
}

impl FeedForwardLayer {
    pub fn new(weights: DMatrix<f32>, bias: DVector<f32>) -> Self {
        Self {
            weights,
            bias,
        }
    }

    pub fn process(
        &self,
        data: &DVector<f32> // size INPUT_SIZE
    ) -> DVector<f32> { // size OUTPUT_SIZE
        (&self.weights * data + &self.bias).map(sigmoid)
    }
}

struct Gate {
    weights: DMatrix<f32>, // OUTPUT_SIZE rows x INPUT_SIZE cols
    bias: DVector<f32>, // OUTPUT_SIZE
    recurrent_weights: DMatrix<f32>, // OUTPUT_SIZE rows x OUTPUT_SIZE cols
    peephole_weights: DVector<f32>, // OUTPUT_SIZE
}

impl Gate {
    pub fn new(
        weights: DMatrix<f32>,
        bias: DVector<f32>,
        recurrent_weights: DMatrix<f32>,
        peephole_weights: DVector<f32>,
    ) -> Self {
        Self {
            weights,
            bias,
            recurrent_weights,
            peephole_weights,
        }
    }

    pub fn process(
        &self,
        data: &DVector<f32>, // size INPUT_SIZE
        prev: &DVector<f32>, // size OUTPUT_SIZE
        state: &DVector<f32>, // size OUTPUT_SIZE
    ) -> DVector<f32> { // size OUTPUT_SIZE
        (
            &self.weights * data
            + state.component_mul(&self.peephole_weights)
            + &self.recurrent_weights * prev
            + &self.bias
        ).map(sigmoid)
    }
}

struct Cell {
    weights: DMatrix<f32>, // OUTPUT_SIZE rows x INPUT_SIZE cols
    bias: DVector<f32>, // OUTPUT_SIZE
    recurrent_weights: DMatrix<f32>, // OUTPUT_SIZE rows x OUTPUT_SIZE cols
}

impl Cell {
    pub fn new(
        weights: DMatrix<f32>,
        bias: DVector<f32>,
        recurrent_weights: DMatrix<f32>,
    ) -> Self {
        Self {
            weights,
            bias,
            recurrent_weights,
        }
    }
    pub fn process(
        &self,
        data: &DVector<f32>, // size INPUT_SIZE
        prev: &DVector<f32>, // size OUTPUT_SIZE
    ) -> DVector<f32> { // size OUTPUT_SIZE
        (
            &self.weights * data
            + &self.recurrent_weights * prev
            + &self.bias
        ).map(tanh)
    }
}

struct LSTMLayer {
    // Note: for an LSTM layer, OUTPUT_SIZE == HIDDEN_SIZE == CELL_STATE_SIZE

    // prev stores the hidden state output of the LSTMLayer from t = n - 1
    prev: DVector<f32>, // OUTPUT_SIZE
    // state stores the cell state of the LSTMLayer from t = n - 1
    state: DVector<f32>, // OUTPUT_SIZE

    // LSTM machinery
    input_gate: Gate,
    forget_gate: Gate,
    cell: Cell,
    output_gate: Gate,
}

impl LSTMLayer {
    pub fn new(
        input_gate: Gate,
        forget_gate: Gate,
        cell: Cell,
        output_gate: Gate,
    ) -> Self {
        let output_size = input_gate.weights.nrows();
        Self {
            prev: DVector::zeros(output_size),
            state: DVector::zeros(output_size),
            input_gate,
            forget_gate,
            cell,
            output_gate,
        }
    }

    pub fn reset(&mut self) {
        self.prev.fill(0.);
        self.state.fill(0.);
    }

    pub fn process(
        &mut self,
        data: &DVector<f32> // size INPUT_SIZE
    ) -> DVector<f32> { // size OUTPUT_SIZE
        // Input gate: operate on current data, previous output, and state
        let ig = self.input_gate.process(data, &self.prev, &self.state);
        // Forget gate: operate on current data, previous output and state
        let fg = self.forget_gate.process(data, &self.prev, &self.state);
        // Cell: operate on current data and previous output
        let cell = self.cell.process(data, &self.prev);
        // Internal state: weight the cell with the input gate
        // and add the previous state weighted by the forget gate
        self.state.copy_from(&(cell.component_mul(&ig) + self.state.component_mul(&fg)));
        // Output gate: operate on current data, previous output and current state
        let og = self.output_gate.process(data, &self.prev, &self.state);
        // Output: apply activation function to state and weight by output gate
        let out = self.state.map(tanh).component_mul(&og);
        self.prev.copy_from(&out);
        out
    }
}

struct NeuralNetwork {
}

impl NeuralNetwork {
    pub fn new() -> Self {
        Self {}
    }
}

// Put everything together
struct BeatTracker {
    framed_processor: FramedSignalProcessor,
    stft_processor: ShortTimeFourierTransformProcessor,
    filter_processor: FilteredSpectrogramProcessor,
    difference_processor: SpectrogramDifferenceProcessor,
}

impl BeatTracker {
    pub fn new() -> Self {
        Self {
            framed_processor: FramedSignalProcessor::new(),
            stft_processor: ShortTimeFourierTransformProcessor::new(),
            filter_processor: FilteredSpectrogramProcessor::new(),
            difference_processor: SpectrogramDifferenceProcessor::new(),
        }
    }

    pub fn process(
        &mut self,
        samples: &[i16]
    ) -> Vec<DVector<f32>> { // each DVector size = N_FILTERS * 2
        println!("Processing {:?} samples", samples.len());
        let frames = self.framed_processor.process(samples);
        println!("Yielded {:?} frames", frames.len());
        frames.iter().map(|frame| {
            let spectrogram = self.stft_processor.process(frame);
            let filtered = self.filter_processor.process(&spectrogram);
            let diff = self.difference_processor.process(&filtered);
            // TODO: NN
            diff
        }).collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use nalgebra::{dvector, dmatrix};

    #[test]
    fn test_framed_signal_processor() {
        let audio_signal: Vec<i16> = (0_i16..2048).collect();
        let mut framed_signal_processor = FramedSignalProcessor::new();
        let frames = framed_signal_processor.process(&audio_signal[0..512]);
        assert_eq!(frames.len(), 0); // No frames should be returned yet
        let frames = framed_signal_processor.process(&audio_signal[512..1024]);
        assert_eq!(frames.len(), 1);
        assert_eq!(frames[0][0], 0);
        assert_eq!(frames[0][1023], 0);
        assert_eq!(frames[0][1024], 0);
        assert_eq!(frames[0][1025], 1);
        assert_eq!(frames[0][2047], 1023);
        let frames = framed_signal_processor.process(&audio_signal[1024..2048]);
        assert_eq!(frames.len(), 2);
        assert_eq!(frames[0][584], 1);
        assert_eq!(frames[0][1024], 441);
        assert_eq!(frames[0][2047], 1464);
        assert_eq!(frames[1][143], 1);
        assert_eq!(frames[1][1024], 882);
        assert_eq!(frames[1][2047], 1905);
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
        let audio_frame = DVector::from_fn(2048, |i, _| i as i16);
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
        assert_eq!(freq2bin(&freqs, 0.), 0);
        assert_eq!(freq2bin(&freqs, 24000.), 1023);
        assert_eq!(freq2bin(&freqs, 440.), 20);
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

    #[test]
    fn test_spectrogram_difference_processor() {
        let mut data = DVector::<f32>::zeros(N_FILTERS);
        let mut proc = SpectrogramDifferenceProcessor::new();

        data[0] = 1.;
        let r1 = proc.process(&data);
        data[0] = 2.;
        let r2 = proc.process(&data);
        data[0] = 1.;
        let r3 = proc.process(&data);

        // First half matches input
        assert_eq!(r1[0], 1.);
        assert_eq!(r2[0], 2.);
        assert_eq!(r3[0], 1.);

        // Second half is clamped differences
        assert_eq!(r1[N_FILTERS], 0.);
        assert_eq!(r2[N_FILTERS], 1.);
        assert_eq!(r3[N_FILTERS], 0.);
    }

    #[test]
    fn test_sigmoid() {
        assert_eq!(sigmoid(0.), 0.5);
        assert_eq!(sigmoid(2.), 0.8807971);
        assert_eq!(sigmoid(-4.), 0.017986208);
    }

    #[test]
    fn test_feed_forward_layer() {
        let weights = dmatrix!(1_f32, 1., 1.; 0., 1., 0.);
        let bias = dvector!(0_f32, 5.);
        let layer = FeedForwardLayer::new(weights, bias);
        let out = layer.process(&dvector!(0.5_f32, 0.6, 0.7));

        assert_eq!(out, dvector!(sigmoid(1.8), sigmoid(5.6)));
    }

    #[test]
    fn test_lstm_layer() {
        // Input gate
        let weights = dmatrix!(2_f32, 3., -1.; 0., 2., 0.);
        let bias = dvector!(1_f32, 3.);
        let recurrent_weights = dmatrix!(0_f32, -1.; 1., 0.);
        let peephole_weights = dvector!(2_f32, 3.);
        let ig = Gate::new(weights, bias, recurrent_weights, peephole_weights);

        // Forget gate;
        let weights = dmatrix!(-1_f32, 1., 0.; -1., 0., 2.);
        let bias = dvector!(-2_f32, -1.);
        let recurrent_weights = dmatrix!(2_f32, 0.; 0., 2.);
        let peephole_weights = dvector!(3_f32, -1.);
        let fg = Gate::new(weights, bias, recurrent_weights, peephole_weights);

        // Cell;
        let weights = dmatrix!(1_f32, -1., 3.; 2., -3., -2.);
        let bias = dvector!(1_f32, 2.);
        let recurrent_weights = dmatrix!(2_f32, 0.; 0., 2.);
        let c = Cell::new(weights, bias, recurrent_weights);

        // Output gate;
        let weights = dmatrix!(1_f32, -1., 2.; 1., -3., 4.);
        let bias = dvector!(1_f32, -2.);
        let recurrent_weights = dmatrix!(2_f32, -2.; 3., -1.);
        let peephole_weights = dvector!(1_f32, 3.);
        let og = Gate::new(weights, bias, recurrent_weights, peephole_weights);

        // LSTMLayer
        let mut l = LSTMLayer::new(ig, fg, c, og);

        let input1 = dvector!(2_f32, 3., 4.);
        let input2 = dvector!(6_f32, 7., 6.);
        let input3 = dvector!(4_f32, 3., 2.);

        let output1 = l.process(&input1);
        let output2 = l.process(&input2);
        let output3 = l.process(&input3);

        assert_eq!(output1, dvector!(0.7614811, -0.74785006));
        assert_eq!(output2, dvector!(0.9619418 , -0.94699216));
        assert_eq!(output3, dvector!(0.99459594, -0.4957872));
    }

    #[ignore]
    #[test]
    fn test_music() {
        use std::fs::File;
        use std::path::Path;

        // Read music from audio file
        let mut inp_file = File::open(Path::new("src/lib/test/frontier.wav")).unwrap();
        let (header, data) = wav::read(&mut inp_file).unwrap();
        assert_eq!(header.audio_format, wav::WAV_FORMAT_PCM);
        assert_eq!(header.channel_count, 1);
        assert_eq!(header.sampling_rate, 44100);
        assert_eq!(header.bits_per_sample, 16);
        let data = data.try_into_sixteen().unwrap();

        println!("WAV file has {:?} samples", data.len());

        // Instantiate a BeatTracker
        let mut bt = BeatTracker::new();
        let result = bt.process(&data);
        println!("{:?}", result[128]);
        panic!();
    }
}
