# WaveDiff

A small toolkit for **A/B comparison of audio files**, built as a
[Vamp](https://www.vamp-plugins.org/) plugin with a Qt Quick frontend.

Given two wave files — a reference (A) and a candidate (B) — WaveDiff
interleaves them into a single multichannel stream and computes, per channel
pair:

| Output            | Meaning                                         |
| ----------------- | ----------------------------------------------- |
| `RMS A`           | Block RMS of the reference stream               |
| `RMS B`           | Block RMS of the candidate stream               |
| `RMS A-B`         | Null-test RMS, i.e. RMS of the sample-wise diff |
| `Peak \|A-B\|`    | Worst-case per-sample divergence in the block   |

These metrics are reported per block (by default every 8192 samples) and as
an overall summary for the whole file. Values can optionally be reported in
decibels via a plugin parameter.

## Repository layout

```
.
├── plugin/          # Vamp plugin (C++17, builds a MODULE library)
│   └── plugins/     # WaveDiffPlugin.{h,cpp} and descriptor registration
├── ui/              # Qt 6 Quick Controls 2 frontend (Material dark)
├── tests/           # CTest unit tests for the plugin core
├── examples/        # End-to-end demo (generates ref/candidate/merged wavs)
├── scripts/         # merger.sh — SoX-based channel interleaver (CLI helper)
└── CMakeLists.txt   # Top-level orchestrator (builds plugin, UI, tests)
```

## Requirements

- CMake ≥ 3.14
- A C++17 compiler (GCC / Clang)
- [Vamp Plugin SDK](https://github.com/vamp-plugins/vamp-plugin-sdk)
  (discovered via `pkg-config` or `VAMP_SDK_PATH`)
- Qt ≥ 6.5 with `QuickControls2` (UI uses `qt_add_qml_module` and
  `QQmlApplicationEngine::loadFromModule`, both of which need 6.5)
- [SoX](http://sox.sourceforge.net/) and optionally
  [vamp-simple-host](https://code.soundsoftware.ac.uk/projects/vamp-plugin-sdk)
  for the UI pipeline

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

To build only a subsystem:

```sh
cmake -S . -B build -DWAVEDIFF_BUILD_UI=OFF       # plugin only
cmake -S . -B build -DWAVEDIFF_BUILD_PLUGIN=OFF   # UI only
cmake -S . -B build -DWAVEDIFF_BUILD_TESTS=OFF    # skip unit tests
```

If the Vamp SDK is not on `pkg-config`'s search path, point CMake at it:

```sh
cmake -S . -B build -DVAMP_SDK_PATH=/opt/vamp-sdk
```

## Tests

Unit tests for the plugin core are wired into CTest:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Example

A self-contained Python script generates a reference, a noisy candidate,
and a pre-merged 2-channel file ready to feed into a Vamp host — see
[examples/README.md](examples/README.md). Standard library only, no extra
deps.

## Install

```sh
cmake --install build --prefix "$HOME/.local"
```

The plugin is installed to `<prefix>/lib/vamp/`. Vamp-aware hosts
(Sonic Visualiser, `vamp-simple-host`, …) will find it either there or by
setting `VAMP_PATH` to the containing directory.

## Using the plugin

### From the command line

```sh
# 1. Interleave fileA and fileB (must share channel count + sample rate):
scripts/merger.sh merged.wav fileA.wav fileB.wav

# 2. Analyze with any Vamp host:
vamp-simple-host vamp_wavediff:wavediff merged.wav -s
```

### From Sonic Visualiser

Open the merged file and select **Transform → Analysis by Maker →
Natanael Olaiz → Wave Diff** to overlay the per-block metrics.

### From the UI

Run the `wavediff` executable, pick both files, and click **Run Diff**. If
`vamp-simple-host` is on `$PATH`, results are shown inline; otherwise the UI
reports the path of the merged file so you can feed it to another host.

## Channel layout

The plugin expects inputs where channels are **interleaved in pairs**:

```
channel 0 = A, stream channel 0
channel 1 = B, stream channel 0
channel 2 = A, stream channel 1
channel 3 = B, stream channel 1
...
```

That is exactly what `scripts/merger.sh` produces from two same-layout
files. The channel count must be even.

## License

Released under the **GNU General Public License v3.0 or later**. See
[LICENSE](LICENSE).
