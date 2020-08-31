#include "audio/filter/1-pole_stereo.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {

Filter_1Pole_Stereo::Filter_1Pole_Stereo()
	: sr_(44100)
	, freq_(1.f)
{
}

ml::DSPVectorArray<2> Filter_1Pole_Stereo::lp() const
{
	return ml::append(filters_[0].lp(), filters_[1].lp());
}

ml::DSPVectorArray<2> Filter_1Pole_Stereo::hp() const
{
	return ml::append(filters_[0].hp(), filters_[1].hp());
}

void Filter_1Pole_Stereo::operator()(const ml::DSPVectorArray<2>& in)
{
	filters_[0](in.constRow(0));
	filters_[1](in.constRow(1));
}

void Filter_1Pole_Stereo::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

void Filter_1Pole_Stereo::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_1Pole_Stereo::recalculate()
{
	auto g = Filter_1Pole::calculate_g(sr_, freq_);

	filters_[0].set_g(g);
	filters_[1].set_g(g);
}

}}}