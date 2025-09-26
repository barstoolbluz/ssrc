#include <vector>
#include <cstring>
#include <unordered_set>
#include "SRC.hpp"
#include "WavReader.hpp"
#include "WavWriter.hpp"
#include "Dither.hpp"
#include "ChannelMixer.hpp"
#include "BGExecutor.hpp"

using namespace std;
using namespace ssrc;
using namespace shibatch;

//

namespace {
  class FactoryMadeRunnable : public Runnable {
    const function<void(void *)> f;
    void *ptr;
  public:
    FactoryMadeRunnable(function<void(void *)> f_, void *ptr_) : f(f_), ptr(ptr_) {}
    ~FactoryMadeRunnable() {}
    void run() { f(ptr); }
  };

  class BGExecutorStatic {
    const unsigned nth;
    bool initialized = false;
    vector<shared_ptr<thread>> vth;
    unordered_set<class BGExecutor *> regset;
    BlockingQueue<shared_ptr<Runnable>> queue;
    mutex mtx;

    void thEntry() {
      for(;;) {
	auto r = queue.pop();
	if (!r) break;
	r->run();
	unique_lock lock(mtx);
	if (regset.count(r->belongsTo)) r->belongsTo->queue.push(r);
      }
    }

    void reg(class BGExecutor* x) {
      unique_lock lock(mtx);
      if (!initialized) {
	initialized = true;
	for(unsigned i=0;i < nth;i++)
	  vth.push_back(make_shared<thread>(&BGExecutorStatic::thEntry, this));
      }
      regset.insert(x);
    }

    void unreg(class BGExecutor* x) {
      unique_lock lock(mtx);
      regset.erase(x);
    }

  public:
    BGExecutorStatic() : nth(thread::hardware_concurrency()) {}

    ~BGExecutorStatic() {
      if (initialized) {
	for(unsigned i=0;i < nth;i++) queue.push(nullptr);
	for(auto t : vth) t->join();
      }
    }

    friend class shibatch::BGExecutor;
  };

  BGExecutorStatic bgExecutorStatic;
}

namespace shibatch {
  shared_ptr<Runnable> Runnable::factory(function<void(void *)> f, void *p) {
    return make_shared<FactoryMadeRunnable>(f, p);
  }

  BGExecutor::BGExecutor() { bgExecutorStatic.reg(this); }
  BGExecutor::~BGExecutor() { bgExecutorStatic.unreg(this); }

  void BGExecutor::push(shared_ptr<Runnable> job) {
    job->belongsTo = this;
    bgExecutorStatic.queue.push(job);
  }

  shared_ptr<Runnable> BGExecutor::pop() { return queue.pop(); }
}

//

template<typename REAL> SSRC<REAL>::SSRC(shared_ptr<StageOutlet<REAL>> inlet_, int64_t sfs_, int64_t dfs_,
					 unsigned l2dftflen_, double aa_, double guard_, double gain_,
					 bool minPhase_, unsigned l2mindftflen_, bool mt_) :
  impl(make_shared<SSRCStage<REAL>>(inlet_, sfs_, dfs_, l2dftflen_, aa_, guard_, gain_, minPhase_, l2mindftflen_, mt_)) {}

template<typename REAL> SSRC<REAL>::~SSRC() {}

template<typename REAL> size_t SSRC<REAL>::read(REAL *ptr, size_t n) {
  return dynamic_pointer_cast<SSRCStage<REAL>>(impl)->read(ptr, n);
}

template<typename REAL> bool SSRC<REAL>::atEnd() {
  return dynamic_pointer_cast<SSRCStage<REAL>>(impl)->atEnd();
}

template<typename REAL> double SSRC<REAL>::getDelay() {
  return dynamic_pointer_cast<SSRCStage<REAL>>(impl)->getDelay();
}

//

template SSRC<float>::SSRC(shared_ptr<StageOutlet<float>>, int64_t, int64_t, unsigned, double, double, double, bool, unsigned, bool);
template SSRC<float>::~SSRC();
template size_t SSRC<float>::read(float *ptr, size_t n);
template bool SSRC<float>::atEnd();
template double SSRC<float>::getDelay();

template SSRC<double>::SSRC(shared_ptr<StageOutlet<double>>, int64_t, int64_t, unsigned, double, double, double, bool, unsigned, bool);
template SSRC<double>::~SSRC();
template size_t SSRC<double>::read(double *ptr, size_t n);
template bool SSRC<double>::atEnd();
template double SSRC<double>::getDelay();

//

template<typename T> WavReader<T>::WavReader(const std::string &filename) :
  impl(make_shared<WavReaderStage<T>>(filename)) {}

template<typename T> WavReader<T>::WavReader() :
  impl(make_shared<WavReaderStage<T>>()) {}

template<typename T> WavReader<T>::~WavReader() {}

template<typename T> std::shared_ptr<StageOutlet<T>> WavReader<T>::getOutlet(uint32_t channel) {
  return dynamic_pointer_cast<WavReaderStage<T>>(impl)->getOutlet(channel);
}

template<typename T> WavFormat WavReader<T>::getFormat() {
  dr_wav::drwav_fmt fmt = dynamic_pointer_cast<WavReaderStage<T>>(impl)->getFmt();
  ssrc::WavFormat ret;

  ret.formatTag = fmt.formatTag;
  ret.channels = fmt.channels;
  ret.sampleRate = fmt.sampleRate;
  ret.avgBytesPerSec = fmt.avgBytesPerSec;
  ret.blockAlign = fmt.blockAlign;
  ret.bitsPerSample = fmt.bitsPerSample;
  ret.extendedSize = fmt.extendedSize;
  ret.validBitsPerSample = fmt.validBitsPerSample;
  ret.channelMask = fmt.channelMask;
  memcpy(&ret.subFormat, &fmt.subFormat, sizeof(ret.subFormat));

  return ret;
}

template<typename T> ContainerFormat WavReader<T>::getContainer() {
  return ContainerFormat((uint16_t)dr_wav::Container(dynamic_pointer_cast<WavReaderStage<T>>(impl)->getContainer()));
}

//

template WavReader<float>::WavReader(const std::string &filename);
template WavReader<float>::WavReader();
template WavReader<float>::~WavReader();
template shared_ptr<StageOutlet<float>> WavReader<float>::getOutlet(uint32_t);
template WavFormat WavReader<float>::getFormat();
template ContainerFormat WavReader<float>::getContainer();

template WavReader<double>::WavReader(const std::string &filename);
template WavReader<double>::WavReader();
template WavReader<double>::~WavReader();
template shared_ptr<StageOutlet<double>> WavReader<double>::getOutlet(uint32_t);
template WavFormat WavReader<double>::getFormat();
template ContainerFormat WavReader<double>::getContainer();

//

template<typename T> WavWriter<T>::WavWriter(const std::string &filename,
					     const ssrc::WavFormat& fmt_, const ssrc::ContainerFormat& cont_,
					     const std::vector<std::shared_ptr<StageOutlet<T>>> &in_,
					     uint64_t nFrames, bool mt_) {
  dr_wav::drwav_fmt fmt;
  memcpy(&fmt, &fmt_, sizeof(fmt));
  impl = make_shared<WavWriterStage<T>>(filename, fmt, dr_wav::Container(cont_.c), in_, nFrames, 65536, mt_);
}

template<typename T> WavWriter<T>::~WavWriter() {}

template<typename T> void WavWriter<T>::execute() {
  dynamic_pointer_cast<WavWriterStage<T>>(impl)->execute();
}

//

template WavWriter<int32_t>::WavWriter(const std::string &, const ssrc::WavFormat&, const ssrc::ContainerFormat&,
				       const std::vector<std::shared_ptr<StageOutlet<int32_t>>> &, uint64_t, bool);
template WavWriter<int32_t>::~WavWriter();
template void WavWriter<int32_t>::execute();

template WavWriter<float>::WavWriter(const std::string &, const ssrc::WavFormat&, const ssrc::ContainerFormat&,
				     const std::vector<std::shared_ptr<StageOutlet<float>>> &, uint64_t, bool);
template WavWriter<float>::~WavWriter();
template void WavWriter<float>::execute();

template WavWriter<double>::WavWriter(const std::string &, const ssrc::WavFormat&, const ssrc::ContainerFormat&,
				      const std::vector<std::shared_ptr<StageOutlet<double>>> &, uint64_t, bool);
template WavWriter<double>::~WavWriter();
template void WavWriter<double>::execute();

//

template<typename OUTTYPE, typename INTYPE>
Dither<OUTTYPE, INTYPE>::Dither(std::shared_ptr<StageOutlet<INTYPE>> in_, double gain_, int32_t offset_,
				int32_t clipMin_, int32_t clipMax_,
				const ssrc::NoiseShaperCoef *coef_, std::shared_ptr<DoubleRNG> rng_) :
  impl(make_shared<DitherStage<OUTTYPE, INTYPE>>(in_, gain_, offset_, clipMin_, clipMax_, coef_, rng_)) {}

template<typename OUTTYPE, typename INTYPE> Dither<OUTTYPE, INTYPE>::~Dither() {}

template<typename OUTTYPE, typename INTYPE> bool Dither<OUTTYPE, INTYPE>::atEnd() {
  return dynamic_pointer_cast<Dither<OUTTYPE, INTYPE>>(impl)->atEnd();
}

template<typename OUTTYPE, typename INTYPE> size_t Dither<OUTTYPE, INTYPE>::read(OUTTYPE *ptr, size_t n) {
  return dynamic_pointer_cast<DitherStage<OUTTYPE, INTYPE>>(impl)->read(ptr, n);
}

//

template Dither<int32_t, float>::Dither(std::shared_ptr<StageOutlet<float>> in_, double gain_, int32_t offset_,
					int32_t clipMin_, int32_t clipMax_,
					const ssrc::NoiseShaperCoef *coef_, std::shared_ptr<DoubleRNG> rng_);
template Dither<int32_t, float>::~Dither();
template size_t Dither<int32_t, float>::read(int32_t *ptr, size_t n);
template bool Dither<int32_t, float>::atEnd();

template Dither<int32_t, double>::Dither(std::shared_ptr<StageOutlet<double>> in_, double gain_, int32_t offset_,
					 int32_t clipMin_, int32_t clipMax_,
					 const ssrc::NoiseShaperCoef *coef_, std::shared_ptr<DoubleRNG> rng_);
template Dither<int32_t, double>::~Dither();
template size_t Dither<int32_t, double>::read(int32_t *ptr, size_t n);
template bool Dither<int32_t, double>::atEnd();

//

std::shared_ptr<DoubleRNG> ssrc::createTriangularRNG(double peak, uint64_t seed) {
  return make_shared<TriangularDoubleRNG>(peak, make_shared<LCG64>(seed));
}

//

template<typename REAL> ChannelMixer<REAL>::ChannelMixer(std::shared_ptr<ssrc::OutletProvider<REAL>> in_,
							 const std::vector<std::vector<double>>& matrix_) :
  impl(make_shared<ChannelMixerStage<REAL>>(in_, matrix_)) {}

template<typename REAL> ChannelMixer<REAL>::~ChannelMixer() {}

template<typename REAL> std::shared_ptr<ssrc::StageOutlet<REAL>> ChannelMixer<REAL>::getOutlet(uint32_t c) {
  return dynamic_pointer_cast<ChannelMixerStage<REAL>>(impl)->getOutlet(c);
}

template<typename REAL> WavFormat ChannelMixer<REAL>::getFormat() {
  return dynamic_pointer_cast<ChannelMixerStage<REAL>>(impl)->getFormat();
}

//

template ChannelMixer<float>::ChannelMixer(std::shared_ptr<ssrc::OutletProvider<float>> in_,
					   const std::vector<std::vector<double>>& matrix_);
template ChannelMixer<float>::~ChannelMixer();
template std::shared_ptr<ssrc::StageOutlet<float>> ChannelMixer<float>::getOutlet(uint32_t c);
template WavFormat ChannelMixer<float>::getFormat();

template ChannelMixer<double>::ChannelMixer(std::shared_ptr<ssrc::OutletProvider<double>> in_,
					   const std::vector<std::vector<double>>& matrix_);
template ChannelMixer<double>::~ChannelMixer();
template std::shared_ptr<ssrc::StageOutlet<double>> ChannelMixer<double>::getOutlet(uint32_t c);
template WavFormat ChannelMixer<double>::getFormat();
