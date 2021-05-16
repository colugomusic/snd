#pragma once

#include "../../diff_detector.h"
#include "../clipping.h"

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace ringmod {

template <int ROWS>
struct Phase
{
	float v[ROWS];
	ml::DSPVectorArray<ROWS> inc;

	Phase()
	{
		reset(0.0f);
	}

	ml::DSPVectorArray<ROWS> operator()()
	{
		ml::DSPVectorArray<ROWS> out;

		for (int r = 0; r < ROWS; r++)
		{
			for (int i = 0; i < kFloatsPerDSPVector; i++)
			{
				out.row(r)[i] = v[r];

				v[r] += inc.row(r)[i];
			}
		}

		return out;
	}

	void reset(float phase)
	{
		for (int r = 0; r < ROWS; r++) v[r] = phase;
	}
};

template <int ROWS>
class RingModulator
{
	Phase<ROWS> phase_;
	int SR_ = 0;
	DiffDetector<float, ROWS> diff_freq_;

	bool needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq);

public:

	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in, const ml::DSPVectorArray<ROWS>& amount);
	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& amount);

	void reset(float phase);

	static ml::DSPVectorArray<ROWS> calculate_inc(int sr, const ml::DSPVectorArray<ROWS>& freq);
};

template <int ROWS>
bool RingModulator<ROWS>::needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq)
{
	if (SR_ != SR)
	{
		SR_ = SR;
		return true;
	}

	return diff_freq_(freq);
}

template <int ROWS>
ml::DSPVectorArray<ROWS> RingModulator<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& dry, const ml::DSPVectorArray<ROWS>& amount)
{
	auto wet = ((ml::sin(phase_()) + 1.0f) * 0.5f) * dry;

	return ml::lerp(dry, wet, amount);
}

template <int ROWS>
ml::DSPVectorArray<ROWS> RingModulator<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& amount)
{
	if (needs_recalc(SR, freq)) phase_.inc = calculate_inc(SR, freq);

	return this->operator()(in, amount);
}

template <int ROWS>
ml::DSPVectorArray<ROWS> RingModulator<ROWS>::calculate_inc(int sr, const ml::DSPVectorArray<ROWS>& freq)
{
	return clipping::hard_clip((1.0f / sr) * freq, 0.5f);
}

template <int ROWS>
void RingModulator<ROWS>::reset(float phase)
{
	phase_.reset(phase);
}

}}}