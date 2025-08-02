#ifndef WAVWRITER_HPP
#define WAVWRITER_HPP

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#include "shibatch/ssrc.hpp"
#include "dr_wav.hpp"

template<typename T> class ssrc::WavWriter<T>::WavWriterImpl {
public:
  virtual ~WavWriterImpl() = default;
};

namespace shibatch {
  template<typename T>
  class WavWriterStage : public ssrc::WavWriter<T>::WavWriterImpl {
    const size_t N;
    dr_wav::WavFile wav;
    const std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> &in;

  public:
    WavWriterStage(const char *filename, const drwav_fmt& fmt,
	      const std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> &in_, size_t bufsize = 65536) :
      N(bufsize), wav(filename, fmt), in(in_) {
      if (fmt.channels != in.size()) std::runtime_error("WavWriterStage::WavWriterStage fmt.channels != in.size()");
    }

    WavWriterStage(const drwav_fmt& fmt, uint64_t nFrames,
	      const std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> &in_, size_t bufsize = 65536) :
      N(bufsize), wav(fmt, nFrames), in(in_) {
      if (fmt.channels != in.size()) std::runtime_error("WavWriterStage::WavWriterStage fmt.channels != in.size()");
    }

    void execute() {
      const unsigned nch = wav.getNChannels();
      std::vector<T> cbuf(N), fbuf(N * nch);

      for(;;) {
	size_t zmax = 0;
	for(unsigned c=0;c<nch;c++) {
	  size_t z = in[c]->read(cbuf.data(), N);
	  zmax = std::max(z, zmax);
	  for(size_t i=0;i<z;i++) fbuf[i * nch + c] = cbuf[i];
	  for(size_t i=z;i<N;i++) fbuf[i * nch + c] = 0;
	}
	if (zmax == 0) break;
	wav.writePCM(fbuf.data(), zmax);
      }
    }

    WavWriterStage(const std::string &filename, const drwav_fmt& fmt,
		   const std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> &in_, size_t bufsize = 65536) :
      WavWriterStage(filename, fmt, in_, bufsize) {}
  };
}
#endif // #ifndef WAVWRITER_HPP
