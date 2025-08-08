#ifndef WAVREADER_HPP
#define WAVREADER_HPP

#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <mutex>

#include "shibatch/ssrc.hpp"
#include "dr_wav.hpp"

template<typename T> class ssrc::WavReader<T>::WavReaderImpl {
public:
  virtual ~WavReaderImpl() = default;
};

namespace shibatch {
  template<typename T>
  class WavReaderStage : public ssrc::WavReader<T>::WavReaderImpl {
    class WavOutlet : public ssrc::StageOutlet<T> {
      WavReaderStage &reader;
      const uint32_t ch;
      std::deque<std::vector<T>> queue;
      size_t pos = 0;

      size_t nSamplesInQueue() {
	size_t ret = 0;
	for(auto v : queue) ret += v.size();
	return ret - pos;
      }

    public:
      WavOutlet(WavReaderStage &reader_, int ch_) : reader(reader_), ch(ch_) {}
      ~WavOutlet() {}
      bool atEnd() { return queue.size() == 0 && reader.atEnd(); }

      size_t read(T *ptr, size_t n) {
	std::unique_lock<std::mutex> lock(reader.mtx);

	size_t s = nSamplesInQueue();

	if (s < n) s += reader.refill(n - s);

	if (s > n) s = n;

	for(size_t r = s;r > 0;) {
	  size_t cs = std::min(queue.front().size() - pos, r);
	  //xassert(cs != 0, "WaveOutlet::read");
	  memcpy(ptr, queue.front().data() + pos, cs * sizeof(T));
	  pos += cs;
	  ptr += cs;
	  r -= cs;
	  if (pos >= queue.front().size()) {
	    queue.pop_front();
	    pos = 0;
	  }
	}

	return s;
      }

      friend WavReaderStage;
    };

    dr_wav::WavFile wav;
    std::vector<std::shared_ptr<ssrc::StageOutlet<T>>> outlet;
    std::mutex mtx;
    std::vector<T> buf;

    size_t refill(size_t n) {
      buf.resize(std::max(n * getNChannels(), buf.size()));
      size_t z = wav.readPCM(buf.data(), n);
      unsigned nc = getNChannels();

      for(unsigned c=0;c<nc;c++) {
	auto o = std::dynamic_pointer_cast<WavOutlet>(outlet[c]);

	o->queue.push_back(std::vector<T>());
	std::vector<T> &le = o->queue.back();

	le.resize(z);
	for(size_t i=0;i<z;i++) le[i] = buf[i * nc + c];
      }

      return z;
    }

  public:
    WavReaderStage(const std::string &filename) : wav(filename.c_str()) {
      outlet.resize(getNChannels());
      for(unsigned ch=0;ch<getNChannels();ch++)
	outlet[ch] = std::make_shared<WavOutlet>(*this, ch);
    }

    WavReaderStage() : wav() {
      outlet.resize(getNChannels());
      for(unsigned ch=0;ch<getNChannels();ch++)
	outlet[ch] = std::make_shared<WavOutlet>(*this, ch);
    }

    drwav getWav() const { return wav.getWav(); }
    drwav_fmt getFmt() const { return wav.getFmt(); }
    drwav_container getContainer() const { return wav.getContainer(); }
    uint32_t getSampleRate() const { return wav.getSampleRate(); }
    uint16_t getNBitsPerSample() const { return wav.getNBitsPerSample(); }
    uint32_t getNChannels() const { return wav.getNChannels(); }
    uint32_t getNFrames() { return wav.getNFrames(); }
    bool isFloat() { return wav.isFloat(); }

    size_t getPosition() { return wav.getNFrames(); }
    bool atEnd() { return wav.atEnd(); }

    std::shared_ptr<ssrc::StageOutlet<T>> getOutlet(uint32_t channel) {
      if (channel >= outlet.size()) throw(std::runtime_error("WavReaderStage::getOutlet channel too large"));
      return outlet[channel];
    }

    ~WavReaderStage() {}
  };
}
#endif // #ifndef WAVREADER_HPP
