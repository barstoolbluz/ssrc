# Performance results

## Method

The final clean re-applied executable was compared with the untouched-base executable built in the same GCC 15.2.0 environment.

Representative input:

- 12,000,000 stereo PCM16 source frames at 44.1 kHz
- about 272.1 seconds / 4.54 minutes of source audio
- High profile
- 44.1 kHz -> 48 kHz
- Float64 Wave64 output
- five paired runs per mode
- base/patched order alternated by round

The environment is a shared container with five visible CPUs. Threaded timings showed substantial run-to-run scheduler noise, so paired ratios and the much more stable `--st` measurements are more informative than a single threaded run.

## `--st`

Final-clean wall seconds:

```text
base:    3.64  3.26  3.31  3.36  3.42
patched: 3.56  3.41  3.36  3.50  3.29
```

- median wall: 3.36 s base, 3.41 s patched, about +1.5%
- paired geometric-mean wall ratio: about +0.8%
- median user+system CPU: 3.35 s base, 3.39 s patched, about +1.2%
- paired geometric-mean CPU ratio: about +0.9%

This is effectively parity for the added block-level accounting and phase-origin selection.

## Normal threaded mode

Final-clean wall seconds:

```text
base:    2.52  2.26  3.85  1.72  1.93
patched: 2.75  2.63  4.17  1.57  1.67
```

The spread is too large for a precise small-delta claim. Within matched rounds the patched/base wall ratios ranged from about 0.865 to 1.164. Their geometric mean was about 1.017. The corresponding paired CPU geometric mean was about 1.024.

An immediately preceding five-pair run of the same optimized source tree, before the final clean re-apply/build timestamp changed, produced threaded geometric ratios of about 0.996 wall and 0.983 CPU. Taken together, the shared-container threaded data do not show a material throughput regression.

## Implementation cost

The repair adds counters and exact integer phase/frame arithmetic. It does not add a second PCM pass, a decode/re-encode cycle, whole-file buffering, per-file rewrite, or serialization of the asynchronous output path. An initially added redundant block-level zero-fill was removed before the final patch series after performance testing showed it was unnecessary.
