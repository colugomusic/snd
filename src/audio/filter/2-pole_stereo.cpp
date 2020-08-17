#include "audio/filter/2-pole_stereo.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {

Filter_2Pole_Stereo::Filter_2Pole_Stereo()
	: sr_(44100.f)
	, freq_(1.f)
	, res_(0.f)
{
}

void Filter_2Pole_Stereo::process_left(float in)
{
	filters_[0].process_frame(in);
}

void Filter_2Pole_Stereo::process_right(float in)
{
	filters_[1].process_frame(in);
}

void Filter_2Pole_Stereo::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

void Filter_2Pole_Stereo::set_res(float res, bool recalc)
{
	res_ = 2.f * (1.f - std::min(0.96875f, res));

	if (recalc)	recalculate();
}

void Filter_2Pole_Stereo::set_sr(float sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_2Pole_Stereo::recalculate()
{
	float w_div_2;
	float d;
	float e;

	Filter_2Pole::calculate(sr_, freq_, res_, &w_div_2, &d, &e);

	filters_[0].set(w_div_2, d, e);
	filters_[1].set(w_div_2, d, e);
}

}}}