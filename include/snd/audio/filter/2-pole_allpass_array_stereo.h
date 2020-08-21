#pragma once

#include "2-pole_allpass_array.h"

namespace snd {
namespace audio {
namespace filter {

template <int Size>
class Filter_2Pole_AllpassArray_Stereo
{
	int sr_ = 44100;
	float freq_L_ = 400.0f;
	float freq_R_ = 400.0f;
	float res_ = 0.0f;

	Filter_2Pole_AllpassArray<Size> filters_[2];

public:

	float process_L(float in);
	float process_R(float in);
	void set_freq(float freq, bool recalculate = true);
	void set_freq_L(float freq);
	void set_freq_R(float freq);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

template <int Size>
float Filter_2Pole_AllpassArray_Stereo<Size>::process_L(float in)
{
	return filters_[0](in);
}

template <int Size>
float Filter_2Pole_AllpassArray_Stereo<Size>::process_R(float in)
{
	return filters_[1](in);
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::set_freq(float freq, bool recalc)
{
	freq_L_ = freq;
	freq_R_ = freq;

	if (recalc)	recalculate();
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::set_freq_L(float freq)
{
	freq_L_ = freq;

	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_L_, res_, &bqap);

	filters_[0].set(bqap);
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::set_freq_R(float freq)
{
	freq_R_ = freq;

	Filter_2Pole_Allpass::BQAP bqap;

	Filter_2Pole_Allpass::calculate(sr_, freq_R_, res_, &bqap);

	filters_[1].set(bqap);
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc) recalculate();
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc) recalculate();
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::recalculate()
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