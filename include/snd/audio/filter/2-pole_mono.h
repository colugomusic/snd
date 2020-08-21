#pragma once

#include "2-pole.h"

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Mono
{
	int sr_;
	float freq_;
	float res_;

	Filter_2Pole filter_;

public:

	Filter_2Pole_Mono();

	float lp() const { return filter_.lp(); }
	float bp() const { return filter_.hp(); }
	float hp() const { return filter_.hp(); }
	void process_frame(float in);
	void set_freq(float freq, bool recalculate = true);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}