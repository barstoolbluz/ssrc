# Test and runtime results

## Untouched-base reproduction

Base source: `b769add0756157ea88d1bbf9023e06c437485a79`.

The reported 12,544-frame, mono, Float64, High, 44.1 kHz -> 48 kHz behavior was reproduced with `--att 0.0 --bits -64`.

Base output for all three WAVE-family containers contained 35,499 frames, not the finite 13,653-frame destination duration.

- RIFF: chunks `fmt`, `data`; no `fact`; data 283,992 bytes.
- Wave64: chunks `fmt`, `data`; no `fact`; `fmt` total chunk size 40 bytes with a 16-byte IEEE-float body; data 283,992 bytes.
- RF64: chunks `ds64`, `fmt`, `data`; no `fact`; `ds64.sampleCount = 0`; data 283,992 bytes.

A separate known-position impulse confirmed the old linear-phase time offset. For a 20,000-frame source with an impulse at source frame 10,000, the old output centroid was at 21,793.129251700728. The rational destination position of the source impulse is 10,884.353741496600, giving an old offset of 10,908.775510204128 destination frames.

Threshold measurements are diagnostic only. On a 12,544-frame fixture with a centered impulse, the old 35,499-frame W64 response exceeded `1e-12` from frame 6,813 through 28,658, leaving 6,813 leading and 6,840 trailing frames outside that threshold-located response. The exact numbers depend on fixture placement and threshold and are not used by the repair.

## Patched 12,544-frame reproducer

Final clean `git am` tree: `2323ef992784ec072709c136221ac4954cafb4bd`.

All three containers publish 13,653 frames and 109,224 bytes of mono Float64 data.

- RIFF: chunks `fmt`, `fact`, `data`; `fact` body is 4-byte little-endian `13653`.
- Wave64: chunks `fmt`, `fact`, `data`; Wave64 `fact` total chunk size is 32 bytes and its body is exactly the required 8-byte little-endian `u64(13653)`.
- RF64: chunks `ds64`, `fmt`, `fact`, `data`; 32-bit `fact` body is `0xffffffff`; `ds64.sampleCount = 13653`; `ds64.dataSize = 109224`; `ds64.riffSize = 109308`.

The independent parser also validates RIFF/RF64 root sizes, Wave64 root sizes and 8-byte alignment, data extent, format tag, block alignment, and `fact` representation.

## Repository test suite

The final exported patch series was applied with `git am` to a fresh checkout of the recorded base. The resulting tree ID exactly matched the topic tree.

Release tests were then run serially in bounded segments because the execution wrapper has a per-command time ceiling. Every test completed successfully:

```text
114 / 114 passed
```

The two long Insane partitioned-convolution tests passed independently in the final clean checkout:

- `test_longnoise_44100_48000_insane_partConv`: PASS, 16.80 s
- `test_longnoise_48000_44100_insane_partConv`: PASS, 13.95 s

The command wrapper expired only when several long tests were placed in one invocation. No individual test timed out or failed.

## New focused regression test

`test_finite_stream_contract` uses its own byte-level WAVE-family parser rather than reopening outputs with SSRC/dr_wav.

It covers:

- Float32 and Float64 RIFF, Wave64, and RF64.
- Direct IEEE float and WAVE_FORMAT_EXTENSIBLE IEEE-float output.
- Correct container-specific `fact` width/count/sentinel.
- RF64 `ds64` RIFF size, data size, and 64-bit sample count.
- Wave64 root size and 8-byte chunk alignment.
- Integer PCM and extensible PCM non-regression, including absence of a new integer `fact` chunk.
- 44.1 -> 48, 48 -> 44.1, 48 -> 96, and 96 -> 44.1 kHz.
- Very short inputs, 12,544 frames, longer inputs, and lengths around rational rounding boundaries including an exact half-frame tie.
- Standard and High-like filter configurations.
- Multithreaded and `--st`-equivalent execution.
- Linear phase, minimum phase, and partitioned convolution.
- Known-position impulses, leading/trailing source silence, first/last zero samples, multichannel extent consistency, DC-like plateaus, +/-1.5, +/-2.0, and `1e-12` values.
- No clipping of above-full-scale floating output.

Final Release result:

```text
test_finite_stream_contract: PASS
```

## Numerical non-regression evidence

For a High 44.1 -> 48 kHz impulse at source frame 10,000:

- exact rational destination position: 10,884.353741496600
- patched centroid: 10,884.353741496678
- error: about 7.8e-11 frame

The patched finite output is not byte-identical to an integer crop of the old padded file because the old net delay contains a fractional destination-frame component. The nearest integer-crop comparisons are recorded in `TIMING_CONTRACT.md`.

A longer High Float64 plateau fixture used 40,000 source frames per level for `0`, `+1.5`, `-1.5`, `+2.0`, `-2.0`, and `1e-12`. In +/-1,000-output-frame windows centered well inside each plateau:

- +1.5 midpoint absolute gain error: about `9.5e-10`
- -1.5 midpoint absolute gain error: about `3.9e-10`
- +2.0 midpoint absolute gain error: about `5.3e-10`
- -2.0 midpoint absolute gain error: about `8.8e-10`
- `1e-12` midpoint absolute error: about `9.7e-22`
- maximum absolute sample anywhere in the payload: about `2.53449`

The last value confirms that above-full-scale floating-point ringing is retained rather than clipped.

## ASan

On the final clean re-applied tree, the repository's supported ASan configuration passed:

- `test_finite_stream_contract`
- `test_api`
- High 44.1 -> 48
- High 48 -> 44.1
- High partitioned-convolution 44.1 -> 48
- High partitioned-convolution 48 -> 44.1

No ASan error was reported in these runs.

## UBSan

A separate `-fsanitize=undefined -fno-sanitize-recover=all` build of the final clean tree ran the new finite-stream test successfully:

```text
finite stream contract: PASS
```

A wider CLI UBSan run is blocked by a pre-existing, unrelated defect at `src/cli/cli.cpp:317` when the CLI handles negative floating-point bit selectors:

```text
runtime error: shift exponent -65 is negative
```

The minimal reproducer exits under UBSan before the resampling repair is exercised. This patch series does not change that CLI code because it is outside the requested writer/timeline repair.

## Evidence classification

Direct runtime evidence in this delivery includes:

- base and patched container chunk/frame measurements;
- base and patched impulse measurements;
- final 114-test Release result;
- independent container/timeline regression result;
- ASan results;
- focused UBSan result and the unrelated CLI UBSan reproducer;
- long plateau numerical measurements;
- executable SHA-256;
- performance measurements.

Source/static reasoning includes:

- the LCM-clock derivation of filter latency;
- the distinction between FastPP look-ahead, DFT FIR group delay, FFT/partition processing latency, and finite publication extent;
- the finite-duration endpoint convention;
- close-time metadata backpatch ordering after the asynchronous writer drains.
