use crate::beat_tracking::{BeatTracker, SAMPLE_RATE};
use std::thread;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::Duration;
use cpal;
use cpal::traits::DeviceTrait;

/// A Mir (Music information retrieval) object
/// handles listening to the music
/// and generates a global timebase from the beats,
/// along with other relevant real-time inputs based on the music
/// such as the highs, mids, lows, and overall audio level.
pub struct Mir {
    thread: thread::JoinHandle<()>,
    shared: Arc<MirShared>,
}

struct MirShared {
    should_stop: AtomicBool,
}

impl Mir {
    fn run(s: Arc<MirShared>) {
        // Make a new beat tracker
        let mut bt = BeatTracker::new();

        // Set up system audio
        use cpal::traits::HostTrait;
        let host = cpal::default_host();
        let device = host.default_output_device().expect("no output device available");

        const MIN_USEFUL_BUFFER_SIZE: cpal::FrameCount = 256; // Lower actually would be useful, but CPAL lies about the min size, so this ought to be safe
        const SAMPLE_RATE_CPAL: cpal::SampleRate = cpal::SampleRate(SAMPLE_RATE as u32);
        let config_range = device.supported_input_configs()
            .expect("error while querying configs")
            .filter(|config| 
                (config.sample_format() == cpal::SampleFormat::I16
                || config.sample_format() == cpal::SampleFormat::U16)
                && SAMPLE_RATE_CPAL >= config.min_sample_rate()
                && SAMPLE_RATE_CPAL <= config.max_sample_rate()
                && match *config.buffer_size() {
                    cpal::SupportedBufferSize::Range {max, ..} => MIN_USEFUL_BUFFER_SIZE <= max,
                    cpal::SupportedBufferSize::Unknown => true,
                } && (config.channels() == 1
                || config.channels() == 2
                )
            ).min_by_key(|config| match *config.buffer_size() {
                cpal::SupportedBufferSize::Range {min, ..} => MIN_USEFUL_BUFFER_SIZE.max(min),
                cpal::SupportedBufferSize::Unknown => 8192, // Large but not unreasonable
            })
            .expect("no supported config")
            .with_sample_rate(SAMPLE_RATE_CPAL);

        println!("found supported config {:?}", config_range);
        let mut config = config_range.config();

        if let cpal::SupportedBufferSize::Range {min, ..} = *config_range.buffer_size() {
            config.buffer_size = cpal::BufferSize::Fixed(MIN_USEFUL_BUFFER_SIZE.max(min));
        }

        println!("choosing audio config {:?}", config);

        let mut process_audio_i16_mono = move |data: &[i16]| {
            let (spectrogram, activation, beat) = match bt.process(data).pop() {
                Some(result) => result,
                None => {return;},
            };
            if beat {
                println!("Beat");
            }
        };

        let process_error = move |err| {
            println!("Audio error, {:?}", err)
        };

        let stream = match (config_range.sample_format(), config.channels) {
                (cpal::SampleFormat::I16, 1) => device.build_input_stream(
                    &config,
                    move |data: &[i16], _: &cpal::InputCallbackInfo| {
                        process_audio_i16_mono(data);
                    },
                    process_error,
                ),
                (cpal::SampleFormat::I16, 2) => device.build_input_stream(
                    &config,
                    move |data: &[i16], _: &cpal::InputCallbackInfo| {
                        let data: Vec<i16> = data.chunks(2).map(|pair| ((pair[0] as i32 + pair[1] as i32) / 2) as i16).collect();
                        process_audio_i16_mono(&data);
                    },
                    process_error,
                ),
                (cpal::SampleFormat::U16, 1) => device.build_input_stream(
                    &config,
                    move |data: &[u16], _: &cpal::InputCallbackInfo| {
                        let data: Vec<i16> = data.iter().map(|&x| ((x as i32) - 32768) as i16).collect();
                        process_audio_i16_mono(&data);
                    },
                    process_error,
                ),
                (cpal::SampleFormat::U16, 2) => device.build_input_stream(
                    &config,
                    move |data: &[u16], _: &cpal::InputCallbackInfo| {
                        let data: Vec<i16> = data.chunks(2).map(|pair| ((pair[0] as i32 + pair[1] as i32) / 2 - 32768) as i16).collect();
                        process_audio_i16_mono(&data);
                    },
                    process_error,
                ),
                _ => panic!("unexpected sample format or channel count")
        }.expect("failed to open input stream");

        while !s.should_stop.load(Ordering::Relaxed) {
            //println!("Thread tick");
            thread::sleep(Duration::from_millis(100));
        }
        println!("MIR thread exiting")
    }

    pub fn stop(self) {
        self.shared.should_stop.store(true, Ordering::Relaxed);
        self.thread.join().expect("Mir thread panicked");
    }

    pub fn new() -> Self {
        let shared = Arc::new(MirShared {
            should_stop: AtomicBool::new(false),
        });
        let shared_clone = shared.clone();
        let thread = thread::spawn(move || {
            Self::run(shared_clone);
        });
        Mir {
            thread,
            shared,
        }
    }
}
