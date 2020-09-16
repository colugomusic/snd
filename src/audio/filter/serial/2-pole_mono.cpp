#include "audio/filter/serial/2-pole_mono.h"
#include <algorithm>

namespace snd {
namespace audio {
namespace filter {
namespace serial {

Filter_2Pole_Mono::Filter_2Pole_Mono()
	: sr_(44100)
	, freq_(1.f)
	, res_(0.f)
{
}

void Filter_2Pole_Mono::operator()(float in)
{
	filter_(in);
}

void Filter_2Pole_Mono::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

void Filter_2Pole_Mono::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc)	recalculate();
}

void Filter_2Pole_Mono::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void Filter_2Pole_Mono::recalculate()
{
	float w_div_2;
	float d;
	float e;

	Filter_2Pole::calculate(sr_, freq_, res_, &w_div_2, &d, &e);

	filter_.set(w_div_2, d, e);
}

}}}}