#pragma once

#include "1-pole.h"

namespace snd {
namespace audio {
namespace filter {

class Filter_1Pole_Stereo
{
	int sr_;
	float freq_;

	Filter_1Pole filters_[2];

public:

	Filter_1Pole_Stereo();

	float lp_L() const { return filters_[0].lp(); }
	float lp_R() const { return filters_[1].lp(); }
	float hp_L() const { return filters_[0].hp(); }
	float hp_R() const { return filters_[1].hp(); }

	void process_left(float in);
	void process_right(float in);
	void set_freq(float freq, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}