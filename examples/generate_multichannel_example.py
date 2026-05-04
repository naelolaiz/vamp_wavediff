#!/usr/bin/env python3
"""Generate a stereo demo dataset for the vamp_wavediff plugin.

Where generate_example.py shows the single-pair (mono) case, this script
exercises the plugin's multichannel layout — channels interleaved in
pairs (A_0, B_0, A_1, B_1) so the plugin reports per-pair metrics.

Two pairs are intentionally given different defects so the per-pair
metrics tell different stories:

    Pair 0 (Left):  candidate = reference + white noise at -30 dBFS
                    -> small null residual (noise floor)
    Pair 1 (Right): candidate = reference * 0.5  (-6 dB level mismatch)
                    -> large null residual; peak |A-B| = 0.5 * peak A

Outputs (under OUT_DIR, default examples/out_multichannel/):

    reference_stereo.wav    2 ch  (L=440 Hz, R=660 Hz, both -6 dBFS)
    candidate_stereo.wav    2 ch  (L=ref+noise, R=ref*0.5)
    merged.wav              4 ch  (A_L, B_L, A_R, B_R) — plugin-ready
    plot.svg                Min/max envelopes of A, B, A-B for each pair

Usage:
    python3 examples/generate_multichannel_example.py [OUT_DIR]

Plugin run with vamp-simple-host:
    vamp-simple-host -s vamp_wavediff:wavediff merged.wav  # bin 0 + 1
"""

from __future__ import annotations

import math
import random
import sys
from pathlib import Path

# Reuse the helpers and constants from the simple example, so the two
# scripts cannot drift apart.
sys.path.insert(0, str(Path(__file__).parent))
from generate_example import (  # noqa: E402  -- after sys.path tweak
    SAMPLE_RATE,
    block_rms,
    db_to_lin,
    write_plot_svg,
    write_wave_pcm16,
)

DURATION_S = 2.0
SIGNAL_DBFS = -6.0
LEFT_FREQ_HZ = 440.0
RIGHT_FREQ_HZ = 660.0

# Defect intensities — chosen so the two pairs have very different metrics.
LEFT_NOISE_DBFS = -30.0     # noise floor, modest null residual
RIGHT_GAIN_DB = -6.0        # candidate is half-amplitude (large null)
RNG_SEED = 0


def main() -> int:
    out_dir = (
        Path(sys.argv[1]) if len(sys.argv) > 1 else Path("examples/out_multichannel")
    )
    out_dir.mkdir(parents=True, exist_ok=True)

    n = int(SAMPLE_RATE * DURATION_S)
    sig_amp = db_to_lin(SIGNAL_DBFS)
    noise_amp = db_to_lin(LEFT_NOISE_DBFS)
    right_gain = db_to_lin(RIGHT_GAIN_DB)

    rng = random.Random(RNG_SEED)
    ref_L = [
        sig_amp * math.sin(2.0 * math.pi * LEFT_FREQ_HZ * i / SAMPLE_RATE)
        for i in range(n)
    ]
    ref_R = [
        sig_amp * math.sin(2.0 * math.pi * RIGHT_FREQ_HZ * i / SAMPLE_RATE)
        for i in range(n)
    ]

    cand_L = [ref_L[i] + noise_amp * (2.0 * rng.random() - 1.0) for i in range(n)]
    cand_R = [right_gain * ref_R[i] for i in range(n)]

    diff_L = [ref_L[i] - cand_L[i] for i in range(n)]
    diff_R = [ref_R[i] - cand_R[i] for i in range(n)]

    # Plugin-expected channel layout: (A_L, B_L, A_R, B_R).
    merged = [ref_L, cand_L, ref_R, cand_R]

    ref_path = out_dir / "reference_stereo.wav"
    cand_path = out_dir / "candidate_stereo.wav"
    merged_path = out_dir / "merged.wav"
    plot_path = out_dir / "plot.svg"

    write_wave_pcm16(ref_path, [ref_L, ref_R])
    write_wave_pcm16(cand_path, [cand_L, cand_R])
    write_wave_pcm16(merged_path, merged)

    diff_L_peak = max(abs(min(diff_L)), abs(max(diff_L))) or 1e-3
    diff_R_peak = max(abs(min(diff_R)), abs(max(diff_R))) or 1e-3

    # Per-block RMS of each diff (8192-sample blocks, matching the plugin's
    # default step) for overlay on its own panel.
    diff_L_rms = block_rms(diff_L, 8192)
    diff_R_rms = block_rms(diff_R, 8192)

    # Six panels: A, B, A-B for each of the two pairs.
    write_plot_svg(
        plot_path,
        panels=[
            (f"Pair 0  L  A — {LEFT_FREQ_HZ:g} Hz sine, {SIGNAL_DBFS:g} dBFS",
             ref_L, 1.0),
            (f"Pair 0  L  B — A + white noise, {LEFT_NOISE_DBFS:g} dBFS",
             cand_L, 1.0),
            (f"Pair 0  L  A - B  (peak ≈ {diff_L_peak:.4f})",
             diff_L, diff_L_peak),
            (f"Pair 1  R  A — {RIGHT_FREQ_HZ:g} Hz sine, {SIGNAL_DBFS:g} dBFS",
             ref_R, 1.0),
            (f"Pair 1  R  B — A × {right_gain:.3f}  ({RIGHT_GAIN_DB:g} dB)",
             cand_R, 1.0),
            (f"Pair 1  R  A - B  (peak ≈ {diff_R_peak:.4f})",
             diff_R, diff_R_peak),
        ],
        # Overlay the per-block RMS on the L diff panel; the R diff is
        # large enough that its envelope is already self-explanatory.
        rms_overlay=(2, diff_L_rms),
        total_samples=n,
    )

    # Quick console summary so the user knows what to expect from the plugin.
    a_rms = sig_amp / math.sqrt(2.0)
    rms_diff_L = noise_amp / math.sqrt(3.0)
    rms_diff_R = (1.0 - right_gain) * a_rms
    peak_diff_R = (1.0 - right_gain) * sig_amp

    print(f"Wrote {ref_path}")
    print(f"Wrote {cand_path}")
    print(f"Wrote {merged_path}  (4-ch: A_L, B_L, A_R, B_R)")
    print(f"Wrote {plot_path}    (open in any browser)")
    print()
    print("Expected plugin output (per-pair, bin 0 = L, bin 1 = R):")
    print(f"  RMS A         pair 0 ≈ {a_rms:.4f}    pair 1 ≈ {a_rms:.4f}")
    print(f"  RMS B         pair 0 ≈ {a_rms:.4f}    "
          f"pair 1 ≈ {right_gain * a_rms:.4f}")
    print(f"  RMS A-B       pair 0 ≈ {rms_diff_L:.4f}    "
          f"pair 1 ≈ {rms_diff_R:.4f}")
    print(f"  Peak |A-B|    pair 0 ≈ {noise_amp:.4f}    "
          f"pair 1 ≈ {peak_diff_R:.4f}")
    print()
    print("Run the plugin against the merged file:")
    print(f"  vamp-simple-host -s vamp_wavediff:wavediff {merged_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
