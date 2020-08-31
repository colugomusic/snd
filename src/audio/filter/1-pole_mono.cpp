#include "audio/filter/1-pole_mono.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {

Filter_1Pole_Mono::Filter_1Pole_Mono()
	: sr_(44100)
	, freq_(1.f)
{
}

void Filter_1Pole_Mono::operator()(const ml::DSPVector& in)
{
	filter_(in);
}

void Filter_1Pole_Mono::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

void Filter_1Pole_Mono::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_1Pole_Mono::recalculate()
{
	filter_.set_g(Filter_1Pole::calculate_g(sr_, freq_));
}

}}}