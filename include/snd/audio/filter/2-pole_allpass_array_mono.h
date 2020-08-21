#pragma once

#include "2-pole_allpass_array.h"

namespace snd {
namespace audio {
namespace filter {

template <int Size>
class Filter_2Pole_AllpassArray_Mono
{
	int sr_ = 44100;
	float freq_ = 400.0f;
	float res_ = 0.0f;

	Filter_2Pole_AllpassArray<Size> filter_;

public:

	float operator()(float in);
	void set_freq(float freq, bool recalculate = true);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

template <int Size>
float Filter_2Pole_AllpassArray_Mono<Size>::operator()(float in)
{
	return filter_(in);
}

template <int Size>
void Filter_2Pole_AllpassArray_Mono<Size>::set_freq(float freq, bool recalc)
{
	freq_ = freq;

	if (recalc) recalculate();
}

template <int Size>
void Filter_2Pole_AllpassArray_Mono<Size>::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc) recalculate();
}

template <int Size>
void Filter_2Pole_AllpassArray_Mono<Size>::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc) recalculate();
}

template <int Size>
void Filter_2Pole_AllpassArray_Mono<Size>::recalculate()
{
	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_, res_, &bqap);

	filter_.set(bqap);
}

}}}