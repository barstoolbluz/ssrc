#include <cstring>
#include "SRC.hpp"
#include "WavReader.hpp"
#include "WavWriter.hpp"
#include "Dither.hpp"

using namespace std;
using namespace ssrc;
using namespace shibatch;

template<typename REAL> SSRC<REAL>::SSRC(shared_ptr<StageOutlet<REAL>> inlet_, int64_t sfs_, int64_t dfs_,
					 unsigned l2dftflen_, double aa_, double guard_) :
  impl(make_shared<SSRCStage<REAL>>(inlet_, sfs_, dfs_, l2dftflen_, aa_, guard_)) {}

template<typename REAL> SSRC<REAL>::~SSRC() {}

template<typename REAL> size_t SSRC<REAL>::read(REAL *ptr, size_t n) {
  return dynamic_pointer_cast<SSRCStage<REAL>>(impl)->read(ptr, n);
}

template<typename REAL> bool SSRC<REAL>::atEnd() {
  return dynamic_pointer_cast<SSRCStage<REAL>>(impl)->atEnd();
}

//

template SSRC<float>::SSRC(shared_ptr<StageOutlet<float>>, int64_t, int64_t, unsigned, double, double);
template SSRC<float>::~SSRC();
template size_t SSRC<float>::read(float *ptr, size_t n);
template bool SSRC<float>::atEnd();

template SSRC<double>::SSRC(shared_ptr<StageOutlet<double>>, int64_t, int64_t, unsigned, double, double);
template SSRC<double>::~SSRC();
template size_t SSRC<double>::read(double *ptr, size_t n);
template bool SSRC<double>::atEnd();

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
  drwav_fmt fmt = dynamic_pointer_cast<WavReaderStage<T>>(impl)->getFmt();
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
					     const std::vector<std::shared_ptr<StageOutlet<T>>> &in_, uint64_t nFrames) {
  drwav_fmt fmt;
  memcpy(&fmt, &fmt_, sizeof(fmt));
  impl = make_shared<WavWriterStage<T>>(filename, fmt, dr_wav::Container(cont_.c), in_, nFrames);
}

template<typename T> WavWriter<T>::~WavWriter() {}

template<typename T> void WavWriter<T>::execute() {
  dynamic_pointer_cast<WavWriterStage<T>>(impl)->execute();
}

//

template WavWriter<int32_t>::WavWriter(const std::string &, const ssrc::WavFormat&, const ssrc::ContainerFormat&,
				       const std::vector<std::shared_ptr<StageOutlet<int32_t>>> &, uint64_t);
template WavWriter<int32_t>::~WavWriter();
template void WavWriter<int32_t>::execute();

template WavWriter<float>::WavWriter(const std::string &, const ssrc::WavFormat&, const ssrc::ContainerFormat&,
				     const std::vector<std::shared_ptr<StageOutlet<float>>> &, uint64_t);
template WavWriter<float>::~WavWriter();
template void WavWriter<float>::execute();

template WavWriter<double>::WavWriter(const std::string &, const ssrc::WavFormat&, const ssrc::ContainerFormat&,
				      const std::vector<std::shared_ptr<StageOutlet<double>>> &, uint64_t);
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
  return make_shared<TriangularDoubleRNG>(peak, make_shared<shibatch::LCG64>(seed));
}
