#ifndef WAVWRITER_HPP
#define WAVWRITER_HPP

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#include "shibatch/ssrc.hpp"
#include "dr_wav.hpp"
#include "BGExecutor.hpp"

template<typename T> class ssrc::WavWriter<T>::WavWriterImpl {
public:
  virtual ~WavWriterImpl() = default;
};

namespace shibatch {
  template<typename T>
  class WavWriterStage : public ssrc::WavWriter<T>::WavWriterImpl {
    const size_t N;
    dr_wav::WavFile wav;
    const std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> in;
    const bool mt;
    std::shared_ptr<BGExecutor> bgExecutor;
    BlockingQueue<std::vector<T>> queue;

    void thEntry() {
      const unsigned nch = wav.getNChannels();

      for(;;) {
	auto v = queue.pop();
	if (v.size() == 0) break;
	wav.writePCM(v.data(), v.size() / nch);
      }
    }

  public:
    WavWriterStage(const std::string &filename, const dr_wav::drwav_fmt &fmt, const dr_wav::Container& container,
	      const std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> &in_, uint64_t nFrames = 0, size_t bufsize = 65536, bool mt_ = true) :
      N(bufsize), wav(filename.c_str(), fmt, container, nFrames), in(in_), mt(mt_) {
      if (fmt.channels != in.size()) throw(std::runtime_error("WavWriterStage::WavWriterStage fmt.channels != in.size()"));
      if (mt) bgExecutor = std::make_shared<BGExecutor>();
    }

    void execute() {
      const unsigned nch = wav.getNChannels();

      if (!mt) {
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
      } else {
	std::thread th(&WavWriterStage::thEntry, this);
	std::vector<T> fbuf(N * nch);
	std::vector<size_t> vz(nch);
	std::vector<std::vector<T>> cbuf(nch);

	for(unsigned c=0;c<nch;c++) cbuf[c].resize(N);

	for(;;) {
	  for(unsigned c=0;c<nch;c++) {
	    bgExecutor->push(Runnable::factory([&, c](void *p) {
	      vz[c] = in[c]->read((T*)p, N);
	    }, cbuf[c].data()));
	  }
	  for(unsigned c=0;c<nch;c++) bgExecutor->pop();

	  size_t zmax = 0;
	  for(unsigned c=0;c<nch;c++) {
	    size_t z = vz[c];
	    zmax = std::max(z, zmax);
	    for(size_t i=0;i<z;i++) fbuf[i * nch + c] = cbuf[c][i];
	    for(size_t i=z;i<N;i++) fbuf[i * nch + c] = 0;
	  }
	  if (zmax == 0) break;
	  std::vector<T> v(zmax * nch);
	  memcpy(v.data(), fbuf.data(), zmax * nch * sizeof(T));
	  queue.push(std::move(v));
	}

	queue.push(std::vector<T>(0));
	th.join();
      }
    }
  };
}
#endif // #ifndef WAVWRITER_HPP
