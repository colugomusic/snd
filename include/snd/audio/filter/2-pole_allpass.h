#pragma once

#include "../../diff_detector.h"

#pragma warning(push, 0)
#include <DSP/MLDSPFilters.h>
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace filter {

template <int ROWS>
class Filter_2Pole_Allpass
{
public:

	struct BQAP
	{
		ml::DSPVectorArray<ROWS> c0;
		ml::DSPVectorArray<ROWS> c1;
		ml::DSPVectorArray<ROWS> c2;
	};

private:

	BQAP bqap_;
	const BQAP* data_;

	float fbk_val_0_[ROWS];
	float fbk_val_1_[ROWS];
	float fbk_val_2_[ROWS];
	float fbk_val_3_[ROWS];

	int SR_ = 0;
	DiffDetector<float, ROWS> diff_freq_;
	DiffDetector<float, ROWS> diff_res_;

	bool needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res);

public:

	Filter_2Pole_Allpass(const BQAP* data = nullptr);

	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in);
	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res);

	void clear();
	void set_external_data(const BQAP* data);

	static void calculate(int sr, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res, BQAP* bqap);
};

template <int ROWS>
static ml::DSPVectorArray<ROWS> clip_freq(float sr, const ml::DSPVectorArray<ROWS>& freq)
{
	const auto min = sr / 24576.0f;
	const auto max = sr / 2.125f;

	return ml::clamp(freq, ml::DSPVectorArray<ROWS>(min), ml::DSPVectorArray<ROWS>(max));
}

template <int ROWS>
static ml::DSPVectorArray<ROWS> omega(float sr, const ml::DSPVectorArray<ROWS>& freq)
{
	return freq * (6.28319f / sr);
}

template <int ROWS>
Filter_2Pole_Allpass<ROWS>::Filter_2Pole_Allpass(const BQAP* data)
	: data_(data ? data : &bqap_)
{
	clear();
}

template <int ROWS>
bool Filter_2Pole_Allpass<ROWS>::needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res)
{
	if (SR != SR_)
	{
		SR_ = SR;

		return true;
	}

	return diff_freq_(freq) || diff_res_(res);
}

template <int ROWS>
ml::DSPVectorArray<ROWS> Filter_2Pole_Allpass<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in)
{
	ml::DSPVectorArray<ROWS> out;

	for (int r = 0; r < ROWS; r++)
	{
		const auto& row_in = in.constRow(r);
		const auto& c0 = data_->c0.constRow(r);
		const auto& c1 = data_->c1.constRow(r);
		const auto& c2 = data_->c2.constRow(r);

		for (int i = 0; i < kFloatsPerDSPVector; i++)
		{
			const auto a = row_in[i] * c0[i];
			const auto b = fbk_val_0_[r] * c1[i];
			const auto c = fbk_val_1_[r] * c2[i];
			const auto d = fbk_val_2_[r] * -c1[i];
			const auto e = fbk_val_3_[r] * -c0[i];

			auto ap = a + b + c + d + e;

			flush_denormal_to_zero(ap);

			fbk_val_3_[r] = fbk_val_2_[r];
			fbk_val_2_[r] = ap;
			fbk_val_1_[r] = fbk_val_0_[r];
			fbk_val_0_[r] = row_in[i];

			out.row(r)[i] = ap;
		}
	}

	return out;
}

template <int ROWS>
ml::DSPVectorArray<ROWS> Filter_2Pole_Allpass<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res)
{
	if (!data_ && needs_recalc(SR, freq, res)) calculate(SR, freq, res, &bqap_);

	return this->operator()(in);
}

template <int ROWS>
void Filter_2Pole_Allpass<ROWS>::clear()
{
	for (int r = 0; r < ROWS; r++)
	{
		fbk_val_0_[r] = 0.0f;
		fbk_val_1_[r] = 0.0f;
		fbk_val_2_[r] = 0.0f;
		fbk_val_3_[r] = 0.0f;
	}
}

template <int ROWS>
void Filter_2Pole_Allpass<ROWS>::set_external_data(const BQAP* data)
{
	data_ = data;
}

template <int ROWS>
void Filter_2Pole_Allpass<ROWS>::calculate(int sr, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res, BQAP* bqap)
{
	const auto o = omega(float(sr), clip_freq(float(sr), freq));
	const auto sn = ml::sin(o);
	const auto cs = ml::cos(o);

	const auto alfa = sn * (1.0f - res);

	const auto b2 = 1.0f + alfa;
	const auto a1 = cs * -2.0f;
	const auto a2 = 1.0f - alfa;

	const auto a0 = 1.0f / b2;

	bqap->c0 = a2 * a0;
	bqap->c1 = a1 * a0;
	bqap->c2 = b2 * a0;
}

}}}