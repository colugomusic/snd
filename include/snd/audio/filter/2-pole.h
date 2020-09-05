#pragma once

#include "../../misc.h"

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace filter {

template <class T>
class Filter_2Pole
{
	float w_div_2_ = 0.0f;
	float d_ = 0.0f;
	float e_ = 0.0f;
	float h_ = 0.0f;
	float zdfbk_val_1_ = 0.0f;
	float zdfbk_val_2_ = 0.0f;

	T lp_ = 0.0f;
	T bp_ = 0.0f;
	T hp_ = 0.0f;

public:

	const T& lp() const { return lp_; }
	const T& bp() const { return bp_; }
	const T& hp() const { return hp_; }
	T peak() const { return lp_ - hp_; }

	void operator()(const T& in) {}

	void set(float w_div_2, float d, float e);

	static void calculate(int sr, float freq, float res, float* w_div_2, float* d, float* e);
};

template <>
void Filter_2Pole<float>::operator()(const float& in)
{
	h_ = zdfbk_val_1_ + (zdfbk_val_2_ * d_);

	auto a = in - h_;

	hp_ = a / e_;

	auto b = hp_ * w_div_2_;

	bp_ = b + zdfbk_val_2_;

	auto c = bp_ * w_div_2_;

	lp_ = c + zdfbk_val_1_;

	zdfbk_val_1_ = c + lp_;
	zdfbk_val_2_ = b + bp_;
}

template <>
void Filter_2Pole<ml::DSPVector>::operator()(const ml::DSPVector& in)
{
	for (int i = 0; i < kFloatsPerDSPVector; i++)
	{
		h_ = zdfbk_val_1_ + (zdfbk_val_2_ * d_);

		auto a = in[i] - h_;

		hp_[i] = a / e_;

		auto b = hp_[i] * w_div_2_;

		bp_[i] = b + zdfbk_val_2_;

		auto c = bp_[i] * w_div_2_;

		lp_[i] = c + zdfbk_val_1_;

		zdfbk_val_1_ = c + lp_[i];
		zdfbk_val_2_ = b + bp_[i];
	}
}

template <class T>
void Filter_2Pole<T>::set(float w_div_2, float d, float e)
{
	w_div_2_ = w_div_2;
	d_ = d;
	e_ = e;
}

template <class T>
void Filter_2Pole<T>::calculate(int sr, float freq, float res, float* w_div_2, float* d, float* e)
{
	res = 2.f * (1.f - std::min(0.96875f, res));

	*w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(float(sr), freq);

	*d = res + *w_div_2;
	*e = (*d * *w_div_2) + 1.f;
}

}}}