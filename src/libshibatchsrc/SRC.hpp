#ifndef SRC_HPP
#define SRC_HPP

#include <cstdint>
#include <string>
#include <algorithm>

#include "Kaiser.hpp"
#include "FastPP.hpp"
#include "DFTFilter.hpp"

#include "shibatch/ssrc.hpp"

template<typename REAL> class ssrc::SSRC<REAL>::SSRCImpl {
public:
  virtual ~SSRCImpl() = default;
};

namespace shibatch {
  template<typename REAL>
  class SSRCStage : public ssrc::StageOutlet<REAL>, public ssrc::SSRC<REAL>::SSRCImpl {
    static int64_t gcd(int64_t x, int64_t y) {
      while (y != 0) { int64_t t = x % y; x = y; y = t; }
      return x;
    }

    class Oversample : public ssrc::StageOutlet<REAL> {
      std::shared_ptr<ssrc::StageOutlet<REAL>> inlet;
      const int64_t sfs, dfs, m;
      size_t remaining = 0;

      const size_t N = 65536;
      std::vector<REAL> buf;
      bool endReached = false;
    public:
      Oversample(std::shared_ptr<ssrc::StageOutlet<REAL>> in_, int64_t sfs_, int64_t dfs_) : inlet(in_), sfs(sfs_), dfs(dfs_), m(dfs_ / sfs_), buf(N) {}

      bool atEnd() { return endReached; }

      size_t read(REAL *out, size_t nSamples) {
	size_t ret = 0;

	while(nSamples > 0 && remaining > 0) {
	  *out++ = 0;
	  ret++;
	  nSamples--;
	  remaining--;
	}

	while(nSamples > 0) {
	  size_t nRead = inlet->read(buf.data(), std::min(size_t((nSamples + m - 1) / m), N));
	  if (nRead == 0) { endReached = true; break; }

	  for(size_t i=0;i < nRead-1;i++) {
	    *out++ = buf[i];
	    for(int j=0;j < m-1;j++) *out++ = 0;
	  }

	  ret += (nRead - 1) * m;
	  nSamples -= (nRead - 1) * m;

	  *out++ = buf[nRead-1];
	  ret++;
	  nSamples--;

	  for(int j=0;j < m-1;j++) {
	    if (nSamples == 0) {
	      remaining = m - 1 - j;
	      break;
	    }
	    *out++ = 0;
	    ret++;
	    nSamples--;
	  }
	}

	return ret;
      }
    };

    class Undersample : public ssrc::StageOutlet<REAL> {
      std::shared_ptr<ssrc::StageOutlet<REAL>> inlet;
      const int64_t sfs, dfs, m;
      bool endReached = false;

      const size_t N = 65536;
      std::vector<REAL> buf;

    public:
      Undersample(std::shared_ptr<ssrc::StageOutlet<REAL>> in_, int64_t sfs_, int64_t dfs_) : inlet(in_), sfs(sfs_), dfs(dfs_), m(sfs_ / dfs_), buf(N * m) {}

      bool atEnd() { return endReached; }

      size_t read(REAL *out, size_t nSamples) {
	REAL *origin = out;

	while(nSamples > 0 && !endReached) {
	  int64_t nRead = 0, toBeRead = std::min(N, nSamples) * m;

	  while(nRead < toBeRead) {
	    size_t r = inlet->read(buf.data() + nRead, toBeRead - nRead);
	    if (r == 0) {
	      endReached = true;
	      break;
	    }
	    nRead += r;
	  }

	  for(int64_t i = 0;i < nRead;i += m) {
	    *out++ = buf[i];
	    nSamples--;
	  }
	}

	return out - origin;
      }
    };

    std::shared_ptr<ssrc::StageOutlet<REAL>> inlet;
    const int64_t sfs, dfs, fslcm, lfs, hfs;

    const int64_t dftflen;
    const double aa, guard;
    double delay = 0;

    int64_t osm, fsos;

    std::shared_ptr<FastPP<REAL>> ppf;
    std::shared_ptr<DFTFilter<REAL>> dftf;
    std::shared_ptr<Oversample> oversample;
    std::shared_ptr<Undersample> undersample;

  public:
    SSRCStage(std::shared_ptr<ssrc::StageOutlet<REAL>> inlet_, int64_t sfs_, int64_t dfs_,
	      unsigned l2dftflen_ = 12, double aa_ = 96, double guard_ = 1) :
      inlet(inlet_), sfs(sfs_), dfs(dfs_), fslcm(sfs_ / gcd(sfs_, dfs_) * dfs_),
      lfs(std::min(sfs_, dfs_)), hfs(std::max(sfs_, dfs_)),
      dftflen(1LL << l2dftflen_), aa(aa_), guard(guard_) {

      if (fslcm/hfs == 1) osm = 1;
      else if (fslcm/hfs % 2 == 0) osm = 2;
      else if (fslcm/hfs % 3 == 0) osm = 3;
      else {
	std::string s = "Resampling from " + std::to_string(sfs_) + " to " + std::to_string(dfs_) + " is not supported. ";
	s += std::to_string(lfs) + " / gcd(" + std::to_string(sfs_) + ", ";
	s += std::to_string(dfs_) + ") must be divided by 2 or 3.";
	throw(std::runtime_error(s.c_str()));
      }
      fsos = hfs * osm;

      std::vector<REAL> ppfv, dftfv;

      if (dfs != sfs) {
	// sampling frequency (fslcm)    : lcm(lfs, hfs) (Hz)
	// pass-band edge frequency (fp) : (fsos + (lfs - fsos)/(1.0 + guard)) / 2 (Hz)
	//                               : guard = 0 => lfs/2     , guard = 1 => (lfs + fsos)/2, guard = inf => fsos / 2
	// transition band width (df)    : (fsos - lfs) / (1.0 + guard) (Hz)
	//                               : guard = 0 => fsos - lfs, guard = 1 => (fsos - lfs)/2, guard = inf => 0
	// gain                          : fslcm / (double)sfs

	ppfv = KaiserWindow::makeLPF<REAL>(fslcm, (fsos + (lfs - fsos)/(1.0 + guard)) / 2, (fsos - lfs) / (1.0 + guard), aa, fslcm / (double)sfs);

	// sampling frequency (fsos)      : hfs * osm (Hz)
	// pass-band edge frequency (fp2) : (lfs / 2 - df) (Hz)
	// length                         : dftflen - 1 (dftflen must be 2^N)
	// gain                           : 1.0 / dftflen

	double df = KaiserWindow::transitionBandWidth(aa, hfs * osm, dftflen - 1);
	dftfv = KaiserWindow::makeLPF<REAL>(fsos, lfs / 2 - df, dftflen - 1, aa, 1.0 / dftflen);

	delay = ((ppfv.size() * 0.5 - 1) / fslcm + (dftfv.size() * 0.5 - 1) / (hfs * osm)) * dfs;
      }

      if (dfs > sfs) {
	ppf = make_shared<FastPP<REAL>>(inlet, sfs, fslcm, fsos, ppfv);
	dftf = make_shared<DFTFilter<REAL>>(ppf, dftfv);
	undersample = make_shared<Undersample>(dftf, fsos, dfs);
      } else if (dfs < sfs) {
	oversample = make_shared<Oversample>(inlet, sfs, fsos);
	dftf = make_shared<DFTFilter<REAL>>(oversample, dftfv);
	ppf = make_shared<FastPP<REAL>>(dftf, fsos, fslcm, dfs, ppfv);
      }
    }

    bool atEnd() {
      if (dfs > sfs) {
	return undersample->atEnd();
      } else if (dfs < sfs) {
	return ppf->atEnd();
      } else {
	return inlet->atEnd();
      }
    }

    size_t read(REAL *out, size_t nSamples) {
      if (dfs > sfs) {
	return undersample->read(out, nSamples);
      } else if (dfs < sfs) {
	return ppf->read(out, nSamples);
      } else {
	return inlet->read(out, nSamples);
      }
    }

    double getDelay() { return delay; }
  };
}
#endif // #ifndef SRC_HPP
