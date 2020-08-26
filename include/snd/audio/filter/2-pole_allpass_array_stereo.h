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

	Filter_2Pole_Allpass::BQAP data_L_;
	Filter_2Pole_Allpass::BQAP data_R_;
	Filter_2Pole_AllpassArray<Size> filters_[2];

public:

	Filter_2Pole_AllpassArray_Stereo();
	Filter_2Pole_AllpassArray_Stereo& operator=(const Filter_2Pole_AllpassArray_Stereo& rhs);

	float process_L(float in);
	float process_R(float in);
	void set_freq(float freq, bool recalculate = true);
	void set_freq_L(float freq);
	void set_freq_R(float freq);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
	void reset();
};

template <int Size>
Filter_2Pole_AllpassArray_Stereo<Size>::Filter_2Pole_AllpassArray_Stereo()
{
	filters_[0].set_external_data(&data_L_);
	filters_[1].set_external_data(&data_R_);
}

template <int Size>
Filter_2Pole_AllpassArray_Stereo<Size>& Filter_2Pole_AllpassArray_Stereo<Size>::operator=(const Filter_2Pole_AllpassArray_Stereo& rhs)
{
	sr_ = rhs.sr_;
	freq_L_ = rhs.freq_L_;
	freq_R_ = rhs.freq_R_;
	res_ = rhs.res_;

	data_L_ = rhs.data_L_;
	data_R_ = rhs.data_R_;

	return *this;
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::reset()
{
	data_L_ = Filter_2Pole_Allpass::BQAP();
	data_R_ = Filter_2Pole_Allpass::BQAP();
}

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

	Filter_2Pole_Allpass::calculate(sr_, freq_L_, res_, &data_L_);
}

template <int Size>
void Filter_2Pole_AllpassArray_Stereo<Size>::set_freq_R(float freq)
{
	freq_R_ = freq;

	Filter_2Pole_Allpass::calculate(sr_, freq_R_, res_, &data_R_);
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
	Filter_2Pole_Allpass::calculate(sr_, freq_L_, res_, &data_L_);

	if (std::abs(freq_L_ - freq_R_) > 0.0001f)
	{
		Filter_2Pole_Allpass::calculate(sr_, freq_R_, res_, &data_R_);
	}
	else
	{
		data_R_ = data_L_;
	}
}

}}}