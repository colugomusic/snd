#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace filter {

class Filter_1Pole
{
	float g_ = 0.0f;
	float zdfbk_val_ = 0.0f;
	ml::DSPVector lp_ = 0.0f;
	ml::DSPVector hp_ = 0.0f;

public:

	const ml::DSPVector& lp() const { return lp_; }
	const ml::DSPVector& hp() const { return hp_; }

	void operator()(const ml::DSPVector& in);

	void set_g(float g);

	static float calculate_g(int sr, float freq);
};

}}}