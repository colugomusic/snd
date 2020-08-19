#pragma once

#include "ring_modulator.h"

namespace snd {
namespace audio {
namespace ringmod {

class RingModulator_Mono
{
	float sr_ = 44100.0f;
	float freq_ = 100.0f;
	float amount_ = 0.0f;

	RingModulator ringmod_;

public:

	float operator()(float in);
	void reset(float phase);
	void set_sr(float sr, bool recalculate = true);
	void set_freq(float freq, bool recalculate = true);
	void set_amount(float amount);
	void recalculate();
};

}}}