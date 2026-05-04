# WaveDiff examples

A minimal end-to-end demo for the `vamp_wavediff` plugin. The script
generates a reference signal, a deliberately altered candidate, and a
ready-to-analyse interleaved version — using only the Python standard
library, so there are no extra dependencies to install.

## Generate the wave files

```sh
python3 examples/generate_example.py
```

This writes three files under `examples/out/`:

| File             | Layout      | Contents                                          |
| ---------------- | ----------- | ------------------------------------------------- |
| `reference.wav`  | 1 ch, 48 kHz | 2 s, 440 Hz sine at -6 dBFS                       |
| `candidate.wav`  | 1 ch, 48 kHz | reference + white noise at -40 dBFS               |
| `merged.wav`     | 2 ch, 48 kHz | interleaved `(A, B)` — what the plugin consumes   |

`merged.wav` skips the SoX-based `scripts/merger.sh` step so you can run
the plugin even on machines without SoX installed.

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
