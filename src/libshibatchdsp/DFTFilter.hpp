#ifndef DFTFILTER_HPP
#define DFTFILTER_HPP

#include <vector>
#include <cstring>
#include <cassert>

#include <sleef.h>
#include <sleefdft.h>

#include "shibatch/ssrc.hpp"

#define RESTRICT __restrict__

namespace shibatch {
  template<typename REAL>
  class DFTFilter : public ssrc::StageOutlet<REAL> {
    static constexpr const size_t toPow2(size_t n) {
      size_t ret = 1;
      for(;ret < n && ret != 0;ret *= 2) ;
      return ret;
    }

    std::shared_ptr<ssrc::StageOutlet<REAL>> in;
    const size_t firlen, dftleno2, dftlen;

    size_t nDispose;
    SleefDFT *dftf = nullptr, *dftb = nullptr;
    REAL *RESTRICT dftfilter = nullptr, *RESTRICT dftbuf = nullptr;

    std::vector<REAL> overlapbuf, fractionBuf;
    size_t fractionLen = 0, nZeroPadding = 0;
    bool endReached = false;

    template<typename T, typename std::enable_if<(std::is_same<T, double>::value), int>::type = 0>
    SleefDFT *SleefDFT_init(uint64_t mode, uint32_t n) {
      return SleefDFT_double_init1d(n, NULL, NULL, mode);
    }

    template<typename T, typename std::enable_if<(std::is_same<T, float>::value), int>::type = 0>
    SleefDFT *SleefDFT_init(uint64_t mode, uint32_t n) {
      return SleefDFT_float_init1d(n, NULL, NULL, mode);
    }

  public:
    DFTFilter(std::shared_ptr<ssrc::StageOutlet<REAL>> in_, const REAL *fircoef_, size_t firlen_) :
      in(in_), firlen(firlen_), dftleno2(toPow2(firlen_)), dftlen(dftleno2 * 2) {

      dftf = SleefDFT_init<REAL>(SLEEF_MODE_REAL | SLEEF_MODE_ALT | SLEEF_MODE_FORWARD  | SLEEF_MODE_NO_MT, dftlen);
      dftb = SleefDFT_init<REAL>(SLEEF_MODE_REAL | SLEEF_MODE_ALT | SLEEF_MODE_BACKWARD | SLEEF_MODE_NO_MT, dftlen);

      dftfilter  = (REAL *)Sleef_malloc(dftlen   * sizeof(REAL));
      dftbuf     = (REAL *)Sleef_malloc(dftlen   * sizeof(REAL));

      memset(dftfilter, 0, dftlen * sizeof(REAL));
      for(size_t z=0;z<firlen_;z++) dftfilter[z] = fircoef_[z] * (1.0 / dftleno2);

      SleefDFT_execute(dftf, dftfilter, dftfilter);

      overlapbuf.resize(dftleno2);
      fractionBuf.resize(dftlen);
    }

    DFTFilter(std::shared_ptr<ssrc::StageOutlet<REAL>> in_, const std::vector<REAL> &v) : DFTFilter(in_, v.data(), v.size()) {}

    ~DFTFilter() {
      Sleef_free(dftbuf);
      Sleef_free(dftfilter);
      SleefDFT_dispose(dftb);
      SleefDFT_dispose(dftf);
    }

    bool atEnd() { return fractionLen > 0 || !endReached; }

    size_t read(REAL *RESTRICT out, size_t nSamples) {
      size_t ret = 0;

      if (fractionLen > 0) {
	size_t nOut = std::min(fractionLen, nSamples);
	memcpy(out, fractionBuf.data(), nOut * sizeof(REAL));
	memmove(fractionBuf.data(), fractionBuf.data() + nOut, (fractionLen - nOut) * sizeof(REAL));
	fractionLen -= nOut;
	nSamples -= nOut;
	out += nOut;
	ret += nOut;
      }

      while(nSamples > 0 && (!endReached || nZeroPadding != 0)) {
	size_t nRead = 0;

	while(nRead < dftleno2) {
	  if (!endReached) {
	    size_t r = in->read(dftbuf + nRead, dftleno2 - nRead);
	    if (r == 0) {
	      endReached = true;
	      nZeroPadding = firlen;
	    }
	    nRead += r;
	  } else {
	    size_t r = std::min(dftleno2 - nRead, nZeroPadding);
	    memset(dftbuf + nRead, 0, r * sizeof(REAL));
	    nRead += r;
	    nZeroPadding -= r;
	    if (nZeroPadding == 0) break;
	  }
	}

	memset(dftbuf + nRead, 0, (dftlen - nRead) * sizeof(REAL));

	//

	SleefDFT_execute(dftf, dftbuf, dftbuf);

	dftbuf[0] = dftfilter[0] * dftbuf[0];
	dftbuf[1] = dftfilter[1] * dftbuf[1]; 

	for(unsigned i=1;i<dftleno2;i++) {
	  REAL re = dftfilter[i*2  ] * dftbuf[i*2] - dftfilter[i*2+1] * dftbuf[i*2+1];
	  REAL im = dftfilter[i*2+1] * dftbuf[i*2] + dftfilter[i*2  ] * dftbuf[i*2+1];

	  dftbuf[i*2  ] = re;
	  dftbuf[i*2+1] = im;
	}

	SleefDFT_execute(dftb, dftbuf, dftbuf);

	//

	const size_t nOut = std::min(nRead, nSamples);

	for(size_t i=0;i<nOut;i++) out[i] = dftbuf[i] + overlapbuf[i];

	if (nOut < nRead) {
	  for(size_t i=0;i<nRead - nOut;i++) fractionBuf[i] = dftbuf[nOut + i] + overlapbuf[nOut + i];
	  fractionLen = nRead - nOut;
	}

	memcpy(overlapbuf.data(), &dftbuf[dftleno2], dftleno2 * sizeof(REAL));

	out += nOut;
	nSamples -= nOut;
	ret += nOut;

	if (fractionLen > 0) break;
      }

      if (nSamples > 0) {
	size_t nOut = std::min(fractionLen, nSamples);
	memcpy(out, fractionBuf.data(), nOut * sizeof(REAL));
	memmove(fractionBuf.data(), fractionBuf.data() + nOut, (fractionLen - nOut) * sizeof(REAL));
	fractionLen -= nOut;
	nSamples -= nOut;
	out += nOut;
	ret += nOut;
      }

      return ret;
    }
  };
}
#endif // #ifndef DFTFILTER_HPP
