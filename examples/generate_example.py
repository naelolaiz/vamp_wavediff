#!/usr/bin/env python3
"""Generate a small demo dataset for the vamp_wavediff plugin.

Writes three 16-bit PCM wave files to OUT_DIR (default: examples/out/):

    reference.wav   1-channel, 48 kHz, 2 s 440 Hz sine at -6 dBFS.
    candidate.wav   Same signal plus a small amount of white noise.
    merged.wav      2-channel interleaved (A=reference, B=candidate),
                    in the layout vamp_wavediff expects, so you can feed
                    it to a Vamp host directly without scripts/merger.sh.

Usage:
    python3 examples/generate_example.py [OUT_DIR]

Then, with the plugin built and on $VAMP_PATH, e.g.:

    vamp-simple-host vamp_wavediff:wavediff examples/out/merged.wav -s

Uses only the Python standard library (math, random, struct, wave); no
third-party packages required.
"""

import math
import random
import struct
import sys
import wave
from pathlib import Path

SAMPLE_RATE = 48000
DURATION_S = 2.0
FREQ_HZ = 440.0
SIGNAL_DBFS = -6.0
NOISE_DBFS = -40.0
RNG_SEED = 0  # deterministic output


def db_to_lin(db: float) -> float:
    return 10.0 ** (db / 20.0)


def write_wave_pcm16(path: Path, channels: list[list[float]]) -> None:
    """Write 16-bit PCM. `channels` is one list per channel; all same length."""
    n = len(channels[0])
    for c in channels:
        if len(c) != n:
            raise ValueError("all channels must have equal length")

    interleaved = [0] * (n * len(channels))
    idx = 0
    for i in range(n):
        for c in channels:
            v = c[i]
            if v > 1.0:
                v = 1.0
            elif v < -1.0:
                v = -1.0
            interleaved[idx] = int(round(v * 32767.0))
            idx += 1

    payload = struct.pack(f"<{len(interleaved)}h", *interleaved)
    with wave.open(str(path), "wb") as f:
        f.setnchannels(len(channels))
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(payload)


def main() -> int:
    out_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("examples/out")
    out_dir.mkdir(parents=True, exist_ok=True)

    n = int(SAMPLE_RATE * DURATION_S)
    sig_amp = db_to_lin(SIGNAL_DBFS)
    noise_amp = db_to_lin(NOISE_DBFS)

    rng = random.Random(RNG_SEED)
    reference = [
        sig_amp * math.sin(2.0 * math.pi * FREQ_HZ * i / SAMPLE_RATE)
        for i in range(n)
    ]
    candidate = [
        reference[i] + noise_amp * (2.0 * rng.random() - 1.0) for i in range(n)
    ]

    ref_path = out_dir / "reference.wav"
    cand_path = out_dir / "candidate.wav"
    merged_path = out_dir / "merged.wav"

    write_wave_pcm16(ref_path, [reference])
    write_wave_pcm16(cand_path, [candidate])
    write_wave_pcm16(merged_path, [reference, candidate])

    print(f"Wrote {ref_path}")
    print(f"Wrote {cand_path}")
    print(f"Wrote {merged_path}  (2-ch interleaved A/B for the plugin)")
    print()
    print("Feed merged.wav to any Vamp host, e.g.:")
    print(f"  vamp-simple-host vamp_wavediff:wavediff {merged_path} -s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
