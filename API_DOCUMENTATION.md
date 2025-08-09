# SSRC API Documentation

This document provides documentation for both the SSRC command-line tool and its underlying C++ library, `libshibatchsrc`.

## 1. Command-Line Tool (`ssrc`)

The `ssrc` executable is a powerful command-line tool for high-quality sample rate conversion of WAV files.

### 1.1. Basic Usage

The basic command structure is as follows:

```bash
ssrc [options] <source_file.wav> <destination_file.wav>
```

You can also use standard input and output:

```bash
cat input.wav | ssrc --stdin [options] --stdout > output.wav
```

### 1.2. Options

The tool offers several options to control the conversion process:

| Option                     | Description                                                                                    |
|----------------------------|------------------------------------------------------------------------------------------------|
| `--rate <sampling rate>`   | **Required**. Specify the output sampling rate in Hz. Example: `48000`.                        |
| `--att <attenuation>`      | Attenuate the output signal in decibels (dB). Default: `0`.                                    |
| `--bits <number of bits>`  | Specify the output quantization bit depth. Common values are `16`, `24`, `32`. Use `-32` or `-64` for 32-bit or 64-bit IEEE floating-point output. Default: `16`. |
| `--dither <type>`          | Select a dithering/noise shaping algorithm by ID. Use `--dither help` to see all available types for different sample rates. |
| `--pdf <type> [<amp>]`     | Select a Probability Distribution Function (PDF) for dithering. `0`: Rectangular, `1`: Triangular. Default: `0`. |
| `--profile <name>`         | Select a conversion quality/speed profile. Use `--profile help` for details. Default: `standard`. |
| `--dstContainer <name>`    | Specify the output file container type (`riff`, `w64`, `rf64`, etc.). Use `--dstContainer help` for options. Defaults to the source container or `riff`. |
| `--genImpulse ...`         | For testing. Generate an impulse signal instead of reading a file.                             |
| `--genSweep ...`           | For testing. Generate a sweep signal instead of reading a file.                                |
| `--stdin`                  | Read audio data from standard input.                                                           |
| `--stdout`                 | Write audio data to standard output.                                                           |
| `--quiet`                  | Suppress informational messages.                                                               |
| `--debug`                  | Print detailed debugging information during processing.                                        |
| `--seed <number>`          | Set the random seed for dithering to ensure reproducible results.                              |

### 1.3. Conversion Profiles

Profiles allow you to balance between conversion speed and quality (stop-band attenuation and filter length).

| Profile Name | FFT Length | Attenuation | Precision | Use Case                               |
|--------------|------------|-------------|-----------|----------------------------------------|
| `insane`     | 262144     | 200 dB      | double    | Highest possible quality, very slow.   |
| `high`       | 65536      | 170 dB      | double    | Excellent quality for audiophiles.     |
| `standard`   | 16384      | 145 dB      | single    | Great quality, default setting.        |
| `fast`       | 1024       | 96 dB       | single    | Good quality, suitable for most uses.  |
| `lightning`  | 256        | 96 dB       | single    | Fastest option, for quick previews.    |

You can see all profiles and their technical details by running `ssrc --profile help`.

### 1.4. Example

Convert a 44.1kHz, 16-bit WAV file to a 96kHz, 24-bit WAV file using a high-quality profile and dithering.

```bash
ssrc --rate 96000 --profile high --bits 24 --dither 0 "path/to/input.wav" "path/to/output.wav"
```

## 2. C++ Library API (`libshibatchsrc`)

The `ssrc` tool is built on top of the `libshibatchsrc` C++ library. You can use this library directly in your own projects to perform sample rate conversion without shelling out to an external command. The library is header-only and uses templates to support both single-precision (`float`) and double-precision (`double`) processing.

### 2.1. Core Concept: The Processing Pipeline

The library is designed around a pipeline concept. Audio data flows through a series of processing stages. The core abstraction is `ssrc::StageOutlet<T>`, an abstract class that represents a source of audio data that can be read from.

The typical pipeline looks like this:

`WavReader` -> `SSRC` -> `Dither` -> `WavWriter`

-   **`WavReader`**: Reads a `.wav` file and acts as the starting `StageOutlet`.
-   **`SSRC`**: Takes a `StageOutlet` as input (the reader) and performs sample rate conversion. It is also a `StageOutlet` itself.
-   **`Dither`**: Takes the `SSRC`'s output and applies dithering. It is also a `StageOutlet`.
-   **`WavWriter`**: Takes one or more final `StageOutlet`s and writes the data to a `.wav` file.

### 2.2. Pipeline Topology and Data Flow

A key detail of the implementation, as seen in `src/cli/cli.cpp`, is that the pipeline **branches** after the `WavReader` to process each audio channel in parallel. A separate `SSRC` (and `Dither`, if used) instance is created for each channel. The `WavWriter` is then responsible for **merging** these parallel streams back into a single, interleaved audio file.

For a stereo (2-channel) file, the topology looks like this:

```text
                                 +------------------------+
                             +-->| SSRC<T> for Channel 0  |--+
                             |   +------------------------+  |
                             |                             |
+-----------+     +----------+                             +------------+
| WavReader |-->--| (Fork)   |                             | WavWriter  |--> output.wav
+-----------+     +----------+                             +------------+
                             |                             |
                             |   +------------------------+  |
                             +-->| SSRC<T> for Channel 1  |--+
                                 +------------------------+
```

-   **Forking**: The `WavReader` provides a separate `StageOutlet` for each channel (`reader->getOutlet(0)`, `reader->getOutlet(1)`, etc.). This is where the pipeline splits.
-   **Parallel Processing**: Each channel is processed independently. This design is clean and allows for channel-specific processing if needed.
-   **Merging**: The `WavWriter` takes a `std::vector` of `StageOutlet`s in its constructor. It reads one sample from each outlet in turn to reconstruct the interleaved audio data needed for the final WAV file.

#### Data Type Lifecycle
The data type of the samples flowing through the pipeline also follows a specific path:
1.  **Floating-Point Processing**: The initial processing stages (`WavReader`, `SSRC`) are templated and typically operate on `float` or `double`. This maintains high precision during the most critical calculations (resampling).
2.  **Conversion to Integer**: If dithering is used, the `Dither` stage is responsible for converting the high-precision floating-point signal into an integer signal.
3.  **Unified Integer Type (`int32_t`)**: Crucially, the `Dither` stage always outputs `int32_t`, regardless of the final target bit depth (e.g., 16-bit or 24-bit). This simplifies the design, as any downstream stages (like the `WavWriter`) only need to handle a single, universal integer type. The `WavWriter` is then responsible for taking the `int32_t` data and correctly writing it to the file with the specified final bit depth.

### 2.3. Key Classes

All necessary classes are available by including one header:
```cpp
#include "shibatch/ssrc.hpp"
```

#### `ssrc::WavReader<T>`
Reads audio data from a WAV file. `T` can be `float` or `double`.

```cpp
// Constructor for reading from a file
ssrc::WavReader<float> reader("input.wav");

// Get format information
ssrc::WavFormat format = reader.getFormat();
int channels = format.channels;
int sampleRate = format.sampleRate;

// Get an outlet for a specific channel
std::shared_ptr<ssrc::StageOutlet<float>> channel_outlet = reader.getOutlet(0); // For channel 0
```

#### `ssrc::SSRC<T>`
The main sample rate converter.

```cpp
// Create a resampler
// - inlet: The source outlet (e.g., from WavReader)
// - sfs: Source frequency (e.g., 44100)
// - dfs: Destination frequency (e.g., 96000)
// - log2dftfilterlen, aa, guard: Profile parameters (see cli.cpp for examples)
auto resampler = std::make_shared<ssrc::SSRC<float>>(
    reader.getOutlet(0),
    44100,
    96000,
    14,   // log2dftfilterlen (from "standard" profile)
    145,  // aa (stop-band attenuation)
    2.0   // guard factor
);
```

#### `ssrc::WavWriter<T>`
Writes audio data from one or more outlets to a WAV file.

```cpp
// Create a vector of outlets (one for each channel)
std::vector<std::shared_ptr<ssrc::StageOutlet<float>>> outlets;
outlets.push_back(resampler); // Add the resampler for channel 0
// ... add resamplers for other channels ...

// Define the output format
ssrc::WavFormat dstFormat(ssrc::WavFormat::PCM, channels, 96000, 24); // 96kHz, 24-bit PCM
ssrc::ContainerFormat dstContainer(ssrc::ContainerFormat::RIFF);

// Create the writer
ssrc::WavWriter<float> writer("output.wav", dstFormat, dstContainer, outlets);

// Execute the entire pipeline (read -> process -> write)
writer.execute();
```

### 2.4. Complete Example

Here is a complete example that ties everything together. It reads a WAV file, resamples all its channels from 44.1kHz to 96kHz using single-precision floats, and saves the result as a 24-bit PCM WAV file.

```cpp
#include <iostream>
#include <vector>
#include <memory>
#include "shibatch/ssrc.hpp"

void convert_file(const std::string& in_path, const std::string& out_path) {
    try {
        // 1. Set up the reader for single-precision floats
        auto reader = std::make_shared<ssrc::WavReader<float>>(in_path);
        ssrc::WavFormat srcFormat = reader->getFormat();

        // 2. Define destination format and conversion parameters
        int dstRate = 96000;
        int dstBits = 24;

        ssrc::WavFormat dstFormat(ssrc::WavFormat::PCM, srcFormat.channels, dstRate, dstBits);
        ssrc::ContainerFormat dstContainer(ssrc::ContainerFormat::RIFF);

        // 3. Create a resampler for each channel
        std::vector<std::shared_ptr<ssrc::StageOutlet<float>>> outlets;
        for (int i = 0; i < srcFormat.channels; ++i) {
            auto resampler = std::make_shared<ssrc::SSRC<float>>(
                reader->getOutlet(i),
                srcFormat.sampleRate,
                dstRate,
                14,   // log2dftfilterlen for "standard" profile
                145,  // aa for "standard" profile
                2.0   // guard for "standard" profile
            );
            outlets.push_back(resampler);
        }

        // 4. Set up the writer
        auto writer = std::make_shared<ssrc::WavWriter<float>>(out_path, dstFormat, dstContainer, outlets);

        // 5. Execute the entire process
        std::cout << "Converting " << in_path << " to " << out_path << "..." << std::endl;
        writer->execute();
        std::cout << "Conversion complete." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
  if (argc == 3) {
    convert_file(argv[1], argv[2]);
    return 0;
  }

  std::cerr << "Usage : " << argv[0] << " <source wav file> <dest wav file>" << std::endl;

  return -1;
}
```

## 3. Advanced Topics

This section delves deeper into specific components of the `libshibatchsrc` API.

### 3.1. Dithering with the `Dither` Class

Converting high-resolution audio to a lower bit depth (e.g., 24-bit to 16-bit) involves quantization, where sample values are rounded to the nearest available level. This process creates quantization errors that manifest as distortion correlated with the original signal, which is musically unpleasant, especially on fading reverb tails.

Dithering is a technique that mitigates this by adding a small amount of uncorrelated noise to the signal prior to quantization. This crucial step trades the harsh, signal-correlated distortion for a more benign and constant noise floor.

To take this a step further, noise shaping can be employed. This process intelligently sculpts the noise floor, pushing the noise energy away from the frequency ranges where the human ear is most sensitive (e.g., 2-5 kHz) and into the far less audible, very high frequencies. While this may physically increase the total noise energy in the system, it results in a significantly lower perceived noise level. This combined process preserves low-level detail and the sense of resolution in the final audio.

The `ssrc::Dither` class is a pipeline stage that performs this function. It takes a high-resolution signal as input (e.g., from the `SSRC` stage) and outputs a quantized signal.

**Pipeline with Dither:**

`WavReader` -> `SSRC` -> `Dither` -> `WavWriter`

**Usage:**

The `Dither` class is templated on its output and input types: `Dither<OUTTYPE, INTYPE>`. Typically, `INTYPE` is `float` or `double` (from the resampler) and `OUTTYPE` is `int32_t` (a standard integer type for PCM data).

```cpp
#include "shibatch/shapercoefs.h" // Required for noise shaper coefficients

// ... inside your conversion function ...

// Assume 'resampler' is a std::shared_ptr<ssrc::SSRC<float>> from the previous stage.
auto gain = (1LL << (16 - 1)) - 1; // For 16-bit output
auto clipMin = -(1LL << (16 - 1));
auto clipMax = (1LL << (16 - 1)) - 1;

// Find the appropriate noise shaper coefficients for the destination sample rate.
const ssrc::NoiseShaperCoef* shaper = nullptr;
for (int i = 0; ssrc::noiseShaperCoef[i].fs >= 0; ++i) {
    if (ssrc::noiseShaperCoef[i].fs == dstRate && ssrc::noiseShaperCoef[i].id == 0) {
        shaper = &ssrc::noiseShaperCoef[i];
        break;
    }
}

if (!shaper) {
    // Handle case where no suitable shaper is found for the target rate
    throw std::runtime_error("No suitable noise shaper found for the destination sample rate.");
}

// Create the dither stage
auto dither_stage = std::make_shared<ssrc::Dither<int32_t, float>>(
    resampler,         // Input outlet
    gain,              // Gain to apply before quantization
    0,                 // DC offset (0 for standard PCM)
    clipMin,           // Minimum clipping value
    clipMax,           // Maximum clipping value
    shaper             // The noise shaper coefficients
);

// Now, pass `dither_stage` to the WavWriter instead of `resampler`.
// The WavWriter's template type should also be int32_t.
// std::vector<std::shared_ptr<ssrc::StageOutlet<int32_t>>> outlets;
// outlets.push_back(dither_stage);
// auto writer = std::make_shared<ssrc::WavWriter<int32_t>>(...);
// writer->execute();
```

### 3.2. Precision: `float` vs. `double`

Most key classes in the library are templated with a `typename T` or `typename REAL`, such as `SSRC<REAL>`, `WavReader<T>`, and `WavWriter<T>`. This template parameter controls the floating-point precision used for internal calculations. You can instantiate these classes with either `float` (single-precision) or `double` (double-precision).

-   **`float` (Single Precision)**
    -   **Pros**: Faster execution, lower memory usage.
    -   **Cons**: Less precision.
    -   **Use Case**: For most standard audio applications, `float` is perfectly sufficient. The "standard" profile and below use `float` by default. The precision of a 32-bit float is already far greater than that of 24-bit audio, so it does not typically become a bottleneck for quality.

-   **`double` (Double Precision)**
    -   **Pros**: Extremely high precision, theoretically higher audio quality.
    -   **Cons**: Slower execution (can be 1.5x to 2x slower), higher memory usage.
    -   **Use Case**: For archival purposes or when working in a signal chain that requires the absolute highest fidelity (e.g., for scientific analysis or a "cost-no-object" audiophile setup). The "high" and "insane" profiles use `double`.

**Example:**
To use double-precision, simply change the template parameter throughout your pipeline:

```cpp
// Double-precision pipeline
auto reader = std::make_shared<ssrc::WavReader<double>>(in_path);
// ...
auto resampler = std::make_shared<ssrc::SSRC<double>>(
    reader->getOutlet(i),
    // ... SSRC parameters ...
);
// ...
// Note: Dither still takes a float/double and outputs int32_t
auto dither_stage = std::make_shared<ssrc::Dither<int32_t, double>>(resampler, ...);
```

### 3.3. `WavFormat` and `ContainerFormat`

These two structs work together to describe the audio data's format and the file structure that contains it.

#### `ssrc::ContainerFormat`
This struct specifies the overall file type by defining its main **`ChunkID`**. This ID tells parsers what kind of file they are dealing with. The choice of container is passed to the underlying `dr_wav` library.

- `ContainerFormat::RIFF`: The `ChunkID` is `'RIFF'`. This is the classic WAV format, but it is limited to a maximum file size of 4 GB.
- `ContainerFormat::W64`: Sony Wave64 format. This is one of several competing formats designed to exceed the 4GB limit using 64-bit addressing.
- `ContainerFormat::RF64`: An extension of RIFF that is also 64-bit compatible. It is designed to be backwards-compatible with systems that don't recognize it.
- `ContainerFormat::AIFF`: Audio Interchange File Format, used by Apple.
- `ContainerFormat::RIFX`: A big-endian variant of RIFF.

Choosing a 64-bit compatible container like `RF64` or `W64` is essential if your output file might be larger than 4 GB.

#### `ssrc::WavFormat`
This struct's contents correspond directly to the data stored in a WAV file's **`fmt ` chunk**. It describes the specific properties of the raw audio data itself.

- `formatTag`: The audio codec. Key values are:
    - `WavFormat::PCM`: Standard Pulse Code Modulation.
    - `WavFormat::IEEE_FLOAT`: 32-bit or 64-bit floating-point samples.
    - `WavFormat::EXTENSIBLE`: A newer format tag used for audio that doesn't fit the classic `PCM` specification, such as multi-channel audio (more than 2 channels).
- `channels`: Number of audio channels.
- `sampleRate`: The sample rate in Hz (e.g., 44100).
- `bitsPerSample`: The bit depth (e.g., 16, 24, 32).
- `channelMask`: A bitmask specifying the speaker layout for multi-channel audio (e.g., `0x3F` for 5.1 surround, `0x63F` for 7.1 surround). Only used when `formatTag` is `EXTENSIBLE`.
- `subFormat`: A GUID specifying the sub-format, also used with `EXTENSIBLE`. The library provides constants for PCM and IEEE Float (`KSDATAFORMAT_SUBTYPE_PCM` and `KSDATAFORMAT_SUBTYPE_IEEE_FLOAT`).

**Example: Creating a format for a 5.1 Surround, 24-bit, 48kHz WAV file**

```cpp
// This requires the extensible format.
uint32_t channelMask_5_1 = 0x3F; // Front L/R, Center, LFE, Back L/R
ssrc::WavFormat format_5_1(
    ssrc::WavFormat::EXTENSIBLE,
    6,     // channels
    48000, // sampleRate
    24,    // bitsPerSample
    channelMask_5_1,
    ssrc::WavFormat::KSDATAFORMAT_SUBTYPE_PCM
);

// Use a container that supports large files, like W64 or RF64
ssrc::ContainerFormat container(ssrc::ContainerFormat::RF64);

// auto writer = std::make_shared<ssrc::WavWriter<float>>(..., format_5_1, container, ...);
```

### 3.4. Custom Dithering with `DoubleRNG`

The dithering process relies on a stream of random numbers to generate the dither noise. The library provides a default triangular PDF random number generator. However, you can provide your own by implementing the `ssrc::DoubleRNG` abstract base class.

This allows you to experiment with different types of noise (e.g., Gaussian, or different distributions) for dithering.

**Interface:**

The `DoubleRNG` interface is very simple:
```cpp
class DoubleRNG {
public:
    virtual double nextDouble() = 0; // Must return a random double
    virtual ~DoubleRNG() = default;
};
```

**Example: Implementing a Simple Uniform RNG**

Here is how you could implement a simple RNG that produces a uniform distribution between -1.0 and 1.0 and use it in the dither stage.

```cpp
#include <random>

// 1. Implement the DoubleRNG interface
class MyUniformRNG : public ssrc::DoubleRNG {
private:
    std::mt19937 engine_;
    std::uniform_real_distribution<double> dist_;

public:
    MyUniformRNG(uint64_t seed = 0) : engine_(seed), dist_(-1.0, 1.0) {}

    double nextDouble() override {
        return dist_(engine_);
    }
};

// ... inside your conversion function ...

// 2. Create an instance of your custom RNG
auto my_rng = std::make_shared<MyUniformRNG>(/* seed */);

// 3. Pass it to the Dither constructor
auto dither_stage = std::make_shared<ssrc::Dither<int32_t, float>>(
    resampler,
    // ... other dither parameters ...
    shaper,
    my_rng // Your custom RNG
);
```

### 3.5. `SSRC` Constructor Parameters

The `SSRC` constructor takes three parameters that define the conversion profile, controlling the trade-off between quality and speed.

`SSRC(inlet, sfs, dfs, log2dftfilterlen, aa, guard)`

-   **`unsigned log2dftfilterlen`**
    -   **Description**: The base-2 logarithm of the FFT filter length. The actual filter length is `1 << log2dftfilterlen`.
    -   **Impact**: This is the most significant parameter for quality. A longer filter (higher `log2dftfilterlen`) allows for a steeper, more precise anti-aliasing filter, which better removes unwanted frequencies. However, it dramatically increases computational cost.
    -   **Values**: Typical values range from `8` ("lightning") to `18` ("insane").

-   **`double aa` (Stop-band Attenuation)**
    -   **Description**: The required attenuation in the stop-band, measured in decibels (dB). The stop-band is the range of frequencies that should be completely eliminated by the filter.
    -   **Impact**: A higher value (e.g., 170 dB) results in a "blacker" background with less aliasing noise, at the cost of increased computational complexity.
    -   **Values**: Typical values range from `96` dB to `200` dB.

-   **`double guard` (Guard Band)**
    -   **Description**: A factor that determines the width of the guard band between the pass-band (frequencies to keep) and the stop-band (frequencies to remove).
    -   **Impact**: A larger guard band makes the filter's job easier, allowing for faster computation, but it does so by slightly narrowing the range of frequencies that are passed through. This is mostly relevant for conversions between very close sample rates (like 44.1kHz and 48kHz).
    -   **Values**: Typical values range from `1.0` to `8.0`.

These parameters are bundled together in the command-line tool's "profiles". When using the library directly, you can mix and match these values to create a custom profile tailored to your specific needs.

### 3.6. Implementing Custom Processing Stages with `StageOutlet`

The entire `libshibatchsrc` library is built on a simple but powerful design pattern: the **processing pipeline**. Audio data flows from a source, through one or more processing stages, to a destination. Each of these stages is connected by a unified interface: `ssrc::StageOutlet<T>`.

This interface is the fundamental building block of the library. `WavReader` is a `StageOutlet`, `SSRC` is a `StageOutlet`, and `Dither` is a `StageOutlet`. By making your own class that implements this interface, you can create custom audio effects, generators, or other processing tools and seamlessly insert them anywhere in the pipeline.

To create a custom stage, you must inherit from `ssrc::StageOutlet<T>` and implement its two pure virtual functions:

-   `virtual bool atEnd()`
    -   **Purpose**: This function should return `true` if the stream has no more data to provide, and `false` otherwise.
    -   **Implementation**: If your stage is processing data from an input stage, you should typically call `atEnd()` on your input and return its value. If you are generating data, you return `true` when your generation logic is complete.

-   `virtual size_t read(T *ptr, size_t n)`
    -   **Purpose**: This is the core function where data is processed and provided. It should fill the provided buffer `ptr` with up to `n` samples of audio data.
    -   **Return Value**: It must return the number of samples that were actually written to `ptr`. A return value of `0` signals that the stream has ended (i.e., `atEnd()` is now `true`).
    -   **Blocking**: If no data is currently available but the stream is not at the end, this function should block until data becomes available.

**Example: Creating a Custom Gain (Volume) Stage**

Here is a complete example of a simple gain stage. It reads data from an input outlet, multiplies each sample by a constant factor, and can be inserted anywhere in the pipeline.

```cpp
#include <memory>
#include "shibatch/ssrc.hpp"

template <typename T>
class GainStage : public ssrc::StageOutlet<T> {
private:
    std::shared_ptr<ssrc::StageOutlet<T>> inlet_; // The previous stage in the pipeline
    double gain_factor_;

public:
    // Constructor takes the input stage and the gain factor (e.g., 1.0 is no change)
    GainStage(std::shared_ptr<ssrc::StageOutlet<T>> inlet, double gain_factor)
        : inlet_(inlet), gain_factor_(gain_factor) {}

    // atEnd() is true if the input stage is at its end.
    bool atEnd() override {
        return inlet_->atEnd();
    }

    // read() fetches data from the input, applies gain, and returns it.
    size_t read(T* ptr, size_t n) override {
        // Read data from the input stage into our buffer.
        size_t samples_read = inlet_->read(ptr, n);

        // Apply the gain to each sample that was read.
        for (size_t i = 0; i < samples_read; ++i) {
            ptr[i] *= gain_factor_;
        }

        return samples_read;
    }
};

// --- How to use it in a pipeline ---

//
// WavReader -> SSRC -> GainStage -> WavWriter
//

// ... after creating the SSRC resampler ...
// auto resampler = std::make_shared<ssrc::SSRC<float>>(...);

// Create an instance of your gain stage, wrapping the resampler.
// This example reduces the volume by half (gain factor 0.5).
auto gain_stage = std::make_shared<GainStage<float>>(resampler, 0.5);

// Pass the gain stage to the writer instead of the resampler.
// std::vector<std::shared_ptr<ssrc::StageOutlet<float>>> outlets;
// outlets.push_back(gain_stage);
// auto writer = std::make_shared<ssrc::WavWriter<float>>(..., outlets);
// writer->execute();
```
