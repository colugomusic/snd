#pragma once

#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPGens.h>
#include <DSP/MLDSPOps.h>
#include <DSP/MLDSPFilters.h>
#pragma warning(pop)

#include <snd/misc.hpp>

namespace snd {
namespace audio {

template <size_t ROWS>
class LevelMeter
{
	std::array<ml::LinearGlide, ROWS> glide_;

	std::array<std::atomic<float>, ROWS> current_value_;
	std::array<std::atomic<float>, ROWS> peak_;

	void process_channel(int index, const ml::DSPVector& in);

public:

	LevelMeter(int time = kFloatsPerDSPVector * 64);

	void process_mono(const ml::DSPVector& in);
	void operator()(const ml::DSPVectorArray<ROWS>& in);
	float operator[](std::size_t index) const;
};

template <size_t ROWS>
LevelMeter<ROWS>::LevelMeter(int time)
{
	for (int i = 0; i < ROWS; i++)
	{
		glide_[i].setGlideTimeInSamples(float(time));
		current_value_[i] = 0.0f;
		peak_[i] = 0.0f;
	}
}

template <size_t ROWS>
void LevelMeter<ROWS>::process_channel(int index, const ml::DSPVector& in)
{
	const auto low = std::abs(ml::min(in));
	const auto high = std::abs(ml::max(in));

	auto peak = ml::max(low, high);

	if (peak > peak_[index])
	{
		glide_[index].setValue(peak);
	}

	const auto glide = glide_[index](peak);

	auto current_value = ml::max(glide);

	peak = ml::max(glide);

	if (current_value < 0.000001f) current_value = 0.0f;
	if (peak < 0.000001f) peak = 0.0f;

	current_value_[index] = current_value;
	peak_[index] = peak;
}

template <size_t ROWS>
void LevelMeter<ROWS>::process_mono(const ml::DSPVector& in)
{
	process_channel(0, in);
}

template <size_t ROWS>
void LevelMeter<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in)
{
	for (int i = 0; i < ROWS; i++)
	{
		process_channel(i, in.constRow(i));
	}
}

template <size_t ROWS>
float LevelMeter<ROWS>::operator[](std::size_t index) const
{
	return current_value_[index];
}

}}
