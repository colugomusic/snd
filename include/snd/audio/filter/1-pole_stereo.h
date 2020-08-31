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

	ml::DSPVectorArray<2> lp() const;
	ml::DSPVectorArray<2> hp() const;

	void operator()(const ml::DSPVectorArray<2>& in);

	void set_freq(float freq, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}