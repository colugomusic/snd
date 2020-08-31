#pragma once

#include <algorithm>
#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace saturator {

class MoronSaturator
{
	float c_ = 1.0f;
	float limit_ = 1.0f;
	float drv_plus_1_ = 1.0f;

public:
	
	ml::DSPVector operator()(const ml::DSPVector& in) const;

	void set(float c, float limit, float drv_plus_1);

	static void calculate(float drive, float* c, float* limit, float* drv_plus_1);
};

}}}