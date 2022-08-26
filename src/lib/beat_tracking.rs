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
}
