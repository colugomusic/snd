#include "audio/filter/2-pole_allpass_stereo.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {

float Filter_2Pole_Allpass_Stereo::process_L(float in)
{
	return filters_[0](in);
}

float Filter_2Pole_Allpass_Stereo::process_R(float in)
{
	return filters_[1](in);
}

void Filter_2Pole_Allpass_Stereo::set_freq(float freq, bool recalc)
{
	freq_L_ = freq;
	freq_R_ = freq;

	if (recalc)	recalculate();
}

void Filter_2Pole_Allpass_Stereo::set_freq_L(float freq)
{
	freq_L_ = freq;

	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_L_, res_, &bqap);

	filters_[0].set(bqap);
}

void Filter_2Pole_Allpass_Stereo::set_freq_R(float freq)
{
	freq_R_ = freq;

	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_R_, res_, &bqap);

	filters_[1].set(bqap);
}

void Filter_2Pole_Allpass_Stereo::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc)	recalculate();
}

void Filter_2Pole_Allpass_Stereo::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_2Pole_Allpass_Stereo::recalculate()
{
	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_L_, res_, &bqap);

	filters_[0].set(bqap);

	if (std::abs(freq_L_ - freq_R_) > 0.0001f)
	{
		Filter_2Pole_Allpass::calculate(sr_, freq_R_, res_, &bqap);
	}

	filters_[1].set(bqap);
}

}}}