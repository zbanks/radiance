use radiance::{BeatTracker, FPS};
use std::fs::File;
use std::path::Path;

#[allow(clippy::print_literal)]
fn main() {
    // Usage: ./beat_tracking_test <filename.wav>
    let mut args = std::env::args();
    args.next().unwrap();
    let filename = args.next().expect("expect .wav file");

    // Read music from audio file
    let mut inp_file = File::open(Path::new(&filename)).unwrap();
    let (header, data) = wav::read(&mut inp_file).unwrap();
    assert_eq!(header.audio_format, wav::WAV_FORMAT_PCM);
    assert_eq!(header.channel_count, 1);
    assert_eq!(header.sampling_rate, 44100);
    assert_eq!(header.bits_per_sample, 16);
    let data = data.try_into_sixteen().unwrap();

    // Instantiate a BeatTracker
    let mut bt = BeatTracker::new();
    let beats = bt.process(&data);

    // Print results as a TSV
    println!("{}\t{}\t{}", "t", "beat", "activation");
    for (i, &(_, activation, beat)) in beats.iter().enumerate() {
        println!("{}\t{}\t{}", (i as f32) / FPS, beat, activation);
    }
}
