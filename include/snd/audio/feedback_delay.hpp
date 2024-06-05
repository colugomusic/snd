#pragma once

#include <array>
#include <functional>

#pragma warning(push, 0)
#include <DSP/MLDSPFilters.h>
#pragma warning(pop)

#include "delay.hpp"

namespace snd {
namespace audio {

template <size_t ROWS>
class FeedbackDelay {
public: 
	using InsertEffect = std::function<ml::DSPVectorArray<ROWS>(const ml::DSPVectorArray<ROWS>&)>;

	FeedbackDelay() = default;

	auto resize(float size) -> void {
		size_ = size;
		for (int row = 0; row < ROWS; row++) {
			delays_[row].setMaxDelayInSamples(size);
		}
	}

	void clear()
	{
		if (is_empty_) return;

		for (int row = 0; row < ROWS; row++)
		{
			delays_[row].clear();
		}

		feedback_ = { 0.0f };
	}

	ml::DSPVectorArray<ROWS> operator()(
		const ml::DSPVectorArray<ROWS>& dry,
		const ml::DSPVectorArray<ROWS>& time,
		const ml::DSPVectorArray<ROWS>& feedback_amount,
		InsertEffect insert_effect = [](const ml::DSPVectorArray<ROWS>& x) { return x; })
	{
		const auto delay_in { dry + (feedback_ * feedback_amount) };

		ml::DSPVectorArray<ROWS> delay_out;

		for (int row = 0; row < ROWS; row++)
		{
			delay_out.row(row) = delays_[row](delay_in.constRow(row), ml::clamp(time.constRow(row) - kFloatsPerDSPVector, {1.0f}, { size_ }));
		} 

		feedback_ = insert_effect(delay_out);
		is_empty_ = false;

		return delay_out;
	}

private:

	std::array<ml::PitchbendableDelay, ROWS> delays_;

	float size_;
	ml::DSPVectorArray<ROWS> feedback_;
	bool is_empty_ { true };
};

} // audio
} // snd