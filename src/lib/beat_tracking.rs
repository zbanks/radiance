use std::sync::Arc;
use rustfft::{FftPlanner, num_complex::Complex, Fft};

const HOP_SIZE: usize = 441;
const FRAME_SIZE: usize = 2048;

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

    pub fn process(&mut self, samples: &[i16]) -> Vec<[i16; FRAME_SIZE]> {
        let mut result = Vec::<[i16; FRAME_SIZE]>::new();
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
                result.push(chunk);
            }
        }
        result
    }
}

struct ShortTimeFourierTransformProcessor {
    window: [f32; FRAME_SIZE],
    fft: Arc<dyn Fft<f32>>,
}

fn hann(n: usize, m: usize) -> f32 {
    0.5 - 0.5 * (std::f32::consts::TAU * n as f32 / (m as f32 - 1.)).cos()
}

impl ShortTimeFourierTransformProcessor {
    pub fn new() -> Self {
        // Generate a hann window that also normalizes i16 audio data to the range -1 to 1
        let mut window = [0_f32; FRAME_SIZE];
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

    pub fn process(&mut self, frame: &[i16; FRAME_SIZE]) -> [Complex<f32>; FRAME_SIZE] {
        let mut buffer = [Complex {re: 0_f32, im: 0_f32}; FRAME_SIZE];
        for i in 0..FRAME_SIZE {
            buffer[i].re = (frame[i] as f32) * self.window[i];
        }
        self.fft.process(&mut buffer);
        buffer
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
        let mut stft_processor = ShortTimeFourierTransformProcessor::new();
        let result = stft_processor.process(&audio_frame);

        assert_eq!(result[0], Complex { re: 31.96973, im: 0.0 });
        assert_eq!(result[1], Complex { re: -16.008266, im: 7.608182 });
        assert_eq!(result[2], Complex { re: 0.01043538, im: -1.7020998 });
        assert_eq!(result[1023], Complex { re: 0., im: 0. });
    }
}
