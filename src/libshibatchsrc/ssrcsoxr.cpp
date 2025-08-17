#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <cmath>

#include "shibatch/ssrc.hpp"
#include "shibatch/ssrcsoxr.h"
#include "ArrayQueue.hpp"

using namespace std;
using namespace ssrc;

namespace shibatch {
  template<typename OUTTYPE, typename INTYPE>
  class Soxifier : OutletProvider<INTYPE> {
    class Outlet : public StageOutlet<INTYPE> {
      Soxifier &parent;
      mutex mtx;
      condition_variable condVar;
      const uint32_t ch;
      shared_ptr<thread> th;
      ArrayQueue<INTYPE> inQueue;
      ArrayQueue<OUTTYPE> outQueue;
      bool finished = false;

      void thEntry() {
	vector<OUTTYPE> buf(parent.N);

	while(!parent.shuttingDown) {
	  size_t z = parent.tail[ch]->read(buf.data(), parent.N);
	  if (z == 0) break;

	  unique_lock lock(mtx);
	  outQueue.write(buf.data(), z);
	}

	while(parent.tail[ch]->read(buf.data(), parent.N)) ;

	unique_lock lock(mtx);
	finished = true;
	condVar.notify_all();
      }

    public:
      Outlet(Soxifier& parent_, int ch_) : parent(parent_), ch(ch_) {}

      bool atEnd() {
	unique_lock lock(mtx);
	return inQueue.size() == 0 && parent.isDraining();
      }

      size_t read(INTYPE *ptr, size_t n) {
	unique_lock lock(mtx);

	while(!(inQueue.size() != 0 || parent.isDraining())) condVar.wait(lock);

	size_t z = inQueue.read(ptr, min(n, inQueue.size()));

	if (inQueue.size() == 0) condVar.notify_all();

	return z;
      }

      friend Soxifier;
    };

    //

    enum { INIT, CLAMPED, STARTED, DRAINING, STOPPED } state = INIT;

    const unsigned nch;
    const size_t N;
    bool shuttingDown = false;

    WavFormat format;
    vector<shared_ptr<Outlet>> outlet;
    vector<shared_ptr<StageOutlet<OUTTYPE>>> tail;

    size_t collectOutput(OUTTYPE *obuf, size_t z) {
      for(unsigned ch=0;ch<nch;ch++) {
	unique_lock lock(outlet[ch]->mtx);
	z = min(z, outlet[ch]->outQueue.size());
      }

      vector<OUTTYPE> buf(z);

      for(unsigned ch=0;ch<nch;ch++) {
	{
	  unique_lock lock(outlet[ch]->mtx);
	  outlet[ch]->outQueue.read(buf.data(), z);
	}
	for(size_t i=0;i<z;i++)
	  obuf[i * nch + ch] = buf[i];
      }

      return z;
    }

    bool isDraining() const { return state == DRAINING || state == STOPPED || shuttingDown; }

  public:
    Soxifier(unsigned nch_, size_t N_ = 65536) : nch(nch_), N(N_) {
      for(unsigned ch=0;ch<nch;ch++)
	outlet.push_back(make_shared<Outlet>(*this, ch));
    }

    ~Soxifier() {
      {
	shuttingDown = true;

	for(unsigned c=0;c<nch;c++) {
	  unique_lock lock(outlet[c]->mtx);
	  outlet[c]->condVar.notify_all();
	}
      }

      for(unsigned ch=0;ch<nch;ch++) {
	if (outlet[ch]->th) outlet[ch]->th->join();
      }
    }

    shared_ptr<StageOutlet<INTYPE>> getOutlet(uint32_t channel) {
      if (channel >= outlet.size()) throw(runtime_error("Soxifier::getOutlet channel too large"));
      return outlet[channel];
    }

    void clamp(const vector<shared_ptr<StageOutlet<OUTTYPE>>> &in_) {
      if (state != INIT) throw(runtime_error("Soxifier::clamp state != INIT"));
      tail = in_;
      state = CLAMPED;
    }

    WavFormat getFormat() { return format; }

    uint32_t getNChannels() const { return format.channels; }

    //

    void start(const WavFormat &format_) {
      if (state != CLAMPED) throw(runtime_error("Soxifier::start state != CLAMPED"));
      if (format_.channels != nch) throw(runtime_error("Soxifier::start format.channels != nch"));

      format = format_;

      for(unsigned ch=0;ch<nch;ch++)
	outlet[ch]->th = make_shared<thread>(&shibatch::Soxifier<float, float>::Outlet::thEntry, outlet[ch]);

      state = STARTED;
    }

    void flow(const INTYPE *ibuf, OUTTYPE *obuf, size_t *inframe, size_t *onframe) {
      if (state != STARTED && state != DRAINING) throw(runtime_error("Soxifier::flow state != STARTED"));

      size_t ilen = *inframe, olen = *onframe;

      {
	size_t z = collectOutput(obuf, olen);
	olen -= z;
	obuf += z * nch;
      }

      for(unsigned c=0;c<nch;c++) {
	vector<INTYPE> v(ilen);
	for(size_t i=0;i<ilen;i++) v[i] = ibuf[i * nch + c];

	unique_lock lock(outlet[c]->mtx);
	outlet[c]->inQueue.write(std::move(v));
	outlet[c]->condVar.notify_all();
      }

      for(unsigned c=0;c<nch;c++) {
	unique_lock lock(outlet[c]->mtx);
	while(outlet[c]->inQueue.size() != 0) outlet[c]->condVar.wait(lock);
      }

      {
	size_t z = collectOutput(obuf, olen);
	olen -= z;
	obuf += z * nch;
      }

      *onframe = *onframe - olen;
    }

    void drain(OUTTYPE *obuf, size_t *onframe) {
      if (state != STARTED && state != DRAINING) throw(runtime_error("Soxifier::drain state != STARTED && state != DRAINING"));

      if (state != DRAINING) {
	state = DRAINING;

	for(unsigned c=0;c<nch;c++) {
	  unique_lock lock(outlet[c]->mtx);
	  outlet[c]->condVar.notify_all();
	  while(!outlet[c]->finished) outlet[c]->condVar.wait(lock);
	}
      }

      size_t z = 0;
      flow(nullptr, obuf, &z, onframe);
    }

    void stop() {
      if (state != STARTED && state != DRAINING) throw(runtime_error("Soxifier::stop state != STARTED && state != DRAINING"));

      state = STOPPED;

      for(unsigned c=0;c<nch;c++) {
	unique_lock lock(outlet[c]->mtx);
	outlet[c]->condVar.notify_all();
      }
    }
  };

  static const uint64_t MAGIC = 0x8046b5efb58216fcULL;
}

using namespace shibatch;

struct ssrc_soxr {
  uint64_t magic = MAGIC;
  ssrc_soxr_datatype_t itype, otype;

  shared_ptr<Soxifier<float, float>> f32f32;
  shared_ptr<Soxifier<double, double>> f64f64;
};

ssrc_soxr_io_spec_t ssrc_soxr_io_spec(ssrc_soxr_datatype_t itype, ssrc_soxr_datatype_t otype) {
  ssrc_soxr_io_spec_t ret;
  ret.itype = itype;
  ret.otype = otype;
  ret.ditherType = 0;
  return ret;
}

ssrc_soxr_quality_spec_t ssrc_soxr_quality_spec(unsigned long recipe, unsigned long flags) {
  ssrc_soxr_quality_spec_t ret;
  switch(recipe) {
  case SSRC_SOXR_QQ:  ret.log2dftfilterlen = 10; ret.aa =  96; ret.guard = 1; ret.dataType = SSRC_SOXR_FLOAT32; break;
  case SSRC_SOXR_LQ:  ret.log2dftfilterlen = 12; ret.aa =  96; ret.guard = 1; ret.dataType = SSRC_SOXR_FLOAT32; break;
  case SSRC_SOXR_MQ:  ret.log2dftfilterlen = 14; ret.aa = 145; ret.guard = 2; ret.dataType = SSRC_SOXR_FLOAT32; break;
  case SSRC_SOXR_HQ:  ret.log2dftfilterlen = 15; ret.aa = 145; ret.guard = 4; ret.dataType = SSRC_SOXR_FLOAT64; break;
  case SSRC_SOXR_VHQ: ret.log2dftfilterlen = 16; ret.aa = 170; ret.guard = 4; ret.dataType = SSRC_SOXR_FLOAT64; break;
  default:
    cerr << "ssrc_soxr_quality_spec : Unknown recipe" << endl;
    abort();
  }
  return ret;
}

struct ssrc_soxr *ssrc_soxr_create(double input_rate, double output_rate, unsigned num_channels,
				   ssrc_soxr_error_t *eptr, ssrc_soxr_io_spec_t const *iospec,
				   ssrc_soxr_quality_spec_t const *qspec, void const *rtspec) {
  if (rint(input_rate) != input_rate || rint(output_rate) != output_rate) {
    cerr << "ssrc_soxr_create : Unsupported sample rate" << endl;
    abort();
  }
  if (num_channels == 0) {
    cerr << "ssrc_soxr_create : Unsupported num_channels" << endl;
    abort();
  }
  if (!iospec || iospec->itype != SSRC_SOXR_FLOAT32 || iospec->otype != SSRC_SOXR_FLOAT32 || iospec->ditherType != 0) {
    cerr << "ssrc_soxr_create : Unsupported iospec" << endl;
    abort();
  }
  if (rtspec) {
    cerr << "ssrc_soxr_create : rtspec must be NULL" << endl;
    abort();
  }

  ssrc_soxr *thiz = new ssrc_soxr();

  thiz->itype = iospec->itype;
  thiz->otype = iospec->otype;

  auto xifier = make_shared<Soxifier<float, float>>(num_channels);

  thiz->f32f32 = xifier;

  vector<shared_ptr<StageOutlet<float>>> out(num_channels);

  for(unsigned i=0;i<num_channels;i++) {
    out[i] = make_shared<SSRC<float>>(xifier->getOutlet(i), input_rate, output_rate, 14, 145, 2);
  }

  xifier->clamp(out);

  xifier->start(WavFormat(WavFormat::IEEE_FLOAT, num_channels, output_rate, 32));

  return thiz;
}

ssrc_soxr_error_t ssrc_soxr_process(struct ssrc_soxr *thiz,
				    void const  *in,  size_t ilen, size_t *idone,
				    void        *out, size_t olen, size_t *odone) {
  if (thiz->magic != MAGIC) {
    cerr << "ssrc_soxr_process : thiz->magic != MAGIC" << endl;
    abort();
  }

  auto xifier = thiz->f32f32;

  if (in) {
    size_t isamp = ilen, osamp = olen;

    xifier->flow((const float *)in, (float *)out, &isamp, &osamp);

    if (idone) *idone = isamp;
    if (odone) *odone = osamp;
  } else {
    size_t osamp = olen;

    xifier->drain((float *)out, &osamp);

    if (odone) *odone = osamp;
  }

  return nullptr;
}

void ssrc_soxr_delete(struct ssrc_soxr *thiz) {
  if (thiz->magic != MAGIC) {
    cerr << "ssrc_soxr_delete : thiz->magic != MAGIC" << endl;
    abort();
  }
  thiz->magic = 0;
  delete thiz;
}

double ssrc_soxr_delay(struct ssrc_soxr *thiz) {
  if (thiz->magic != MAGIC) {
    cerr << "ssrc_soxr_delay : thiz->magic != MAGIC" << endl;
    abort();
  }
  return 0;
}
