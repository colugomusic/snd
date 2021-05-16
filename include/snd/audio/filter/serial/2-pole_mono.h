#pragma once

#include "2-pole.h"

namespace snd {
namespace audio {
namespace filter {
namespace serial {

class Filter_2Pole_Mono
{
	int sr_;
	float freq_;
	float res_;

	Filter_2Pole filter_;

public:

	Filter_2Pole_Mono();

	float lp() const { return filter_.lp(); }
	float bp() const { return filter_.bp(); }
	float hp() const { return filter_.hp(); }
	float peak() const { return filter_.peak(); }

	void operator()(float in);

	void set_freq(float freq, bool recalculate = true);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

inline Filter_2Pole_Mono::Filter_2Pole_Mono()
	: sr_(44100)
	, freq_(1.f)
	, res_(0.f)
{
}

inline void Filter_2Pole_Mono::operator()(float in)
{
	filter_(in);
}

inline void Filter_2Pole_Mono::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

inline void Filter_2Pole_Mono::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc)	recalculate();
}

inline void Filter_2Pole_Mono::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

inline void Filter_2Pole_Mono::recalculate()
{
	float w_div_2;
	float d;
	float e;

	Filter_2Pole::calculate(sr_, freq_, res_, &w_div_2, &d, &e);

	filter_.set(w_div_2, d, e);
}

}}}}
