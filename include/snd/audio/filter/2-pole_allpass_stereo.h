#pragma once

#include "2-pole_allpass.h"

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Allpass_Stereo
{
	int sr_ = 44100;
	float freq_L_ = 400.0f;
	float freq_R_ = 400.0f;
	float res_ = 0.0f;

	Filter_2Pole_Allpass filters_[2];

public:

	ml::DSPVectorArray<2> operator()(ml::DSPVectorArray<2> in);
	void set_freq(float freq, bool recalculate = true);
	void set_freq_L(float freq);
	void set_freq_R(float freq);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}