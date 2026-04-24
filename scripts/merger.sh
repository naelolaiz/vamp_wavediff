#!/usr/bin/env bash
#
# merger.sh — interleave the channels of N same-layout wave files into a
# single file whose channels are ordered so that each "column" (corresponding
# channel across all inputs) is adjacent:
#
#   input_1 = A (C channels), input_2 = B (C channels), ...
#   output  = A1, B1, ..., A2, B2, ..., AC, BC, ...
#
# That layout is what the `vamp_wavediff` Vamp plugin consumes.
#
# Usage: merger.sh OUTPUT.wav INPUT1.wav INPUT2.wav [INPUT3.wav ...]
#
# Requires: sox, soxi (on $PATH).

set -euo pipefail

die() {
    printf 'merger.sh: %s\n' "$*" >&2
    exit 1
}

if [[ $# -lt 3 ]]; then
    cat >&2 <<USAGE
Usage: $0 OUTPUT.wav INPUT1.wav INPUT2.wav [INPUT3.wav ...]

Interleaves channels of the inputs into OUTPUT. All inputs must share the
same channel count and sample rate.
USAGE
    exit 1
fi

for cmd in sox soxi; do
    command -v "$cmd" >/dev/null 2>&1 \
        || die "required command '$cmd' not found on PATH"
done

output_file=$1
shift
inputs=("$@")

if [[ -e $output_file && ! -w $output_file ]]; then
    die "output path is not writable: $output_file"
fi

declare -i num_channels=0
declare -i sample_rate=0
declare -i first=1

for f in "${inputs[@]}"; do
    [[ -f $f ]] || die "input file not found: $f"

    ch=$(soxi -c "$f")
    sr=$(soxi -r "$f")

    if (( first )); then
        num_channels=$ch
        sample_rate=$sr
        first=0
    else
        [[ $ch -eq $num_channels ]] \
            || die "channel-count mismatch in $f ($ch vs $num_channels)"
        [[ $sr -eq $sample_rate ]] \
            || die "sample-rate mismatch in $f ($sr vs $sample_rate)"
    fi
done

# Build the remix spec so each channel-position is grouped across inputs.
remix_args=()
for (( ch=1; ch<=num_channels; ch++ )); do
    for (( i=0; i<${#inputs[@]}; i++ )); do
        remix_args+=("$(( ch + i * num_channels ))")
    done
done

sox -M "${inputs[@]}" "$output_file" remix "${remix_args[@]}"

printf 'merger.sh: wrote %s (%d inputs × %d channels = %d)\n' \
    "$output_file" "${#inputs[@]}" "$num_channels" \
    "$(( ${#inputs[@]} * num_channels ))"
