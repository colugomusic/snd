#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace ringmod {

struct Phase
{
	double v = 0.0f;
	double inc;

	void set_inc(double i)
	{
		inc = i;
	}

	ml::DSPVector operator()()
	{
		ml::DSPVector out;

		for (int i = 0; i < kFloatsPerDSPVector; i++)
		{
			out[i] = float(v);

			v += inc;
		}

		return out;
	}

	void reset(float phase)
	{
		v = phase;
	}
};

class RingModulator
{
	Phase phase_;
	float amount_ = 0.0f;

public:

	ml::DSPVector operator()(const ml::DSPVector& in);

	void reset(float phase);
	void set_amount(float amount);
	void set_inc(double inc);

	static double calculate_inc(int sr, float freq);
};

}}}