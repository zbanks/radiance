import bisect
import subprocess
import sys
from dataclasses import dataclass, field, replace
from pathlib import Path
from tempfile import NamedTemporaryFile

import numpy as np
import numpy.typing as npt
import rubberband
import soundfile

PROFILE = "release"
BEAT_TEST_BIN = Path(__file__).parent / "../../target" / PROFILE / "beat_tracking_test"

if not BEAT_TEST_BIN.is_file():
    print(
        f"{BEAT_TEST_BIN} does not exist, please run `cargo build --profile {PROFILE}`"
    )
    sys.exit(1)


@dataclass
class BeatSample:
    time: float
    is_beat: bool
    activation: float

    @classmethod
    def from_line(cls, line: str) -> "BeatSample":
        time, is_beat, activation = line.strip().split("\t")
        return cls(
            time=float(time),
            is_beat=is_beat == "true",
            activation=float(activation),
        )


def get_beats(wav_path: Path) -> list[BeatSample]:
    if not wav_path.is_file():
        raise ValueError(f"input file {wav_path} does not exist")
    result = subprocess.run([BEAT_TEST_BIN, wav_path], check=True, capture_output=True)
    lines = result.stdout.decode("utf-8").split("\n")
    assert lines[0] == "t\tbeat\tactivation"
    output = []
    for line in lines[1:]:
        if not line:
            continue
        output.append(BeatSample.from_line(line))
    output = sorted(output, key=lambda x: x.time)
    return output


def read_expected_beats(wav_path: Path) -> list[float]:
    beats_path = wav_path.with_suffix(".beats")
    if not beats_path.is_file():
        raise ValueError(
            "beats file {beats_path} for input file {wav_path} does not exist"
        )
    output = []
    for line in beats_path.read_text().split("\n"):
        line = line.strip()
        if not line:
            continue
        output.append(float(line))
    output = sorted(output)
    return output


@dataclass
class BeatComparison:
    expected: list[float] = field(repr=False)
    measured: list[BeatSample] = field(repr=False)

    exact_match: bool = field()
    error_sq: float = field()
    aligned_fraction: float = field()

    @classmethod
    def compare(
        cls,
        expected: list[float],
        measured: list[BeatSample],
        sample_rate: float = 0.01,
    ) -> "BeatComparison":
        measured_beat_times = [b.time for b in measured if b.is_beat]

        # This measurement is asymmetric, crude, and slow to compute,
        # but it's good enough for some rough tests/comparisons
        error_sq = 0.0
        exact_count = 0
        for exp in expected:
            left = bisect.bisect_left(measured_beat_times, exp)
            right = bisect.bisect_right(measured_beat_times, exp)
            error = min(
                abs(exp - m) for m in measured_beat_times[max(0, left - 1) : right + 1]
            )
            # Allow rounding from unaligned sample rates
            error = max(0, error - sample_rate / 2)
            if error == 0:
                exact_count += 1
            error_sq += error**2

        exact_match = (exact_count == len(expected)) and len(expected) == len(
            measured_beat_times
        )

        return cls(
            expected=expected,
            measured=measured,
            exact_match=exact_match,
            error_sq=error_sq / len(expected),
            aligned_fraction=exact_count / len(expected),
        )


@dataclass
class Wav:
    name: str = field()
    sample_rate: int = field()
    data: npt.NDArray[np.int16] = field(repr=False)
    expected_beats: list[float] = field(repr=False)

    def __post_init__(self) -> None:
        assert sorted(self.expected_beats) == self.expected_beats
        assert len(self.expected_beats) >= 2
        assert 0.0 <= self.expected_beats[0] <= 3.0
        assert 0.0 <= self.duration_seconds() - self.expected_beats[-1] <= 3.0

    @classmethod
    def from_path(cls, wav_path: Path) -> "Wav":
        name = wav_path.name
        data, sample_rate = soundfile.read(wav_path, dtype="int16")
        expected_beats = read_expected_beats(wav_path)

        return cls(
            name=name,
            sample_rate=sample_rate,
            data=data,
            expected_beats=expected_beats,
        )

    def duration_seconds(self) -> float:
        return len(self.data) / self.sample_rate

    def get_beats(self) -> list[BeatSample]:
        with NamedTemporaryFile(suffix=".wav") as tmp:
            soundfile.write(tmp, self.data, self.sample_rate)
            tmp.flush()
            return get_beats(Path(tmp.name))

    def stretch(self, ratio: float) -> "Wav":
        new_data = rubberband.stretch(
            self.data,
            rate=self.sample_rate,
            ratio=ratio,
            crispness=5,
            formants=False,
            precise=True,
        )
        new_beats = [t * ratio for t in self.expected_beats]
        return replace(
            self,
            data=new_data,
            expected_beats=new_beats,
        )

    def compare(self) -> BeatComparison:
        return BeatComparison.compare(self.expected_beats, self.get_beats())

    def scale(self, k: float) -> "Wav":
        # TODO: saturating math?
        return replace(
            self,
            data=self.data * k,
        )

    @classmethod
    def concat(cls, *wavs: "Wav") -> "Wav":
        if not wavs:
            raise ValueError("must provide at least one Wav")
        sample_rate = wavs[0].sample_rate
        if not all(wav.sample_rate == sample_rate for wav in wavs):
            raise ValueError("all wavs must have the same sample rate")
        name = "|".join(wav.name for wav in wavs)
        data = np.concatenate([wav.data for wav in wavs])

        expected_beats: list[float] = []
        t = 0.0
        for wav in wavs:
            expected_beats.extend(b + t for b in wav.expected_beats)
            t += wav.duration_seconds()

        return cls(
            name=name,
            sample_rate=sample_rate,
            data=data,
            expected_beats=expected_beats,
        )


def test_frontier_wav() -> None:
    example_path = BEAT_TEST_BIN.parent / "../../src/lib/test/frontier.wav"
    wav = Wav.from_path(example_path)
    measured = wav.get_beats()
    for b in measured:
        if b.is_beat:
            print(b)
    comparison = wav.compare()
    print(comparison)
    assert comparison.exact_match

    for ratio in [0.5, 0.7, 0.9, 1.1, 1.3, 1.5, 1.9]:
        print(ratio, wav.stretch(ratio).compare())

    concat = Wav.concat(wav, wav, wav, wav)
    for b in concat.get_beats():
        if b.is_beat:
            print(b)
    print(concat.expected_beats)
    print("concat", concat.compare())


if __name__ == "__main__":
    test_frontier_wav()
