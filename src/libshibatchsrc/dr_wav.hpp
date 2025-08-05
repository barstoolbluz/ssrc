#ifndef DR_WAV_HPP
#define DR_WAV_HPP

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace dr_wav {
  class Format {
    drwav_fmt fmt;
  public:
    static const uint16_t PCM = 0x0001, IEEE_FLOAT = 0x0003, ALAW = 0x0006, MULAW = 0x0007, EXTENSIBLE = 0xFFFE;

    Format(uint16_t formatTag_, uint16_t channels_, uint32_t sampleRate_, uint16_t bitsPerSample_, uint8_t *subFormat_ = nullptr) {
      memset(&fmt, 0, sizeof(fmt));

      fmt.formatTag = formatTag_;
      fmt.channels = channels_;
      fmt.sampleRate = sampleRate_;
      fmt.bitsPerSample = bitsPerSample_;
      if (subFormat_) memcpy(fmt.subFormat, subFormat_, sizeof(fmt.subFormat));

      fmt.blockAlign = (uint32_t)channels_ * bitsPerSample_ / 8;
    }

    Format(const drwav_fmt &fmt_) : fmt(fmt_) {}

    drwav_fmt getFmt() const { return fmt; }

    friend std::string to_string(const Format &fmt) {
      std::string s = "[Format: ";
      switch(fmt.fmt.formatTag) {
      case PCM: s += "PCM"; break;
      case IEEE_FLOAT: s += "IEEE_FLOAT"; break;
      case ALAW: s += "ALAW"; break;
      case MULAW: s += "MULAW"; break;
      case EXTENSIBLE: s += "EXTENSIBLE"; break;
      default: s += "UNKNOWN"; break;
      }

      return s + ", " + std::to_string(fmt.fmt.channels) + ", " +
	std::to_string(fmt.fmt.sampleRate) + ", " +
	std::to_string(fmt.fmt.bitsPerSample) + "]";
    }

    std::ostream& operator<<(std::ostream &os) { return os << to_string(*this); }

    friend class DataFormat;
  };

  std::ostream& operator<<(std::ostream &os, const drwav_fmt &fmt) { return os << to_string(Format(fmt)); }

  class Container {
    const drwav_container c;
  public:
    Container(const Container &c_) : c(c_.c) {}
    Container(const drwav_container &c_) : c(c_) {}
    Container(const uint16_t &c_) : c(
       (c_ == 0x1001 ? drwav_container_rifx :
	(c_ == 0x1002 ? drwav_container_w64 :
	 (c_ == 0x1003 ? drwav_container_rf64 :
	  (c_ == 0x1004 ? drwav_container_aiff : drwav_container_riff))))) {}
    operator uint16_t() const {
      return (c == drwav_container_rifx ? 0x1001 :
	      (c == drwav_container_w64  ? 0x1002 :
	       (c == drwav_container_rf64 ? 0x1003 :
		(c == drwav_container_aiff ? 0x1004 : 0x1000))));
    }
    operator drwav_container() const { return c; }
  };

  struct Sample24 {
    uint8_t l, m, h;

    Sample24() : l(0), m(0), h(0) {}

    Sample24(const Sample24 &s) : l(s.l), m(s.m), h(s.h) {}

    Sample24(int32_t s) {
      if (s > +0x7fffff) s = +0x7fffff;
      if (s < -0x800000) s = -0x800000;
      l = s & 0xff; s >>= 8;
      m = s & 0xff; s >>= 8;
      h = s & 0xff;
    }

    Sample24(float f) : Sample24((int32_t)rintf(f * 0x7fffff)) {}
    Sample24(double d) : Sample24((int32_t)rint(d * 0x7fffff)) {}
  };

  struct Sample16 {
    uint8_t l, h;

    Sample16() : l(0), h(0) {}

    Sample16(const Sample16 &s) : l(s.l), h(s.h) {}

    Sample16(int32_t s) {
      if (s > +0x7fff) s = +0x7fff;
      if (s < -0x8000) s = -0x8000;
      l = s & 0xff; s >>= 8;
      h = s & 0xff;
    }

    Sample16(float f) : Sample16((int32_t)rintf(f * 0x7fff)) {}
    Sample16(double d) : Sample16((int32_t)rint(d * 0x7fff)) {}
  };

  struct Sample8 {
    uint8_t u;

    Sample8() : u(0) {}

    Sample8(const Sample8 &s) : u(s.u) {}

    Sample8(int16_t s) {
      if (s > 0xff) s = 0xff;
      if (s < 0x00) s = 0x00;
      u = (uint8_t)s;
    }

    Sample8(int32_t s) : Sample8((int16_t)s) {}
    Sample8(float f) : Sample8((int16_t)rintf(f * 0x7f + 0x80)) {}
    Sample8(double d) : Sample8((int16_t)rint(d * 0x7f + 0x80)) {}
  };

  class DataFormat {
    drwav_data_format format;
  public:
    DataFormat(uint32_t channels_, uint32_t sampleRate_, uint32_t bitsPerSample_,
	       uint32_t format_ = DR_WAVE_FORMAT_PCM, Container container_ = drwav_container_riff) {
      format.container = container_;
      format.format = format_;
      format.channels = channels_;
      format.sampleRate = sampleRate_;
      format.bitsPerSample = bitsPerSample_;
    }

    DataFormat(const Format &fmt, Container container_ = drwav_container_riff) {
      format.container = container_;
      format.format = fmt.fmt.formatTag == Format::IEEE_FLOAT ? DR_WAVE_FORMAT_IEEE_FLOAT : DR_WAVE_FORMAT_PCM;
      format.channels = fmt.fmt.channels;
      format.sampleRate = fmt.fmt.sampleRate;
      format.bitsPerSample = fmt.fmt.bitsPerSample;
    }

    DataFormat(const drwav_data_format &format_) : format(format_) {}
    DataFormat(const drwav_fmt &fmt_, Container container_ = drwav_container_riff) :
      DataFormat(Format(fmt_), container_) {}

    drwav_data_format getContent() const { return format; }

    friend class WavFile;
  };

  class WavFile {
    static constexpr const char *resultCodeString[54] = {
      "SUCCESS", "ERROR", "INVALID_ARGS", "INVALID_OPERATION",
      "OUT_OF_MEMORY", "OUT_OF_RANGE", "ACCESS_DENIED", "DOES_NOT_EXIST",
      "ALREADY_EXISTS", "TOO_MANY_OPEN_FILES", "INVALID_FILE", "TOO_BIG",
      "PATH_TOO_LONG", "NAME_TOO_LONG", "NOT_DIRECTORY", "IS_DIRECTORY",
      "DIRECTORY_NOT_EMPTY", "END_OF_FILE", "NO_SPACE", "BUSY",
      "IO_ERROR", "INTERRUPT", "UNAVAILABLE", "ALREADY_IN_USE",
      "BAD_ADDRESS", "BAD_SEEK", "BAD_PIPE", "DEADLOCK",
      "TOO_MANY_LINKS", "NOT_IMPLEMENTED", "NO_MESSAGE", "BAD_MESSAGE",
      "NO_DATA_AVAILABLE", "INVALID_DATA", "TIMEOUT", "NO_NETWORK",
      "NOT_UNIQUE", "NOT_SOCKET", "NO_ADDRESS", "BAD_PROTOCOL",
      "PROTOCOL_UNAVAILABLE", "PROTOCOL_NOT_SUPPORTED",
      "PROTOCOL_FAMILY_NOT_SUPPORTED", "ADDRESS_FAMILY_NOT_SUPPORTED",
      "SOCKET_NOT_SUPPORTED", "CONNECTION_RESET", "ALREADY_CONNECTED", "NOT_CONNECTED",
      "CONNECTION_REFUSED", "NO_HOST", "IN_PROGRESS", "CANCELLED",
      "MEMORY_ALREADY_MAPPED", "AT_END",
    };

    drwav wav;

    static size_t on_stdin_read(void* pUserData, void* pBuffer, size_t bytesToRead) {
      return fread(pBuffer, 1, bytesToRead, stdin);
    }

    static drwav_bool32 on_stdin_seek(void* pUserData, int offset, drwav_seek_origin origin) {
      int whence = SEEK_SET;
      if (origin == DRWAV_SEEK_CUR) {
        whence = SEEK_CUR;
      } else if (origin == DRWAV_SEEK_END) {
        whence = SEEK_END;
      }

      return fseek(stdin, offset, whence) == 0;
    }

    static size_t on_stdout_write(void* pUserData, const void* pData, size_t bytesToWrite) {
      return fwrite(pData, 1, bytesToWrite, stdout);
    }

  public:
    WavFile(const std::string &filename) {
      memset(&wav, 0, sizeof(wav));
      if (!drwav_init_file(&wav, filename.c_str(), NULL))
	throw(std::runtime_error(("WavFile::WavFile Could not open " + filename + " for reading").c_str()));
    }

    WavFile(const std::string &filename, const DataFormat &format) {
      memset(&wav, 0, sizeof(wav));
      if (!drwav_init_file_write(&wav, filename.c_str(), &format.format, NULL))
	throw(std::runtime_error(("WavFile::WavFile Could not open " + filename + " for writing").c_str()));
    }

    WavFile() {
      memset(&wav, 0, sizeof(wav));
#ifdef _WIN32
      _setmode(_fileno(stdin), _O_BINARY);
#endif
      if (!drwav_init(&wav, on_stdin_read, on_stdin_seek, NULL, NULL, NULL))
	throw(std::runtime_error("WavFile::WavFile Could not open STDIN for reading"));
    }

    WavFile(const DataFormat &format, uint64_t totalPCMFrameCount) {
#ifdef _WIN32
      _setmode(_fileno(stdout), _O_BINARY);
#endif
      memset(&wav, 0, sizeof(wav));
      if (!drwav_init_write_sequential_pcm_frames(&wav, &format.format, totalPCMFrameCount, on_stdout_write, NULL, NULL))
	throw(std::runtime_error("WavFile::WavFile Could not open STDOUT for writing"));
    }

    WavFile(const WavFile &&w) {
      wav = w.wav;
      memset(&wav, 0, sizeof(wav));
    }

    WavFile(const WavFile &w) = delete;

    ~WavFile() { drwav_uninit(&wav); }

    static void checkResult(drwav_int32 c, std::string s = "") {
      if (c == 0) return;
      c = -c;
      if (c < 0 || c >= (int)(sizeof(resultCodeString) / sizeof(resultCodeString[0]))) c = 1;
      s += resultCodeString[c];
      throw(std::runtime_error(s.c_str()));
    }

    drwav getWav() const { return wav; }

    drwav_fmt getFmt() const { return wav.fmt; }
    drwav_container getContainer() const { return wav.container; }

    uint32_t getSampleRate() const { return wav.fmt.sampleRate; }
    uint16_t getNBitsPerSample() const { return wav.fmt.bitsPerSample; }
    uint32_t getNChannels() const { return wav.fmt.channels; }
    bool isFloat() const { return wav.fmt.formatTag == Format::IEEE_FLOAT; }

    uint32_t getNFrames() {
      drwav_uint64 c = 0;
      checkResult(drwav_get_length_in_pcm_frames(&wav, &c), "WavFile::getNFrames");
      return c;
    }

    size_t getPosition() {
      drwav_uint64 c = 0;
      checkResult(drwav_get_cursor_in_pcm_frames(&wav, &c), "WavFile::getPosition");
      return c;
    }

    bool atEnd() { return getNFrames() == getPosition(); }

    size_t readPCM(float *ptr, size_t nFrame) {
      return drwav_read_pcm_frames_f32(&wav, nFrame, ptr);
    }

    size_t readPCM(double *ptr, size_t nFrame) {
      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 64)
	return drwav_read_pcm_frames(&wav, nFrame, ptr);

      std::vector<float> buf(nFrame * getNChannels());
      size_t ret = drwav_read_pcm_frames_f32(&wav, nFrame, buf.data());
      for(size_t i=0;i<nFrame * getNChannels();i++) ptr[i] = buf[i];
      return ret;
    }

    size_t writePCM(float *ptr, size_t nFrame) {
      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 32)
	return drwav_write_pcm_frames(&wav, nFrame, ptr);
      
      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 64) {
	std::vector<double> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * sizeof(double), buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 32) {
	std::vector<int32_t> buf(nFrame * getNChannels());
	drwav_f32_to_s32(buf.data(), ptr, nFrame * getNChannels());
	return drwav_write_pcm_frames(&wav, nFrame, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 24) {
	std::vector<Sample24> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 3, buf.data());
      }
      
      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 16) {
	std::vector<Sample16> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 2, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 8) {
	std::vector<Sample8> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 1, buf.data());
      }

      std::string s = "WavFile::writePCM(f32) Unsupported format, formatTag = ";
      s += std::to_string(wav.fmt.formatTag) + ", bitsPerSample = " + std::to_string(wav.fmt.bitsPerSample);
      throw(std::runtime_error(s.c_str()));
    }

    size_t writePCM(double *ptr, size_t nFrame) {
      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 64)
	return drwav_write_raw(&wav, nFrame * getNChannels() * sizeof(double), ptr);
      
      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 32) {
	std::vector<float> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_pcm_frames(&wav, nFrame, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 32) {
	std::vector<int32_t> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i] * (1LL << 31);
	return drwav_write_pcm_frames(&wav, nFrame, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 24) {
	std::vector<Sample24> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 3, buf.data());
      }
      
      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 16) {
	std::vector<Sample16> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 2, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 8) {
	std::vector<Sample8> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 1, buf.data());
      }

      std::string s = "WavFile::writePCM(f32) Unsupported format, formatTag = ";
      s += std::to_string(wav.fmt.formatTag) + ", bitsPerSample = " + std::to_string(wav.fmt.bitsPerSample);
      throw(std::runtime_error(s.c_str()));

    }

    size_t writePCM(int32_t *ptr, size_t nFrame) {
      if (wav.fmt.formatTag == Format::PCM && (wav.fmt.bitsPerSample == 32)) {
	return drwav_write_pcm_frames(&wav, nFrame, ptr);
      }
      
      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 32) {
	std::vector<float> buf(nFrame * getNChannels());
	drwav_s32_to_f32(buf.data(), ptr, nFrame * getNChannels());
	return drwav_write_pcm_frames(&wav, nFrame, buf.data());
      }

      if (wav.fmt.formatTag == Format::IEEE_FLOAT && wav.fmt.bitsPerSample == 64) {
	std::vector<double> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i] * (1.0 / (1 << 23));
	return drwav_write_raw(&wav, nFrame * getNChannels() * sizeof(double), buf.data());
      }
      
      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 24) {
	std::vector<Sample24> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 3, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 16) {
	std::vector<Sample16> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 2, buf.data());
      }

      if (wav.fmt.formatTag == Format::PCM && wav.fmt.bitsPerSample == 8) {
	std::vector<Sample8> buf(nFrame * getNChannels());
	for(size_t i=0;i<nFrame * getNChannels();i++) buf[i] = ptr[i];
	return drwav_write_raw(&wav, nFrame * getNChannels() * 1, buf.data());
      }

      std::string s = "WavFile::writePCM(s32) Unsupported format, formatTag = ";
      s += std::to_string(wav.fmt.formatTag) + ", bitsPerSample = " + std::to_string(wav.fmt.bitsPerSample);
      throw(std::runtime_error(s.c_str()));
    }
  };
}
#endif // #ifndef DR_WAV_HPP
