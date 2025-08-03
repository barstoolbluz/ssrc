# Shibatch Sampling Rate Converter

SSRC (Shibatch Sampling Rate Converter) is a fast and high-quality sampling rate converter for PCM WAV files. It is designed to handle the conversion between popular sampling rates such as 44.1kHz and 48kHz, ensuring minimal sound quality degradation.

## Features

- **High-Quality Conversion**: Achieves excellent audio quality with minimal artifacts.
- **FFT-Based Algorithm**: Utilizes a unique FFT-based algorithm for precise and efficient sampling rate conversion.
- **SleefDFT Integration**: Leverages SleefDFT, a product of the [SLEEF Project](https://sleef.org/), for fast Fourier transforms (FFT), enabling high-speed conversions.
- **SIMD Optimization**: Takes advantage of SIMD (Single Instruction, Multiple Data) techniques for accelerated processing.
- **Dithering Functionality**: Supports various dithering techniques, including noise shaping based on the absolute threshold of hearing (ATH) curve.
- **Specialized Filters**: Implements high-order filters to address the challenges of converting between 44.1kHz and 48kHz.

## Why SSRC?

Sampling rates of 44.1kHz (used in CDs) and 48kHz (used in professional audio and video) are widely used, but their conversion ratio (147:160) requires highly sophisticated algorithms to maintain quality. SSRC addresses this challenge by using an FFT-based approach, coupled with SleefDFT and SIMD optimization, to achieve a balance between speed and audio fidelity.

## Getting Started

### Prerequisites

- **Operating System**: SSRC is compatible with multiple platforms.
- **Audio Files**: Input files should be in PCM WAV format.

### Installation

1. Download the latest release from the [GitHub repo](https://github.com/shibatch/ssrc/).
2. Extract the downloaded archive to a directory of your choice.
3. Ensure the `ssrc` executable is accessible from your command line.

### Usage

Basic command:

```bash
ssrc [options] input.wav output.wav
```

#### Options

| Option                     | Description                                                                                    |
|----------------------------|------------------------------------------------------------------------------------------------|
| `--rate <sampling rate>`   | Specify the output sampling rate in Hz.                                                        |
| `--att <attenuation>`      | Attenuate the output in decibels (dB).                                                         |
| `--bits <number of bits>`  | Specify the output quantization bit length. Use `0` for IEEE 32-bit floating-point WAV files.  |
| `--dither <type>`          | Select dithering type:                                                                         |
|                            | `0`: Low intensity ATH-based noise shaping                                                     |
|                            | `98`: Triangular noise shaping                                                                 |
|                            | `help`: Show all available options for dithering                                               |
| `--pdf <type> [<amp>]`     | Select Probability Distribution Function (PDF) for dithering:                                  |
|                            | `0`: Rectangular                                                                               |
|                            | `1`: Triangular                                                                                |
| `--profile <type>`         | Specify a conversion profile:                                                                  |
|                            | `fast`: This setting is actually enough for almost every purpose                               |
|                            | `help`: Show all available profile options                                                     |

#### Example

Convert a WAV file from 44.1kHz to 48kHz with dithering:

```bash
ssrc --rate 48000 --dither 0 input.wav output.wav
```

## How to build

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

Copyright [Naoki Shibata](https://shibatch.github.io/) and contributors.
