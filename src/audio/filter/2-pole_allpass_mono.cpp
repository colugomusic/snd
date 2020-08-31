#include "audio/filter/2-pole_allpass_mono.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {

ml::DSPVector Filter_2Pole_Allpass_Mono::operator()(const ml::DSPVector in)
{
	return filter_(in);
}

void Filter_2Pole_Allpass_Mono::set_freq(float freq, bool recalc)
{
	freq_ = freq;

	if (recalc)	recalculate();
}

void Filter_2Pole_Allpass_Mono::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc)	recalculate();
}

void Filter_2Pole_Allpass_Mono::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_2Pole_Allpass_Mono::recalculate()
{
	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_, res_, &bqap);

	filter_.set(bqap);
}

}}}