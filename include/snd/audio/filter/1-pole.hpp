#pragma once

#include <snd/misc.hpp>
#include <snd/diff_detector.hpp>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace filter {

template <int ROWS>
class Filter_1Pole
{
	ml::DSPVectorArray<ROWS> g_ = 0.0f;
	float zdfbk_val_[ROWS];
	ml::DSPVectorArray<ROWS> lp_ = 0.0f;
	ml::DSPVectorArray<ROWS> hp_ = 0.0f;
	int SR_ = 0;
	bool is_cleared_ { true };
	DiffDetector<float, ROWS> diff_freq_;

	bool needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq);

public:

	Filter_1Pole();

	const ml::DSPVectorArray<2>& lp() const { return lp_; }
	const ml::DSPVectorArray<2>& hp() const { return hp_; }

	void operator()(const ml::DSPVectorArray<ROWS>& in);
	void operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq);

	static ml::DSPVectorArray<ROWS> calculate_g(int sr, const ml::DSPVectorArray<ROWS>& freq);

	void clear()
	{
		if (is_cleared_) return;

		g_ = 0.0f;
		lp_ = 0.0f;
		hp_ = 0.0f;

		for (int row = 0; row < ROWS; row++)
		{
			zdfbk_val_[row] = 0.0f;
		}

		diff_freq_.clear();

		is_cleared_ = true;
	}
};

template <int ROWS>
Filter_1Pole<ROWS>::Filter_1Pole()
{
	for (int r = 0; r < ROWS; r++) zdfbk_val_[r] = 0.0f;
}

template <int ROWS>
bool Filter_1Pole<ROWS>::needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq)
{
	if (SR != SR_)
	{
		SR_ = SR;

		return true;
	}

	return diff_freq_(freq);
}

template <int ROWS>
void Filter_1Pole<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in)
{
	for (int r = 0; r < ROWS; r++)
	{
		const auto& row_in = in.constRow(r);
		const auto& g = g_.constRow(r);

		auto& lp = lp_.row(r);
		auto& hp = hp_.row(r);

		for (int i = 0; i < kFloatsPerDSPVector; i++)
		{
			auto a = row_in[i] - zdfbk_val_[r];
			auto b = a * g[i];

			lp[i] = b + zdfbk_val_[r];
			hp[i] = row_in[i] - lp[i];

			zdfbk_val_[r] = b + lp[i];
		}
	}

	is_cleared_ = false;
}

template <int ROWS>
void Filter_1Pole<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq)
{
	if (needs_recalc(SR, freq)) g_ = calculate_g(SR, freq);

	this->operator()(in);
}

template <int ROWS>
ml::DSPVectorArray<ROWS> Filter_1Pole<ROWS>::calculate_g(int sr, const ml::DSPVectorArray<ROWS>& freq)
{
	auto w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);

	return w_div_2 / (w_div_2 + 1.f);
}

}}}