#!/usr/bin/env python3
"""Generate a small demo dataset for the vamp_wavediff plugin.

Writes to OUT_DIR (default: examples/out/):

    reference.wav   1-channel, 48 kHz, 2 s 440 Hz sine at -6 dBFS.
    candidate.wav   Same signal plus a small amount of white noise.
    merged.wav      2-channel interleaved (A=reference, B=candidate),
                    in the layout vamp_wavediff expects, so you can feed
                    it to a Vamp host directly without scripts/merger.sh.
    plot.svg        Min/max envelope plot of A, B, and the null test
                    (A - B), with per-block RMS overlaid on the diff
                    panel. Open in any browser.

Usage:
    python3 examples/generate_example.py [OUT_DIR]

Then, with the plugin built and on $VAMP_PATH, e.g.:

    vamp-simple-host vamp_wavediff:wavediff examples/out/merged.wav -s

Uses only the Python standard library; no third-party packages required.
"""

from __future__ import annotations

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


def envelope(signal: list[float], width_px: int) -> list[tuple[float, float]]:
    """Down-sample a signal to (min, max) pairs, one per pixel column."""
    n = len(signal)
    out = []
    for px in range(width_px):
        i0 = (px * n) // width_px
        i1 = ((px + 1) * n) // width_px
        if i1 <= i0:
            i1 = i0 + 1
        chunk = signal[i0:i1]
        out.append((min(chunk), max(chunk)))
    return out


def block_rms(signal: list[float], block: int) -> list[tuple[int, float]]:
    """Return (centre_sample, RMS) per non-overlapping block of `block` samples."""
    out = []
    for start in range(0, len(signal), block):
        chunk = signal[start : start + block]
        if not chunk:
            break
        s = sum(v * v for v in chunk) / len(chunk)
        out.append((start + len(chunk) // 2, math.sqrt(s)))
    return out


def write_plot_svg(
    path: Path,
    panels: list[tuple[str, list[float], float]],
    rms_overlay: tuple[int, list[tuple[int, float]]] | None,
    total_samples: int,
) -> None:
    """SVG with one envelope panel per (label, signal, y_scale).

    rms_overlay = (panel_index, points) draws the per-block RMS as a polyline
    on top of that panel, with its own y-axis (positive only, same y_scale).
    """
    width = 1000
    panel_h = 160
    panel_gap = 24
    margin = (40, 16, 32, 64)  # top, right, bottom, left
    plot_w = width - margin[1] - margin[3]
    height = margin[0] + len(panels) * (panel_h + panel_gap) - panel_gap + margin[2]

    out = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        'font-family="sans-serif" font-size="12">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{margin[3]}" y="20" font-size="14" font-weight="bold">'
        'WaveDiff demo — A (reference), B (candidate), A - B (null test)'
        '</text>',
    ]

    for idx, (label, signal, y_scale) in enumerate(panels):
        y_top = margin[0] + idx * (panel_h + panel_gap)
        y_mid = y_top + panel_h / 2
        half = panel_h / 2 - 4

        out.append(
            f'<rect x="{margin[3]}" y="{y_top}" width="{plot_w}" height="{panel_h}" '
            'fill="#fafafa" stroke="#cfcfcf"/>'
        )
        out.append(
            f'<line x1="{margin[3]}" y1="{y_mid:.1f}" '
            f'x2="{margin[3] + plot_w}" y2="{y_mid:.1f}" stroke="#bbb"/>'
        )
        # Left labels: ±y_scale at top/bottom, 0 at middle.
        out.append(
            f'<text x="{margin[3] - 6}" y="{y_top + 12}" text-anchor="end" '
            f'fill="#555">+{y_scale:.4g}</text>'
        )
        out.append(
            f'<text x="{margin[3] - 6}" y="{y_mid + 4:.1f}" text-anchor="end" '
            'fill="#555">0</text>'
        )
        out.append(
            f'<text x="{margin[3] - 6}" y="{y_top + panel_h - 2}" '
            f'text-anchor="end" fill="#555">-{y_scale:.4g}</text>'
        )
        # Panel title.
        out.append(
            f'<text x="{margin[3] + 6}" y="{y_top + 14}" '
            f'font-weight="bold">{label}</text>'
        )

        # Envelope as a single path: vertical line per pixel column.
        env = envelope(signal, plot_w)
        d = []
        for px, (lo, hi) in enumerate(env):
            x = margin[3] + px
            yhi = y_mid - max(-1.0, min(1.0, hi / y_scale)) * half
            ylo = y_mid - max(-1.0, min(1.0, lo / y_scale)) * half
            if yhi == ylo:
                ylo += 0.5
            d.append(f'M{x},{yhi:.1f}V{ylo:.1f}')
        out.append(
            f'<path d="{"".join(d)}" stroke="#1976d2" stroke-width="1" '
            'fill="none"/>'
        )

        if rms_overlay is not None and rms_overlay[0] == idx:
            points = rms_overlay[1]
            poly = []
            for sample_idx, rms in points:
                x = margin[3] + plot_w * sample_idx / total_samples
                # RMS is non-negative; map [0, y_scale] onto upper half.
                y = y_mid - min(1.0, rms / y_scale) * half
                poly.append(f'{x:.1f},{y:.1f}')
            out.append(
                f'<polyline points="{" ".join(poly)}" stroke="#d32f2f" '
                'stroke-width="2" fill="none"/>'
            )
            out.append(
                f'<text x="{margin[3] + plot_w - 6}" y="{y_top + 14}" '
                'text-anchor="end" fill="#d32f2f">per-block RMS</text>'
            )

    # X axis label on the bottom panel.
    last_y = margin[0] + (len(panels) - 1) * (panel_h + panel_gap) + panel_h
    out.append(
        f'<text x="{margin[3]}" y="{last_y + 18}" fill="#555">0 s</text>'
    )
    out.append(
        f'<text x="{margin[3] + plot_w}" y="{last_y + 18}" '
        f'text-anchor="end" fill="#555">{total_samples / SAMPLE_RATE:.2f} s</text>'
    )

    out.append('</svg>')
    path.write_text('\n'.join(out))


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
    plot_path = out_dir / "plot.svg"

    write_wave_pcm16(ref_path, [reference])
    write_wave_pcm16(cand_path, [candidate])
    write_wave_pcm16(merged_path, [reference, candidate])

    diff = [reference[i] - candidate[i] for i in range(n)]
    diff_peak = max(abs(min(diff)), abs(max(diff))) or 1e-3
    diff_rms = block_rms(diff, 8192)
    write_plot_svg(
        plot_path,
        panels=[
            ("A — reference (440 Hz sine, -6 dBFS)", reference, 1.0),
            ("B — candidate (= A + noise at -40 dBFS)", candidate, 1.0),
            (f"A - B  (null test, peak ≈ {diff_peak:.4f})", diff, diff_peak),
        ],
        rms_overlay=(2, diff_rms),
        total_samples=n,
    )

    print(f"Wrote {ref_path}")
    print(f"Wrote {cand_path}")
    print(f"Wrote {merged_path}  (2-ch interleaved A/B for the plugin)")
    print(f"Wrote {plot_path}    (open in any browser)")
    print()
    print("Feed merged.wav to any Vamp host, e.g.:")
    print(f"  vamp-simple-host vamp_wavediff:wavediff {merged_path} -s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
