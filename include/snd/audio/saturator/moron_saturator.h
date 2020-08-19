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

public:
	
	float operator()(float in) const;

	void set(float c, float limit, float mix);

	static void calculate(float drive, float ceiling_db, float* c, float* limit, float* mix);
};

}}}