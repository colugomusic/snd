#pragma once

#include <algorithm>
#include <cmath>

namespace snd {
namespace audio {
namespace ringmod {

class RingModulator
{
	double phase_ = 0.0f;
	double inc_ = 0.0f;
	float amount_ = 0.0f;

public:

	float operator()(float in);

	void reset(double phase);
	void set_amount(float amount);
	void set_inc(double inc);

	static double calculate_inc(int sr, float freq);
};

}}}