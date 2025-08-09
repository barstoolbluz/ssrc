### Overview of the Algorithm

### 1\. Introduction

One of the key challenges in digital audio processing is the conversion between two frequencies, specifically the rational sample rate conversion where the ratio is a rational number L/M (where L and M are large integers, such as 44,100 Hz and 48,000 Hz, with a ratio of 147/160). Theoretically, the ideal method involves oversampling the source signal to the least common multiple (LCM) of the two frequencies, applying a single sharp low-pass filter (LPF), and then performing decimation. However, the enormous LCM value makes the required filter order and computational load impractical.

The proposed algorithm employs two-stage filtering to avoid the load of a single-stage LCM-based approach. This algorithm combines two FIR filters—one using a polyphase filter and the other using FFT-based fast convolution—to achieve a balance between filtering accuracy and computational efficiency. This design enables high-fidelity conversion in practical applications while remaining within computationally feasible limits.


### 2\. Proposed Method

The algorithm's core is a two-stage filtering process mediated by an intermediate sampling frequency, *fsos*. This architecture decomposes the complex filtering problem into two more manageable steps. Crucially, the order in which the filters are applied is reversed for upsampling versus downsampling, reflecting the different goals of each process: anti-imaging for upsampling and anti-aliasing for downsampling.

#### 2.1. Upsampling Process (Low Frequency *lfs* &rarr; High Frequency *hfs*)

The primary goal of upsampling is to interpolate new sample values while suppressing the spectral images created during oversampling.

1. The input signal is first oversampled via zero-insertion to the LCM frequency, *fslcm* \= lcm(*lfs*, *hfs*). This generates the original baseband signal along with numerous high-frequency spectral images.  
2. At the *fslcm* rate, a **Polyphase filter** is applied. It serves as an efficient, primary anti-imaging filter, attenuating the majority of the high-order images.  
3. The signal is then decimated to the intermediate sampling frequency *fsos*, defined as *hfs* &middot; *osm*.  
4. At the *fsos* rate, an **FIR filter is applied using a fast convolution algorithm**. This filter is engineered as a very high-order, sharp LPF. Its role is to definitively remove all frequency components above the original signal's Nyquist frequency (*lfs*/2), thereby eliminating all remaining spectral images.  
5. Finally, the clean signal is decimated to the target frequency *hfs*.


#### 2.2. Filter Characteristics and Roles

The efficacy of this algorithm stems from the complementary strengths of the two chosen filter implementations.

* **FIR Filter with Fast Convolution**: The filter itself is a standard Finite Impulse Response (FIR) filter. It is implemented using a **fast convolution** algorithm (e.g., Overlap-Add), which leverages FFT. This technique allows for the efficient application of a very high-order (long tap count) FIR filter. This enables a near-ideal LPF with an extremely sharp transition band and high stopband attenuation, making it perfect for the precise separation of frequency components.  
* **Polyphase Filter**: The polyphase structure is inherently optimized for the mechanics of sample rate conversion, efficiently combining the operations of upsampling, filtering, and downsampling. While it cannot typically achieve the same filter order as the fast convolution method, it excels at the computational task of interpolation and decimation.


#### 2.3. The *osm* Parameter and Implementation Constraints

The intermediate frequency *fsos* is defined by the parameter *osm*, where *fsos* \= *hfs* &middot; *osm*. *osm* is defined as the smallest positive integer (*osm* &ge; 1\) that satisfies the following condition:

lcm(*lfs*, *hfs*)/(*hfs* &middot; *osm*) &isin; Z

This ensures that sample positions remain on a commensurate grid between the *fslcm* and *fsos* timebases. However, the efficiency of the fast convolution stage degrades as *fsos* (and thus *osm*) increases. To manage this trade-off, this implementation imposes a constraint: **only combinations of *lfs* and *hfs* that result in *osm* &le; 3 are permitted.** This constraint means the converter is not universal; it cannot, in principle, convert between any two arbitrary frequencies. In practice, however, this design choice covers all common sampling frequencies used in audio. It is a strategic trade-off, sacrificing absolute universality for optimized performance in the most common use cases.  

### 3\. Conclusion

This algorithm for rational sample rate conversion successfully balances high fidelity and computational efficiency. By employing a multi-stage architecture with two complementary filter implementations (Polyphase and a fast convolution FIR), it strategically decomposes the filtering problem. The order of filter application is adapted to the specific requirements of upsampling and downsampling. This design, constrained by the *osm* parameter for practical efficiency, provides a robust and high-quality solution for the most common sample rate conversion tasks.
