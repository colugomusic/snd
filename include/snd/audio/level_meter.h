#pragma once

#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPGens.h>
#include <DSP/MLDSPOps.h>
#include <DSP/MLDSPFilters.h>
#pragma warning(pop)

#include <snd/misc.h>

namespace snd {
namespace audio {

template <size_t ROWS>
class LevelMeter
{
	std::array<ml::LinearGlide, ROWS> glide_;

	std::array<std::atomic<float>, ROWS> current_value_;
	std::array<std::atomic<float>, ROWS> peak_;

public:

	LevelMeter(int time = kFloatsPerDSPVector * 64);

	void operator()(const ml::DSPVectorArray<ROWS>& in);
	float operator[](std::size_t index) const;
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
void LevelMeter<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in)
{
	for (int i = 0; i < ROWS; i++)
	{
		const auto low = std::abs(ml::min(in.constRow(i)));
		const auto high = std::abs(ml::max(in.constRow(i)));

		auto peak = ml::max(low, high);

		if (peak > peak_[i])
		{
			glide_[i].setValue(peak);
		}

		const auto glide = glide_[i](peak);

		auto current_value = ml::max(glide);

		peak = ml::max(glide);

		if (current_value < 0.000001f) current_value = 0.0f;
		if (peak < 0.000001f) peak = 0.0f;

		current_value_[i] = current_value;
		peak_[i] = peak;
	}
}

template <size_t ROWS>
float LevelMeter<ROWS>::operator[](std::size_t index) const
{
	return current_value_[index];
}

}}