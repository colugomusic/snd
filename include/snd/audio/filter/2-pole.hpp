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
class Filter_2Pole
{
	ml::DSPVectorArray<ROWS> w_div_2_ = 0.0f;
	ml::DSPVectorArray<ROWS> d_ = 0.0f;
	ml::DSPVectorArray<ROWS> e_ = 0.0f;
	float h_[ROWS];
	float zdfbk_val_1_[ROWS];
	float zdfbk_val_2_[ROWS];

	int SR_ = 0;
	DiffDetector<float, ROWS> diff_freq_;
	DiffDetector<float, ROWS> diff_res_;

	ml::DSPVectorArray<ROWS> lp_ = 0.0f;
	ml::DSPVectorArray<ROWS> bp_ = 0.0f;
	ml::DSPVectorArray<ROWS> hp_ = 0.0f;

	bool needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res);

public:

	Filter_2Pole();

	const ml::DSPVectorArray<ROWS>& lp() const { return lp_; }
	const ml::DSPVectorArray<ROWS>& bp() const { return bp_; }
	const ml::DSPVectorArray<ROWS>& hp() const { return hp_; }
	ml::DSPVectorArray<ROWS> peak() const { return lp_ - hp_; }

	void operator()(const ml::DSPVectorArray<ROWS>& in);
	void operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res);

	static void calculate(int sr, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res, ml::DSPVectorArray<ROWS>* w_div_2, ml::DSPVectorArray<ROWS>* d, ml::DSPVectorArray<ROWS>* e);
};

template <int ROWS>
Filter_2Pole<ROWS>::Filter_2Pole()
{
	for (int r = 0; r < ROWS; r++)
	{
		h_[r] = 0.0f;
		zdfbk_val_1_[r] = 0.0f;
		zdfbk_val_2_[r] = 0.0f;
	}
}

template <int ROWS>
bool Filter_2Pole<ROWS>::needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res)
{
	if (SR != SR_)
	{
		SR_ = SR;

		return true;
	}

	return diff_freq_(freq) || diff_res_(res);
}

template <int ROWS>
void Filter_2Pole<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in)
{
	for (int r = 0; r < ROWS; r++)
	{
		const auto& row_in = in.constRow(r);
		const auto& wdiv_2 = w_div_2_.constRow(r);
		const auto& d = d_.constRow(r);
		const auto& e = e_.constRow(r);

		auto& lp = lp_.row(r);
		auto& bp = bp_.row(r);
		auto& hp = hp_.row(r);

		for (int i = 0; i < kFloatsPerDSPVector; i++)
		{
			h_[r] = zdfbk_val_1_[r] + (zdfbk_val_2_[r] * d[i]);

			auto a = row_in[i] - h_[r];

			hp[i] = a / e[i];

			auto b = hp[i] * wdiv_2[i];

			bp[i] = b + zdfbk_val_2_[r];

			auto c = bp[i] * wdiv_2[i];

			lp[i] = c + zdfbk_val_1_[r];

			zdfbk_val_1_[r] = c + lp[i];
			zdfbk_val_2_[r] = b + bp[i];
		}
	}
}

template <int ROWS>
void Filter_2Pole<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res)
{
	if (needs_recalc(SR, freq, res)) calculate(SR, freq, res, &w_div_2_, &d_, &e_);

	this->operator()(in);
}

template <int ROWS>
void Filter_2Pole<ROWS>::calculate(int sr, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res, ml::DSPVectorArray<ROWS>* w_div_2, ml::DSPVectorArray<ROWS>* d, ml::DSPVectorArray<ROWS>* e)
{
	auto r = 2.0f * (1.0f - ml::min(ml::DSPVectorArray<ROWS>(0.96875f), res));

	*w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);

	*d = r + *w_div_2;
	*e = (*d * *w_div_2) + 1.0f;
}

}}}