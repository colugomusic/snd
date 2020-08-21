#pragma once

#include "1-pole.h"

namespace snd {
namespace audio {
namespace filter {

class Filter_1Pole_Mono
{
	int sr_;
	float freq_;

	Filter_1Pole filter_;

public:

	Filter_1Pole_Mono();

	float lp() const { return filter_.lp(); }
	float hp() const { return filter_.hp(); }
	void process_frame(float in);
	void set_freq(float freq, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}