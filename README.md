# SSRC - An audiophile-grade sample rate converter

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/shibatch/SSRC)

**You can download Windows binaries from [Releases](https://github.com/shibatch/SSRC/releases) (under Assets)**.

**If you have any thoughts or comments about this project, feel free to post them in [Discussions](https://github.com/shibatch/SSRC/discussions)**.

Shibatch Sample Rate Converter (SSRC) is a fast and high-quality sample rate converter for PCM WAV files. It is designed to efficiently handle the conversion between commonly used sampling rates such as 44.1kHz and 48kHz while ensuring minimal sound quality degradation.

## Features

- **High-Quality Conversion**: Achieves excellent audio quality with minimal artifacts.
- **FFT-Based Algorithm**: Utilizes a unique [FFT-based algorithm](ALGORITHM.md) for precise and efficient sample rate conversion.
- **SleefDFT Integration**: Leverages [SleefDFT](https://sleef.org/dft.xhtml), a product of the [SLEEF Project](https://sleef.org/), for fast Fourier transforms (FFT), enabling high-speed conversions.
- **SIMD Optimization**: Takes advantage of SIMD (Single Instruction, Multiple Data) techniques for accelerated processing. It is capable of high-speed conversion using [AVX-512](https://en.wikipedia.org/wiki/AVX-512).
- **Dithering Functionality**: Supports various dithering techniques, including noise shaping based on the absolute threshold of hearing (ATH) curve.
- **Specialized Filters**: Implements high-order filters to address the challenges of converting between 44.1kHz and 48kHz.
- **Selectable Conversion Profile**: You can select the filter lengths and computing precision. Single-precision computation is generally sufficient for audio processing, and even the standard profile allows for highly accurate conversion. However, since this tool is designed for audiophiles, you can also select a profile that performs all computation in double precision.
- **Low-Latency Real-Time Processing**: Suitable for demanding real-time applications by combining minimum-phase filters with an efficient partitioned convolution algorithm.


## Why SSRC?

Sampling rates of 44.1kHz (used in CDs) and 48kHz (used in DVDs) are widely used, but their conversion ratio (147:160) requires highly sophisticated algorithms to maintain quality. SSRC addresses this challenge by using an FFT-based approach, coupled with SleefDFT and SIMD optimization, to achieve a balance between speed and audio fidelity.

See [here](https://shibatch.org/ssrc) for some experimental results.


## Getting Started

### Prerequisites

- **Operating System**: SSRC is compatible with multiple platforms.
- **Audio Files**: Input files should be in PCM WAV format.

### Installation

1. Download the latest release from the [GitHub repo](https://github.com/shibatch/ssrc/).
2. Extract the downloaded archive to a directory of your choice.
3. Ensure the `ssrc` executable is accessible from your command line.

### Usage

The basic command structure is as follows:

```bash
ssrc [options] <source_file.wav> <destination_file.wav>
```

You can also use standard input and output:

```bash
cat input.wav | ssrc --stdin [options] --stdout > output.wav
```

#### Options

| Option                     | Description                                                                                    |
|----------------------------|------------------------------------------------------------------------------------------------|
| `--rate <sampling rate>`   | Specify the output sampling rate in Hz. Example: `48000`.                                      |
| `--att <attenuation>`      | Attenuate the output signal in decibels (dB). Default: `0`.                                    |
| `--bits <number of bits>`  | Specify the output quantization bit depth. Common values are `16`, `24`, `32`. Use `-32` or `-64` for 32-bit or 64-bit IEEE floating-point output. Default: `16`. |
| `--dither <type>`          | Select a dithering/noise shaping algorithm by ID. Use `--dither help` to see all available types for different sample rates. |
| `--mixChannels <matrix>`   | Mix, re-route, or change the number of channels. See the "Channel Mixing" section below for details and examples. |
| `--pdf <type> [<amp>]`     | Select a Probability Distribution Function (PDF) for dithering. `0`: Rectangular, `1`: Triangular. Default: `0`. |
| `--profile <name>`         | Select a conversion quality/speed profile. Use `--profile help` for details. Default: `standard`. |
| `--minPhase`               | Use minimum-phase filters instead of the default linear-phase filters, which makes the processing delay negligible. |
| `--partConv <log2len>`     | Divide a long filter into smaller sub-filters so that they can be applied without significant processing delays. |
| `--dstContainer <name>`    | Specify the output file container type (`riff`, `w64`, `rf64`, etc.). Use `--dstContainer help` for options. Defaults to the source container or `riff`. |
| `--genImpulse ...`         | For testing. Generate an impulse signal instead of reading a file.                             |
| `--genSweep ...`           | For testing. Generate a sweep signal instead of reading a file.                                |
| `--stdin`                  | Read audio data from standard input.                                                           |
| `--stdout`                 | Write audio data to standard output.                                                           |
| `--quiet`                  | Suppress informational messages.                                                               |
| `--debug`                  | Print detailed debugging information during processing.                                        |
| `--seed <number>`          | Set the random seed for dithering to ensure reproducible results.                              |

##### Example

Convert a WAV file from 44.1kHz to 48kHz with dithering:

```bash
ssrc --rate 48000 --dither 0 input.wav output.wav
```

#### Conversion Profiles

Profiles allow you to balance between conversion speed and quality (stop-band attenuation and filter length).

| Profile Name | FFT Length | Attenuation | Precision | Use Case                               |
|--------------|------------|-------------|-----------|----------------------------------------|
| `insane`     | 262144     | 200 dB      | double    | Highest possible quality, very slow.   |
| `high`       | 65536      | 170 dB      | double    | Excellent quality for audiophiles.     |
| `long`       | 32768      | 145 dB      | double    | Superb quality.                        |
| `standard`   | 16384      | 145 dB      | single    | Great quality, default setting.        |
| `short`      | 4096       | 96 dB       | single    | Good quality.                          |
| `fast`       | 1024       | 96 dB       | single    | Good quality, suitable for most uses.  |
| `lightning`  | 256        | 96 dB       | single    | Low latency, suitable for real-time uses. |

You can see all profiles and their technical details by running `ssrc --profile help`.


#### Channel Mixing (`--mixChannels`)

The `--mixChannels` option allows you to mix, re-route, or change the number of channels using a matrix string.

- **Syntax**: The matrix string is a series of numbers separated by commas (`,`) and semicolons (`;`).
  - Commas (`,`) separate the gain values for each column in a row.
  - Semicolons (`;`) separate the rows.
- **Logic**:
  - The number of rows in the matrix defines the number of output channels.
  - The number of columns in the matrix must match the number of input channels.

##### Example 1: Stereo to Mono Downmix
To combine a 2-channel stereo input into a 1-channel mono output, you can use a 1-row, 2-column matrix. The standard formula is `Mono = 0.5 * Left + 0.5 * Right`.
```bash
--mixChannels '0.5,0.5'
```

##### Example 2: Mono to Stereo
To duplicate a 1-channel mono input into a 2-channel stereo output, you can use a 2-row, 1-column matrix.
```bash
--mixChannels '1;1'
```
This sets both the left and right output channels to be equal to the mono input channel.

##### Example 3: Swapping Stereo Channels
To swap the left and right channels of a stereo file, you need a 2x2 matrix. The goal is to make the new left channel equal to the old right channel, and the new right channel equal to the old left channel.
```bash
--mixChannels '0,1;1,0'
```
- The first row `0,1` means `Output0 = (0 * Input0) + (1 * Input1)`.
- The second row `1,0` means `Output1 = (1 * Input0) + (0 * Input1)`.


## Spectrum Analyzer (`scsa`)

The project includes `scsa`, a command-line spectrum analyzer. While it can be used as a general-purpose analyzer, it is primarily designed for automated testing and verification, for example in a CI environment.

#### Purpose and Features

- **Automated Testing**: The primary purpose of `scsa` is to check audio spectra against predefined criteria, making it ideal for automated quality assurance in a CI/CD pipeline.
- **Cross-Platform and Dependency-Free**: As a command-line tool, it does not rely on any GUI libraries or have OS-specific dependencies, making it highly portable and easy to integrate into various workflows.
- **SVG Output for Debugging**: When a test fails, `scsa` can generate an SVG image of the spectrum. This visual output is extremely useful for identifying the cause of the failure. An SVG is also generated if no check file is provided, allowing `scsa` to be used as a general-purpose analyzer.
- **High-Precision Analysis**: Unlike many standard analyzers, all internal processing is performed in double precision. This minimizes the impact of floating-point noise, allowing for highly accurate measurements.
- **High-Resolution Windowing**: It uses a 7-term Blackman-Harris window function, which provides excellent dynamic range and frequency resolution, enabling very sharp and precise spectrum analysis.

#### Usage

```bash
scsa [<options>] <source file name> <first position> <last position> <interval>
```

#### Options

| Option                     | Description                                                                                    |
|----------------------------|------------------------------------------------------------------------------------------------|
| `--log2dftlen <log2dftlen>`| Set the log2 of the DFT length. Default: 12.                                                   |
| `--check <check file>`     | Specify a file containing spectrum check criteria.                                             |
| `--svgout <svg file name>` | Specify the output SVG file name for the spectrum graph.                                       |
| `--debug`                  | Print detailed debugging information during processing.                                        |

#### Check File Format

The check file is a plain text file that defines the spectral criteria for the `scsa` tool. Each line in the file specifies a single constraint.

- **Format**: Each constraint is defined on a new line with the following format:
  `<low_freq> <high_freq> <comparison> <threshold_db>`
  - `<low_freq>`: The lower bound of the frequency range in Hz (double).
  - `<high_freq>`: The upper bound of the frequency range in Hz (double).
  - `<comparison>`: The comparison operator, which must be either `<` (less than) or `>` (greater than).
  - `<threshold_db>`: The threshold value in decibels (double).

- **Comments**: Lines starting with a `#` character are treated as comments and are ignored. Empty lines are also ignored.

- **Logic**: For a check to pass, **all** frequency components of the signal that fall within the `[low_freq, high_freq]` range must satisfy the condition.
  - If the comparison is `<`, every spectral point in the range must be below the threshold.
  - If the comparison is `>`, every spectral point in the range must be above the threshold.

- **Example**:
  ```
  # This is a comment
  # Check for stop-band attenuation
  1 9900 < -140

  # Check for pass-band flatness (hypothetical)
  # 20 20000 > -1
  ```
  In this example, the tool will check if the spectrum is below -140 dB in the frequency range from 1 Hz to 9900 Hz. The second rule is commented out, so it will be ignored.

## For Developers (Library Usage)

In addition to the command-line tool, SSRC provides powerful C++ and C APIs, allowing you to integrate the resampling engine directly into your own projects. It can be built as a static or shared library for native applications on Windows (without requiring MSYS/Cygwin), Linux, and other platforms.

For detailed information on the API, please see [**API_DOCUMENTATION.md**](API_DOCUMENTATION.md).

### C++ API (`ssrc.hpp`)
A modern, C++17 API that uses templates and standard library features for flexible and type-safe audio processing pipelines.

### C API (`ssrcsoxr.h`)
A C-language API that is compatible with the popular SoX Resampler library (`libsoxr`). By defining `SSRC_LIBSOXR_EMULATION`, SSRC can serve as a drop-in replacement for `soxr` in existing projects.

### How to Build the Library

1. Clone the repository:
    ```bash
    git clone https://github.com/shibatch/ssrc
    cd ssrc
    ```
2. Make a separate directory to create an out-of-source build:
    ```bash
    mkdir build && cd build
    ```
3. Run cmake to configure the project:
    ```bash
    cmake .. -DCMAKE_INSTALL_PREFIX=../../install
    ```
4. Run make to build and install the project:
    ```bash
    make && make install
    ```

### Building on Windows

1. Download and install Visual Studio Community 20XX.
  * Choose "Desktop development with C++" option in Workloads pane.
  * Choose "C++ CMake tools for Windows" in Individual components
    pane.
  * Choose "C++ Clang Compiler for Windows" in Individual components
    pane.

2. Create a build directory, launch Developer Command Prompt for VS
  20XX and move to the build directory.

3. Clone the repository:
    ```bat
    git clone https://github.com/shibatch/ssrc
    cd ssrc
    ```

4. Run the batch file for building with Clang on Windows.
    ```bat
    winbuild-clang.bat -DCMAKE_BUILD_TYPE=Release
    ```

5. Copy libomp.dll to the directory where the exe file is located.


## Credits

This project uses the following third-party library:

- [**dr_wav**](https://github.com/mackron/dr_libs): A public domain single-file library for working with .wav files. The library is used in this project for reading and writing WAV files.

## Support

- **Email**: [shibatch@users.sourceforge.net](mailto:shibatch@users.sourceforge.net)

## License

The software is distributed under the Boost Software License, Version
1.0. See accompanying file LICENSE.txt or copy at :
http://www.boost.org/LICENSE_1_0.txt. Contributions to this project
are accepted under the same license.

The fact that our software is released under an open source license
only means that you can use the current and older versions of the
software for free. If you want us to continue maintaining our
software, you need to financially support our project. Please see our
[Code of Conduct](https://github.com/shibatch/nofreelunch?tab=coc-ov-file)
or its [introduction video](https://youtu.be/35zFfdCuBII).

Copyright [Naoki Shibata](https://shibatch.org/) and contributors.
