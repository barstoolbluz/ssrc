// Shibatch Sampling Rate Converter written by Naoki Shibata  https://shibatch.github.io

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <random>
#include <cstdint>
#include <cstdlib>

#include "shibatch/ssrc.hpp"
#include "shapercoefs.h"

#ifndef SSRC_VERSION
#error SSRC_VERSION not defined
#endif

using namespace std;
using namespace ssrc;

struct ConversionProfile {
  unsigned log2dftfilterlen;
  double aa, guard;
  bool doublePrecision;
};

const unordered_map<string, ConversionProfile> availableProfiles = {
  { "insane",    { 18, 200, 8.0, true } },
  { "high",      { 16, 170, 2.0, true } },
  { "long",      { 15, 140, 1.0, false} },
  { "normal",    { 14, 120, 1.0, false} },
  { "standard",  { 14, 120, 1.0, false} },
  { "default",   { 14, 120, 1.0, false} },
  { "short",     { 10,  96, 1.0, false} },
  { "fast",      { 10,  96, 1.0, false} },
  { "lightning", {  8,  80, 0.5, false} },
};

void showProfileOptions() {
  cerr << "Available profiles :" << endl;
  for(auto p : availableProfiles) {
    cerr << "Profile name : " << p.first << endl;
    cerr << "  FFT length : " << (1LL << p.second.log2dftfilterlen) << endl;
    cerr << "  Stop band attenuation : " << p.second.aa << " dB" << endl;
    cerr << "  Guard value : " << p.second.guard << endl;
    cerr << "  Floating point precision : " << (p.second.doublePrecision ? "double" : "single") << endl;
    cerr << endl;
  }
  exit(-1);
}

void showDitherOptions() {
  cerr << "Available dither options :" << endl;
  int lastfs = 0;
  for(int i=0;;i++) {
    const NoiseShaperCoef &c = noiseShaperCoef[i];
    if (c.fs < 0) break;
    if (lastfs != c.fs) {
      cerr << endl << "Sampling freq : " << c.fs << endl;
      lastfs = c.fs;
    }
    cerr << "  ID " << c.id << " : " << c.name << endl;
  }
  exit(-1);
}

void showUsage(const string& argv0, const string& mes = "") {
  cerr << ("Shibatch sampling rate converter  Version " SSRC_VERSION) << endl;
  cerr << endl;
  cerr << "usage: " << argv0 << " [<options>] <source file name> <destination file name>" << endl;
  cerr << endl;
  cerr << "options : --rate <sampling rate(Hz)> output sample rate" << endl;
  cerr << "          --att <attenuation(dB)>    attenuate output" << endl;
  cerr << "          --bits <number of bits>    output quantization bit length" << endl;
  cerr << "                                     Specify 0 to convert to an IEEE 32-bit FP wav file" << endl;
  cerr << "          --dither <type>            dither options" << endl;
  cerr << "                                       0    : Low intensity ATH-based noise shaping" << endl;
  cerr << "                                       98   : Triangular noise shaping" << endl;
  cerr << "                                       help : Show all available options" << endl;
  cerr << "          --pdf <type> [<amp>]       select probability distribution function for dithering" << endl;
  cerr << "                                       0 : Rectangular" << endl;
  cerr << "                                       1 : Triangular" << endl;
  //cerr << "                                       2 : Gaussian" << endl;
  //cerr << "                                       3 : Two-level (experimental)" << endl;
  cerr << "          --profile <type>           specify profile" << endl;
  cerr << "                                       fast : shorter filter length, quick conversion" << endl;
  cerr << "                                       help : Show all available options" << endl;
  cerr << endl;
  cerr << "If you like this tool, visit https://github.com/shibatch/ssrc and give it a star." << endl;
  cerr << endl;

  if (mes != "") cerr << "Error : " << mes << endl;

  exit(-1);
}

class RectangularRNG : public DoubleRNG {
  const double min, max;
  default_random_engine rng;
  uniform_real_distribution<double> dist{0.0, 1.0};
public:
  RectangularRNG(double min_, double max_, uint64_t seed) : min(min_), max(max_), rng(seed) {}
  double nextDouble() { return min + dist(rng) * (max - min); }
};

int main(int argc, char **argv) {
  if (argc < 2) showUsage(argv[0], "");

  string srcfn, dstfn, profileName = "default";
  int64_t rate = -1, bits = 16, dither = -1, pdf = 0;
  uint64_t seed = ~0ULL;
  double att = 0, peak = 1.0;
  bool quiet = false, debug = false;

  int nextArg;
  for(nextArg = 1;nextArg < argc;nextArg++) {
    if (string(argv[nextArg]) == "--rate") {
      if (nextArg+1 >= argc) showUsage(argv[0]);
      char *p;
      rate = strtol(argv[nextArg+1], &p, 0);
      if (p == argv[nextArg+1] || *p || rate < 0)
	showUsage(argv[0], "A non-negative integer is expected after --rate.");
      nextArg++;
    } else if (string(argv[nextArg]) == "--att") {
      if (nextArg+1 >= argc) showUsage(argv[0]);
      char *p;
      att = strtod(argv[nextArg+1], &p);
      if (p == argv[nextArg+1] || *p)
	showUsage(argv[0], "A number is expected after --att.");
      nextArg++;
    } else if (string(argv[nextArg]) == "--bits") {
      if (nextArg+1 >= argc) showUsage(argv[0]);
      char *p;
      bits = strtol(argv[nextArg+1], &p, 0);
      if (p == argv[nextArg+1] || *p || bits < 0)
	showUsage(argv[0], "A non-negative integer is expected after --bits.");
      nextArg++;
    } else if (string(argv[nextArg]) == "--dither") {
      if (nextArg == 1 && argc == 2) showDitherOptions();
      if (nextArg+1 >= argc) showUsage(argv[0]);
      if (argv[nextArg+1] == string("help")) showDitherOptions();
      char *p;
      dither = strtol(argv[nextArg+1], &p, 0);
      if (p == argv[nextArg+1] || *p || dither < 0)
	showUsage(argv[0], "A positive value is expected after --dither.");
      nextArg++;
    } else if (string(argv[nextArg]) == "--pdf") {
      if (nextArg+1 >= argc) showUsage(argv[0]);
      char *p;
      pdf = strtol(argv[nextArg+1], &p, 0);
      if (p == argv[nextArg+1] || *p || pdf < 0)
	showUsage(argv[0], "A positive value is expected after --pdf.");
      nextArg++;
      double peak_ = strtod(argv[nextArg+1], &p);
      if (*p == '\0') { peak = peak_; nextArg++; }
    } else if (string(argv[nextArg]) == "--profile") {
      if (nextArg == 1 && argc == 2) showProfileOptions();
      if (nextArg+1 >= argc) showUsage(argv[0], "Specify a profile name after --profile");
      if (argv[nextArg+1] == string("help")) showProfileOptions();
      profileName = argv[nextArg+1];
      nextArg++;
    } else if (string(argv[nextArg]) == "--quiet") {
      quiet = true;
    } else if (string(argv[nextArg]) == "--debug") {
      debug = true;
    } else if (string(argv[nextArg]) == "--seed") {
      if (nextArg+1 >= argc) showUsage(argv[0]);
      char *p;
      seed = strtoull(argv[nextArg+1], &p, 0);
      if (p == argv[nextArg+1] || *p)
	showUsage(argv[0], "A positive integer is expected after --seed.");
      nextArg++;
    } else if (string(argv[nextArg]) == "--tmpfile") {
      showUsage(argv[0], "--tmpfile option is no longer available.");
    } else if (string(argv[nextArg]) == "--twopass") {
      showUsage(argv[0], "--twopass option is no longer available.");
    } else if (string(argv[nextArg]) == "--normalize") {
      showUsage(argv[0], "--normalize option is no longer available.");
    } else if (string(argv[nextArg]).substr(0, 2) == "--") {
      showUsage(argv[0], string("Unrecognized option : ") + argv[nextArg]);
    } else {
      break;
    }
  }

  if (nextArg < argc) srcfn = argv[nextArg++]; else showUsage(argv[0], "Specify a source file name.");
  if (nextArg < argc) dstfn = argv[nextArg++]; else showUsage(argv[0], "Specify a destination file name.");

  if (nextArg != argc) showUsage(argv[0], "Extra arguments after the destination file name.");

  if (pdf > 1)
    showUsage(argv[0], "PDF ID " + to_string(pdf) + " is not supported");

  if (availableProfiles.count(profileName) == 0)
    showUsage(argv[0], "There is no profile of name \"" + profileName + "\"");

  if (bits != 0 && bits != 8 && bits != 16 && bits != 24 && bits != 32)
    showUsage(argv[0], to_string(bits) + "-bit quantization is not supported");

  //

  if (seed == ~0ULL) seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const ConversionProfile &profile = availableProfiles.at(profileName);

  auto reader = make_shared<WavReader<float>>(srcfn);

  const auto srcFormat = reader->getFormat();
  const int sfs = srcFormat.sampleRate;
  const int dfs = rate < 0 ? srcFormat.sampleRate : rate;
  const int nch = srcFormat.channels;

  int shaperid = -1;

  for(int i=0;;i++) {
    const NoiseShaperCoef &c = noiseShaperCoef[i];
    if (c.fs < 0) break;
    if (c.fs == dfs && c.id == dither) {
      shaperid = i;
      break;
    }
  }

  if (dither != -1 && shaperid == -1)
    showUsage(argv[0], "Dither type " + to_string(dither) + " is not available for destination sampling frequency " + to_string(dfs) + "Hz");

  //

  if (debug) {
    cerr << "srcfn = "        << srcfn << endl;
    cerr << "dstfn = "        << dstfn << endl;
    cerr << "nch = "          << nch << endl;
    cerr << "sfs = "          << sfs << endl;
    cerr << "dfs = "          << dfs << endl;
    cerr << "bits = "         << bits << endl;
    cerr << endl;

    cerr << "dither = "       << dither << endl;
    cerr << "shaperid = "     << shaperid << endl;
    cerr << "pdf = "          << pdf << endl;
    cerr << "peak = "         << peak << endl;
    cerr << endl;

    cerr << "profileName = "  << profileName << endl;
    cerr << "dftfilterlen = " << (1LL << profile.log2dftfilterlen) << endl;
    cerr << "doublePrec = "   << profile.doublePrecision << endl;
    cerr << "aa = "           << profile.aa << endl;
    cerr << "guard = "        << profile.guard << endl;
    cerr << endl;

    cerr << "att = "          << att << endl;
    cerr << "quiet = "        << quiet << endl;
    cerr << "seed = "         << seed << endl;
  }

  //

  const WavFormat dstFormat = bits == 0 ?
    WavFormat(WavFormat::IEEE_FLOAT, srcFormat.channels, dfs, 32  ) :
    WavFormat(WavFormat::PCM       , srcFormat.channels, dfs, bits);

  try {
    if (!profile.doublePrecision) {
      if (shaperid == -1 || bits == 0) {
	vector<shared_ptr<ssrc::StageOutlet<float>>> out(nch);

	for(int i=0;i<nch;i++) {
	  auto ssrc = make_shared<SSRC<float>>(reader->getOutlet(i), sfs, dfs,
					       profile.log2dftfilterlen, profile.aa, profile.guard);
	  out[i] = ssrc;
	}

	auto writer = make_shared<WavWriter<float>>(dstfn, dstFormat, out);
	writer->execute();
      } else {
	const double gain = (1LL << (bits - 1)) - 1;
	const int32_t clipMin = bits != 8 ? -(1LL << (bits - 1)) + 0 : 0x00;
	const int32_t clipMax = bits != 8 ? +(1LL << (bits - 1)) - 1 : 0xff;
	const int32_t offset  = bits != 8 ? 0 : 0x80;

	vector<shared_ptr<ssrc::StageOutlet<int32_t>>> out(nch);

	for(int i=0;i<nch;i++) {
	  std::shared_ptr<DoubleRNG> rng;
	  if (pdf == 0) {
	    rng = createTriangleRNG(peak, seed + i);
	  } else {
	    rng = make_shared<RectangularRNG>(-peak, peak, seed + i);
	  }

	  auto ssrc = make_shared<SSRC<float>>(reader->getOutlet(i), sfs, dfs,
					       profile.log2dftfilterlen, profile.aa, profile.guard);

	  auto dither = make_shared<Dither<int32_t, float>>(ssrc, gain, offset, clipMin, clipMax, &ssrc::noiseShaperCoef[shaperid], rng);
	  out[i] = dither;
	}

	auto writer = make_shared<WavWriter<int32_t>>(dstfn, dstFormat, out);

	writer->execute();
      }
    } else {
      // Create a double precision pipe

      if (shaperid == -1 || bits == 0) {
	vector<shared_ptr<ssrc::StageOutlet<double>>> out(nch);

	for(int i=0;i<nch;i++) {
	  auto cast = make_shared<CastStage<double, float>>(reader->getOutlet(i));
	  auto ssrc = make_shared<SSRC<double>>(cast, sfs, dfs,
						profile.log2dftfilterlen, profile.aa, profile.guard);
	  out[i] = ssrc;
	}

	auto writer = make_shared<WavWriter<double>>(dstfn, dstFormat, out);
	writer->execute();
      } else {
	const double gain = (1LL << (bits - 1)) - 1;
	const int32_t clipMin = bits != 8 ? -(1LL << (bits - 1)) + 0 : 0x00;
	const int32_t clipMax = bits != 8 ? +(1LL << (bits - 1)) - 1 : 0xff;
	const int32_t offset  = bits != 8 ? 0 : 0x80;

	vector<shared_ptr<ssrc::StageOutlet<int32_t>>> out(nch);

	for(int i=0;i<nch;i++) {
	  std::shared_ptr<DoubleRNG> rng;
	  if (pdf == 0) {
	    rng = createTriangleRNG(peak, seed + i);
	  } else {
	    rng = make_shared<RectangularRNG>(-peak, peak, seed + i);
	  }

	  auto cast = make_shared<CastStage<double, float>>(reader->getOutlet(i));
	  auto ssrc = make_shared<SSRC<double>>(cast, sfs, dfs,
						profile.log2dftfilterlen, profile.aa, profile.guard);

	  auto dither = make_shared<Dither<int32_t, double>>(ssrc, gain, offset, clipMin, clipMax, &ssrc::noiseShaperCoef[shaperid], rng);
	  out[i] = dither;
	}

	auto writer = make_shared<WavWriter<int32_t>>(dstfn, dstFormat, out);

	writer->execute();
      }
    }
  } catch(exception &ex) {
    cerr << "Error : " << ex.what() << endl;
    return -1;
  }

  return 0;
}
