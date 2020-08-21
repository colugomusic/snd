#pragma once

#include "ring_modulator.h"

namespace snd {
namespace audio {
namespace ringmod {

class RingModulator_Stereo
{
	int sr_ = 44100;
	float freq_ = 100.0f;
	float amount_ = 0.0f;

	RingModulator ringmod_[2];

public:

	float process_left(float in);
	float process_right(float in);
	void reset(float phase);
	void set_sr(int sr, bool recalculate = true);
	void set_freq(float freq, bool recalculate = true);
	void set_amount(float amount);
	void recalculate();
};

}
}
}