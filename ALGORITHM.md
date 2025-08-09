## Overview of the Algorithm

### 1. Introduction

One of the key challenges in digital audio processing is converting PCM signals between different sampling frequencies. This is particularly difficult for rational sample rate conversion, where the ratio is a rational number L/M (e.g., converting 44,100 Hz to 48,000 Hz involves a ratio of 147/160).

Theoretically, the ideal method is to:
1.  Upsample the source signal to the least common multiple (LCM) of the two frequencies.
2.  Apply a single, sharp, ideal low-pass filter (LPF) to remove unwanted spectral content.
3.  Downsample (or decimate) the result to the target frequency.

However, the LCM of common audio frequencies is often enormous (e.g., lcm(44100, 48000) = 7,840,000 Hz). This makes the required filter order and computational load impractically large for a single-stage approach.

To solve this, the proposed algorithm employs **two-stage filtering**. It combines two different types of FIR filters—one implemented as a **polyphase filter** and the other using **FFT-based fast convolution**—to achieve high-fidelity conversion within feasible computational limits.

### 2. The Two-Stage Process

The core of the algorithm is a two-stage filtering process that uses an intermediate sampling frequency, *fsos*. This architecture decomposes the complex filtering problem into two more manageable steps. The order of operations depends on whether we are upsampling or downsampling.

#### 2.1. Upsampling Process (Low Frequency *lfs* &rarr; High Frequency *hfs*)

The goal of upsampling is to interpolate new sample values while suppressing unwanted high-frequency **spectral images** (copies of the original signal's spectrum at higher frequencies) that are created during the process.

**Conceptual Flow:**

`Input (@lfs)` &rarr; `Polyphase Filter` &rarr; `Intermediate (@fsos)` &rarr; `Fast Conv. FIR` &rarr; `Output (@hfs)`

**Steps:**

1.  **Stage 1: Polyphase Filter:** The input signal is first processed by a polyphase filter.
    *   Conceptually, this step involves upsampling the signal to the LCM frequency (*fslcm*) and applying a low-pass filter.
    *   In practice, the polyphase structure is a highly efficient implementation that combines upsampling, filtering, and downsampling into a single operation. It avoids explicitly generating the massive intermediate signal at *fslcm*, thus saving significant computation.
    *   The output of this stage is a signal at the intermediate sampling frequency *fsos* = *hfs* &middot; *osm*.

2.  **Stage 2: Fast Convolution FIR Filter:** The signal at *fsos* is then filtered by a very high-order, sharp FIR low-pass filter.
    *   This filter is implemented using a **fast convolution** algorithm (e.g., Overlap-Add), which leverages FFTs for efficiency.
    *   Its purpose is to definitively remove all frequency components above the original signal's Nyquist frequency (*lfs*/2), eliminating any remaining spectral images with near-ideal precision.

3.  **Final Decimation:** The clean signal from Stage 2 is decimated (downsampled) to the target frequency *hfs*.

#### 2.2. Downsampling Process (High Frequency *hfs* &rarr; Low Frequency *lfs*)

For downsampling, the primary challenge is to prevent **aliasing**, where high-frequency content folds down into the audible low-frequency band after decimation. The filter order is reversed to address this.

**Conceptual Flow:**

`Input (@hfs)` &rarr; `Fast Conv. FIR` &rarr; `Intermediate (@fsos)` &rarr; `Polyphase Filter` &rarr; `Output (@lfs)`

**Steps:**

1.  **Stage 1: Fast Convolution FIR Filter:** The input signal is first upsampled to the intermediate frequency *fsos* and then processed by the sharp FIR filter.
    *   This filter acts as a very effective **anti-aliasing filter**. It sharply cuts off all frequencies above the target Nyquist frequency (*lfs*/2) *before* the final decimation occurs.

2.  **Stage 2: Polyphase Filter:** The filtered signal at *fsos* is then passed to the polyphase filter, which efficiently decimates it down to the final target frequency *lfs*.

### 3. Filter Characteristics and Roles

The efficacy of this algorithm stems from the complementary strengths of the two chosen filter implementations.

*   **FIR Filter with Fast Convolution**: This is a standard Finite Impulse Response (FIR) filter implemented using a **fast convolution** algorithm. This technique allows for the efficient application of a very high-order filter, enabling a near-ideal LPF with an extremely sharp transition band and high stopband attenuation. It is perfect for precise frequency separation, serving as the primary anti-imaging (for upsampling) or anti-aliasing (for downsampling) filter.
*   **Polyphase Filter**: The polyphase structure is inherently optimized for the mechanics of sample rate conversion. While it cannot typically achieve the same extreme filter order as the fast convolution method, it excels at the computational task by efficiently combining the operations of upsampling, filtering, and downsampling.

### 4. Filter Design and Parameters

The quality of the conversion is determined by the design of the two FIR filters. This implementation uses a Kaiser window to design the low-pass filters, guided by a few key parameters. The default values are chosen to provide high-quality audio conversion.

*   **Stopband Attenuation (`aa`)**: Defines the minimum attenuation in the stopband. The default is **96 dB**, a common value for high-fidelity audio that effectively eliminates audible artifacts.
*   **DFT Filter Length (`dftflen`)**: The length of the FIR filter implemented with fast convolution. The default is **4096 taps**. A longer filter allows for a sharper transition band.
*   **Guard Factor (`guard`)**: A parameter used to adjust the transition band of the polyphase filter. The default is **1**.

#### Polyphase Filter Design

The polyphase filter acts as the first stage in upsampling and the second in downsampling. Its characteristics are defined by the following formulas:
*   **Transition Band Width**: `(fsos - lfs) / (1.0 + guard)`
*   **Pass-band Edge Frequency**: `(fsos + (lfs - fsos)/(1.0 + guard)) / 2`

These formulas show how the `guard` parameter helps define the cutoff characteristics relative to the low frequency (`lfs`) and the intermediate frequency (`fsos`).

#### Fast Convolution FIR Filter Design

This filter provides the final, sharp filtering. Its design is based on achieving the target stopband attenuation (`aa`) given its length (`dftflen`).
1.  First, the required transition band width (`df`) for the filter is calculated based on `aa`, `fsos`, and `dftflen`.
2.  The pass-band edge frequency is then set to **`lfs / 2 - df`**. This ensures that the filter's transition band starts just below the Nyquist frequency of the lower-rate signal, providing a very sharp cutoff that prevents aliasing (in downsampling) and removes spectral images (in upsampling) with high precision.

### 5. The *osm* Parameter and Implementation Constraints

The intermediate frequency *fsos* is defined by the parameter *osm*, where *fsos* = *hfs* &middot; *osm*. The parameter *osm* is the smallest positive integer (*osm* &ge; 1) that satisfies the following condition:

lcm(*lfs*, *hfs*) / (*hfs* &middot; *osm*) &isin; Z

In simpler terms, this condition ensures that the decimation factor from the conceptual LCM frequency down to the intermediate frequency *fsos* is a whole number. This keeps sample positions on a regular grid, simplifying the process.

However, the efficiency of the fast convolution stage degrades as *fsos* (and thus *osm*) increases. To manage this trade-off, this implementation imposes a constraint: **only combinations of *lfs* and *hfs* that result in *osm* &le; 3 are permitted.** This constraint means the converter is not universal; it cannot, in principle, convert between any two arbitrary frequencies. In practice, however, this design choice covers all common sampling frequencies used in audio. It is a trade-off, sacrificing absolute universality for optimized performance in the most common use cases.

### 6. Conclusion

This algorithm for rational sample rate conversion strikes a balance between high fidelity and computational efficiency. By employing a multi-stage architecture with two complementary filter implementations (Polyphase and a fast convolution FIR), it decomposes the filtering problem into manageable parts. This design, constrained by the *osm* parameter for practical efficiency, provides a robust and high-quality solution for the most common sample rate conversion tasks in audio engineering.
