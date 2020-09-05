#pragma once

#include "2-pole.h"

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Stereo
{
	int sr_;
	float freq_;
	float res_;

	Filter_2Pole<ml::DSPVector> filters_[2];

public:

	Filter_2Pole_Stereo();

	ml::DSPVectorArray<2> lp() const;
	ml::DSPVectorArray<2> bp() const;
	ml::DSPVectorArray<2> hp() const;
	ml::DSPVectorArray<2> peak() const;

	void operator()(const ml::DSPVectorArray<2>& in);

	void set_freq(float freq, bool recalculate = true);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

}}}