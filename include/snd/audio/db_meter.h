#pragma once

#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#include <DSP/MLDSPFilters.h>
#pragma warning(pop)

namespace snd {
namespace audio {

template <size_t ROWS>
class LevelMeter
{
	std::array<ml::LinearGlide, ROWS> glide_;

	std::array<float, ROWS> currentValue_;
	std::array<float, ROWS> peak_;

public:

	LevelMeter(int time = kFloatsPerDSPVector * 64);

	const std::array<float, ROWS>& operator()(const ml::DSPVectorArray<ROWS>& in);
};

template <size_t ROWS>
LevelMeter<ROWS>::LevelMeter(int time)
{
	for (int i = 0; i < ROWS; i++)
	{
		glide_[i].setGlideTimeInSamples(float(time));
	}
}

template <size_t ROWS>
const std::array<float, ROWS>& LevelMeter<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in)
{
	for (int i = 0; i < ROWS; i++)
	{
		const auto low = std::abs(ml::min(in.constRow(i)));
		const auto high = std::abs(ml::max(in.constRow(i)));
		const auto peak = ml::max(low, high);

		if (peak > peak_[i])
		{
			glide_[i].setValue(peak);
		}

		const auto glide = glide_[i](peak);

		currentValue_[i] = ml::max(glide);
		peak_[i] = ml::max(glide);
	}

	return currentValue_;
}

}}