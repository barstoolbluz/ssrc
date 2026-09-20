# Finite-stream timing contract

## Scope

This repair changes which samples from the existing SSRC arithmetic are published for finite file conversion. It does not change the High/Long/Insane filter coefficients, SLEEF arithmetic, FFT arithmetic, dither, channel mixing, or linear-phase mode.

## What the old implementation was doing

SSRC's resampling path combines two distinct filter stages:

- `FastPP` evaluates a symmetric polyphase FIR with input look-ahead. Relative to a causal implementation, its FIR center advances the signal on the common LCM-rate clock.
- `DFTFilter` / `PartDFTFilter` applies the long FIR causally. Its FIR center delays the signal at `fsos`.

The old implementation calculated a floating-point `delay` value, but did not use it to select the finite published timeline. The pipeline therefore serialized its startup delay and convolution flush extent. The old delay expression also added the two FIR contributions even though `FastPP`'s look-ahead contribution has the opposite sign in the published time mapping.

FFT block latency, partition size, asynchronous write buffering, and thread scheduling are processing latencies. They do not define file time origin and are not part of the published timeline contract.

## Exact time origin

Let:

- `Fs` be the source rate.
- `Fd` be the destination rate.
- `L = lcm(Fs, Fd)`.
- `fsos` be SSRC's internal oversampled rate.
- `ppfCenter = (ppfLength - 1) / 2`.
- `dftCenter = (dftLength - 1) / 2`.
- `dftTick = L / fsos` LCM-clock ticks per DFT-stage sample.
- `dstTick = L / Fd` LCM-clock ticks per destination sample.

For the default linear-phase path, the net filter latency in the common integer clock is:

```text
delayTicks = dftCenter * dftTick - ppfCenter
```

The repair decomposes that exact value as:

```text
latencyFrames = delayTicks / dstTick
phaseOffset   = delayTicks % dstTick
```

`FastPP` evaluates the destination polyphase sequence from `phaseOffset + dpos * dstep`, which removes the fractional destination-frame component. The publication layer then discards exactly `latencyFrames` generated frames. The first published frame is therefore destination timeline frame 0, not the first nonzero sample and not an amplitude-threshold crossing.

No floating-point seconds are used for these frame-domain decisions.

### High 44.1 kHz -> 48 kHz example

For the current High profile and this rate pair:

```text
L                 = 7,056,000
fsos              = 144,000
ppfLength         = 3,987
ppfCenter         = 1,993
dftLength         = 65,535
dftCenter         = 32,767
dftTick           = 49
dstTick           = 147

delayTicks        = 32,767 * 49 - 1,993
                  = 1,603,590
latencyFrames     = 10,908
phaseOffset       = 114
net old offset    = 10,908 + 114/147
                  = 10,908.775510204081... destination frames
```

A runtime impulse measurement on the unpatched executable gives a centroid offset of `10,908.775510204128` frames, matching the integer-clock derivation. After the repair, a source impulse at frame 10,000 maps to an output centroid of `10,884.353741496678`; the exact rational target is `10,000 * 48,000 / 44,100 = 10,884.353741496600`, an error of about `7.8e-11` frame.

For 48 kHz -> 44.1 kHz the same `delayTicks` is expressed on a 160-tick destination grid, giving 10,022 whole frames plus 70/160 frame.

## Finite endpoint

An input file containing `N` frames represents a finite duration of `N / Fs`. The output has the same time origin. Among destination files with an integer frame count `M`, the duration error is:

```text
abs(M / Fd - N / Fs)
```

The nearest integer to `N * Fd / Fs` minimizes that error. The implementation therefore publishes:

```text
M = round_half_up(N * Fd / Fs)
```

The calculation uses integer quotient/remainder arithmetic. Exact half-frame ties round up. This makes the endpoint rule explicit and deterministic rather than an accidental consequence of filter flush length or block boundaries.

For the required 12,544-frame 44.1 kHz -> 48 kHz case:

```text
M = round_half_up(12,544 * 48,000 / 44,100) = 13,653 frames
```

The filter stages are still allowed to consume zero extension and produce internal samples beyond this endpoint when required to calculate legitimate final in-timeline samples. The publication layer suppresses everything after frame `M - 1`.

## Minimum phase

`--minPhase` remains a separate signal-processing mode. The repair does not apply the linear-phase delay decomposition to its minimum-phase filters. Its phase origin is left unchanged (`phaseOffset = 0`, `latencyFrames = 0`), while the same finite endpoint rule limits the published file duration.

## Partitioned convolution and threading

Ordinary convolution, partitioned convolution, multithreaded execution, and `--st` use the same publication contract. Partition sizes, asynchronous blocks, and thread count do not enter either `delayTicks` or the finite endpoint calculation.

## Why the repaired output is not an integer crop of the old file

The old High 44.1 -> 48 kHz delay contains a fractional destination-frame component, 114/147 frame. Removing only 10,908 or 10,909 whole frames cannot place the old samples on the exact destination phase grid.

For a 20,000-frame impulse fixture, comparison against the nearest whole-frame crops of the old padded output produced zero byte-identical Float64 samples:

- crop at 10,908: max absolute difference about 0.86944, RMS about 0.008045;
- crop at 10,909: max absolute difference about 0.27856, RMS about 0.002613.

This is expected. The repair retains the existing FIR/FFT arithmetic but selects the correct polyphase origin before publication. It does not run a second resampler or perform a post-hoc numerical trim.
