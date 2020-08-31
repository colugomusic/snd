#pragma once

#include "2-pole_allpass.h"

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Allpass_Mono
{
	int sr_ = 44100;
	float freq_ = 400.0f;
	float res_ = 0.0f;

	Filter_2Pole_Allpass filter_;

public:

	ml::DSPVector operator()(const ml::DSPVector in);
	void set_freq(float freq, bool recalculate = true);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}