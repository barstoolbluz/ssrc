#ifndef SHIBATCH_SSRC_HPP
#define SHIBATCH_SSRC_HPP

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>

namespace ssrc {
  struct WavFormat {
    static const inline uint16_t PCM = 0x0001, IEEE_FLOAT = 0x0003;

    uint16_t formatTag, channels;
    uint32_t sampleRate, avgBytesPerSec;
    uint16_t blockAlign, bitsPerSample, extendedSize, validBitsPerSample;
    uint32_t channelMask;
    uint8_t subFormat[16];

    WavFormat() = default;

    WavFormat(const WavFormat &w) :
      formatTag(w.formatTag), channels(w.channels), sampleRate(w.sampleRate), avgBytesPerSec(w.avgBytesPerSec),
      blockAlign(w.blockAlign), bitsPerSample(w.bitsPerSample), extendedSize(w.extendedSize),
      validBitsPerSample(w.validBitsPerSample), channelMask(w.channelMask) {
      for(int i=0;i<16;i++) subFormat[i] = w.subFormat[i];
    }

    WavFormat(uint16_t formatTag_, uint16_t channels_, uint32_t sampleRate_, uint16_t bitsPerSample_, uint8_t *subFormat_ = nullptr) :
      formatTag(formatTag_), channels(channels_), sampleRate(sampleRate_), avgBytesPerSec(0),
      blockAlign((uint32_t)channels_ * bitsPerSample_ / 8), bitsPerSample(bitsPerSample_),
      extendedSize(0), validBitsPerSample(0), channelMask(0) {
      if (subFormat_) memcpy(subFormat, subFormat_, sizeof(subFormat));
    }
  };

  struct ContainerFormat {
    static const inline uint16_t RIFF = 0x1000, RIFX = 0x1001, W64 = 0x1002;
    static const inline uint16_t RF64 = 0x1003, AIFF = 0x1004;

    uint16_t c;

    ContainerFormat(uint16_t c_) : c(c_) {}
    operator uint16_t() const { return c; }
  };

  inline std::string to_string(ContainerFormat c) {
    switch(c.c) {
    case ContainerFormat::RIFF: return "RIFF";
    case ContainerFormat::RIFX: return "RIFX";
    case ContainerFormat::W64:  return "W64";
    case ContainerFormat::RF64: return "RF64";
    case ContainerFormat::AIFF: return "AIFF";
    default:                    return "N/A";
    }
  }

  struct NoiseShaperCoef {
    const int fs, id;
    const char *name;
    const int len;
    const double coefs[64];
  };

  template<typename T>
  class StageOutlet {
  public:
    virtual ~StageOutlet() = default;
    virtual bool atEnd() = 0;

    /**
     * Returns 0 only when EOF.
     * If not EOF and no data is available for reading, it must block.
     */
    virtual size_t read(T *ptr, size_t n) = 0;
  };

  template<typename T>
  class OutletProvider {
  public:
    virtual ~OutletProvider() = default;
    virtual std::shared_ptr<StageOutlet<T>> getOutlet(uint32_t channel) = 0;
    virtual WavFormat getFormat() = 0;
    virtual ContainerFormat getContainer() { return ContainerFormat(0); }
  };

  class DoubleRNG {
  public:
    virtual double nextDouble() = 0;
    virtual void fill(double *ptr, size_t n) {
      for(;n>0;n--) *ptr++ = nextDouble();
    }
    virtual ~DoubleRNG() = default;
  };

  template<typename REAL>
  class SSRC : public StageOutlet<REAL> {
  public:
    class SSRCImpl;
    SSRC(std::shared_ptr<StageOutlet<REAL>> inlet_, int64_t sfs_, int64_t dfs_,
	 unsigned log2dftfilterlen_ = 10, double aa_ = 80, double guard_ = 1);
    ~SSRC();
    bool atEnd();
    size_t read(REAL *ptr, size_t n);
  private:
    std::shared_ptr<class SSRCImpl> impl;
  };

  template<typename T>
  class WavReader : public OutletProvider<T> {
  public:
    class WavReaderImpl;
    WavReader(const char *filename);
    WavReader(const std::string &filename) : WavReader(filename.c_str()) {}
    WavReader();
    ~WavReader();
    std::shared_ptr<StageOutlet<T>> getOutlet(uint32_t channel);
    WavFormat getFormat();
    ContainerFormat getContainer();
  private:
    std::shared_ptr<class WavReaderImpl> impl;
  };

  template<typename T>
  class WavWriter {
  public:
    class WavWriterImpl;
    WavWriter(const char *filename, const WavFormat& fmt, const ContainerFormat& cont_,
	      const std::vector<std::shared_ptr<StageOutlet<T>>> &in_);
    WavWriter(const std::string &filename, const WavFormat& fmt, const ContainerFormat& cont_,
	      const std::vector<std::shared_ptr<StageOutlet<T>>> &in_) :
      WavWriter(filename.c_str(), fmt, cont_, in_) {}
    WavWriter(const WavFormat& fmt, const ContainerFormat& cont_, uint64_t nFrames,
	      const std::vector<std::shared_ptr<StageOutlet<T>>> &in_);
    ~WavWriter();
    void execute();
  private:
    std::shared_ptr<class WavWriterImpl> impl;
  };

  std::shared_ptr<DoubleRNG> createTriangularRNG(double peak = 1.0,
    uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count());

  template<typename OUTTYPE, typename INTYPE>
  class Dither : public StageOutlet<OUTTYPE> {
  public:
    class DitherImpl;
    Dither(std::shared_ptr<StageOutlet<INTYPE>> in_, double gain_, int32_t offset_, int32_t clipMin_, int32_t clipMax_,
	   const ssrc::NoiseShaperCoef *coef_, std::shared_ptr<DoubleRNG> rng_ = createTriangularRNG());
    ~Dither();
    bool atEnd();
    size_t read(OUTTYPE *ptr, size_t n);
  private:
    std::shared_ptr<class DitherImpl> impl;
  };
}
#endif // #ifndef SHIBATCH_SSRC_HPP
