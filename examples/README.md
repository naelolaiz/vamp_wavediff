# WaveDiff examples

A minimal end-to-end demo for the `vamp_wavediff` plugin. The script
generates a reference signal, a deliberately altered candidate, and a
ready-to-analyse interleaved version — using only the Python standard
library, so there are no extra dependencies to install.

## Generate the wave files

```sh
python3 examples/generate_example.py
```

This writes four files under `examples/out/`:

| File             | Layout       | Contents                                          |
| ---------------- | ------------ | ------------------------------------------------- |
| `reference.wav`  | 1 ch, 48 kHz | 2 s, 440 Hz sine at -6 dBFS                       |
| `candidate.wav`  | 1 ch, 48 kHz | reference + white noise at -40 dBFS               |
| `merged.wav`     | 2 ch, 48 kHz | interleaved `(A, B)` — what the plugin consumes   |
| `plot.svg`       | 1000×~580 px | min/max envelope of A, B, and A-B + per-block RMS |

`merged.wav` skips the SoX-based `scripts/merger.sh` step so you can run
the plugin even on machines without SoX installed. Open `plot.svg` in any
browser to visually compare the streams — the bottom panel auto-scales to
the diff signal's peak so the noise is actually visible against the sine.

## Analyse with the plugin

With the plugin built and discoverable via `VAMP_PATH`:

```sh
vamp-simple-host vamp_wavediff:wavediff examples/out/merged.wav -s
```

You should see a steady ~0.354 RMS for both A and B (sine at -6 dBFS has
linear RMS = 10^(-6/20) / sqrt(2) ≈ 0.354) and a tiny `RMS A-B` near the
noise floor (~0.006, i.e. 10^(-40/20) / sqrt(3) for uniform noise).

## Reproducing with SoX instead

If you prefer to start from two independent files and use the merger
helper, the equivalent end-to-end pipeline is:

```sh
python3 examples/generate_example.py
scripts/merger.sh examples/out/merged-via-sox.wav \
    examples/out/reference.wav \
    examples/out/candidate.wav
vamp-simple-host vamp_wavediff:wavediff examples/out/merged-via-sox.wav -s
```

## Multichannel example

`generate_multichannel_example.py` shows the per-pair behaviour. It
writes a stereo reference, a stereo candidate with a *different* defect
on each channel, and a 4-channel `merged.wav` interleaved as
`(A_L, B_L, A_R, B_R)` — exactly the layout the plugin consumes for
multi-pair inputs.

```sh
python3 examples/generate_multichannel_example.py
vamp-simple-host -s vamp_wavediff:wavediff examples/out_multichannel/merged.wav
```

Defects (intentionally different so the per-pair metrics diverge):

| Pair | Channel | Reference (A)        | Candidate (B)              | Expected null |
| ---- | ------- | -------------------- | -------------------------- | ------------- |
| 0    | Left    | 440 Hz sine, -6 dBFS | A + white noise at -30 dBFS | small (~0.018 RMS) |
| 1    | Right   | 660 Hz sine, -6 dBFS | A × 0.5  (-6 dB mismatch)  | large (~0.177 RMS) |

The plugin reports `bin 0` for pair 0 and `bin 1` for pair 1 in each of
its four outputs (`rms_a`, `rms_b`, `rms_diff`, `peak_diff`). The
`-s` flag adds the overall summary line at the end of each output
stream. Open `plot.svg` in a browser to see the six-panel breakdown
(reference, candidate, and null-test trace per pair).

> **Heads-up for the Qt UI:** at the moment the UI parser only displays
> bin 0, so a multi-pair file will still render but you'll only see the
> Left-channel metrics there. Use `vamp-simple-host` directly for the
> full per-pair picture.
