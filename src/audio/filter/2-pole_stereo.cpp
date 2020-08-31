#include "audio/filter/2-pole_stereo.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {

Filter_2Pole_Stereo::Filter_2Pole_Stereo()
	: sr_(44100)
	, freq_(1.f)
	, res_(0.f)
{
}

ml::DSPVectorArray<2> Filter_2Pole_Stereo::lp() const
{
	return ml::append(filters_[0].lp(), filters_[1].lp());
}

ml::DSPVectorArray<2> Filter_2Pole_Stereo::bp() const
{
	return ml::append(filters_[0].bp(), filters_[1].bp());
}

ml::DSPVectorArray<2> Filter_2Pole_Stereo::hp() const
{
	return ml::append(filters_[0].hp(), filters_[1].hp());
}

void Filter_2Pole_Stereo::operator()(const ml::DSPVectorArray<2>& in)
{
	filters_[0](in.constRow(0));
	filters_[1](in.constRow(1));
}

void Filter_2Pole_Stereo::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

void Filter_2Pole_Stereo::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc)	recalculate();
}

void Filter_2Pole_Stereo::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_2Pole_Stereo::recalculate()
{
	float w_div_2;
	float d;
	float e;

	Filter_2Pole<ml::DSPVector>::calculate(sr_, freq_, res_, &w_div_2, &d, &e);

	filters_[0].set(w_div_2, d, e);
	filters_[1].set(w_div_2, d, e);
}

}}}