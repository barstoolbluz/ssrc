# SSRC API Documentation

This document provides documentation for both the SSRC command-line tool and its underlying C++ library, `libshibatch`.

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

## 2. C++ Library API (`libshibatch`)

The `ssrc` tool is built on top of the `libshibatch` C++ library. You can use this library directly in your own projects to perform sample rate conversion without shelling out to an external command. The library is header-only and uses templates to support both single-precision (`float`) and double-precision (`double`) processing.

### 2.1. Core Concept: The Processing Pipeline

The library is designed around a pipeline concept. Audio data flows through a series of processing stages. The core abstraction is `ssrc::StageOutlet<T>`, an abstract class that represents a source of audio data that can be read from.

The typical pipeline looks like this:

`WavReader` -> `SSRC` -> `Dither` -> `WavWriter`

-   **`WavReader`**: Reads a `.wav` file and acts as the starting `StageOutlet`.
-   **`SSRC`**: Takes a `StageOutlet` as input (the reader) and performs sample rate conversion. It is also a `StageOutlet` itself.
-   **`Dither`**: Takes the `SSRC`'s output and applies dithering. It is also a `StageOutlet`.
-   **`WavWriter`**: Takes one or more final `StageOutlet`s and writes the data to a `.wav` file.

### 2.2. Key Classes

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

### 2.3. Complete Example

Here is a complete example that ties everything together. It reads a WAV file, resamples all its channels from 44.1kHz to 96kHz using single-precision floats, and saves the result as a 24-bit PCM WAV file.

```cpp
#include <iostream>
#include <vector>
#include <memory>
#include "shibatch/ssrc.hpp"

// For simplicity, this example does not include dithering.
// See src/cli/cli.cpp for a more advanced example with dithering.

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
    // Note: You need to link against the library or include the source files.
    // This example assumes you have a file named "input_44100.wav".
    // convert_file("input_44100.wav", "output_96000.wav");
    return 0;
}
```

## 3. Advanced Topics

This section delves deeper into specific components of the `libshibatch` API.

### 3.1. Dithering with the `Dither` Class

When converting audio from a higher bit depth to a lower bit depth (e.g., from 24-bit to 16-bit), quantization errors can introduce audible artifacts. **Dithering** is the process of adding a small amount of carefully shaped noise before quantization to mask these artifacts and produce a more pleasant, analog-like sound.

The `ssrc::Dither` class is a pipeline stage that performs this function. It takes a high-resolution signal as input (e.g., from the `SSRC` stage) and outputs a signal ready for quantization.

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

These two structs are used to describe the properties of the audio data and its file container.

#### `ssrc::ContainerFormat`
This struct specifies the top-level file format. It's a simple wrapper around a `uint16_t`.
- `ContainerFormat::RIFF`: The standard and most common WAV file container.
- `ContainerFormat::RIFX`: A big-endian variant of RIFF.
- `ContainerFormat::W64`: Sony Wave64 format, used for files larger than 4 GB.
- `ContainerFormat::RF64`: An extension of RIFF that allows for file sizes greater than 4 GB.
- `ContainerFormat::AIFF`: Audio Interchange File Format, used by Apple.

#### `ssrc::WavFormat`
This struct holds the detailed audio format information.
- `formatTag`: The audio codec. Key values are:
    - `WavFormat::PCM`: Standard Pulse Code Modulation.
    - `WavFormat::IEEE_FLOAT`: 32-bit or 64-bit floating-point samples.
    - `WavFormat::EXTENSIBLE`: Used for formats that don't fit in the standard PCM header, such as multi-channel audio (more than 2 channels) or high bit depths (>16).
- `channels`: Number of audio channels.
- `sampleRate`: The sample rate in Hz (e.g., 44100).
- `bitsPerSample`: The bit depth (e.g., 16, 24, 32).
- `channelMask`: A bitmask specifying the speaker layout for multi-channel audio (e.g., `0x3F` for 5.1 surround). Only used when `formatTag` is `EXTENSIBLE`.
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
    MyUniformRNG(uint64_t seed) : engine_(seed), dist_(-1.0, 1.0) {}

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
