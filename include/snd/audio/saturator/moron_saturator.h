#pragma once

#include <algorithm>
#include <cmath>

namespace snd {
namespace audio {
namespace saturator {

class MoronSaturator
{
	float c_ = 1.0f;
	float limit_ = 1.0f;
	float mix_ = 0.0f;
	float drv_plus_1_ = 1.0f;

public:
	
	float operator()(float in) const;

	void set(float c, float limit, float mix, float drv_plus_1);

	static void calculate(float drive, float* c, float* limit, float* mix, float* drv_plus_1);
};

}}}