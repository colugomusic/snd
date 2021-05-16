#pragma once

namespace snd {
namespace audio {
namespace filter {
namespace serial {

class Filter_2Pole
{
	float w_div_2_ = 0.0f;
	float d_ = 0.0f;
	float e_ = 0.0f;
	float h_ = 0.0f;
	float zdfbk_val_1_ = 0.0f;
	float zdfbk_val_2_ = 0.0f;

	float lp_ = 0.0f;
	float bp_ = 0.0f;
	float hp_ = 0.0f;

public:

	float lp() const { return lp_; }
	float bp() const { return bp_; }
	float hp() const { return hp_; }
	float peak() const { return lp_ - hp_; }

	void operator()(float in);

	void set(float w_div_2, float d, float e);

	static void calculate(int sr, float freq, float res, float* w_div_2, float* d, float* e);
};

inline void Filter_2Pole::operator()(float in)
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

inline void Filter_2Pole::set(float w_div_2, float d, float e)
{
	w_div_2_ = w_div_2;
	d_ = d;
	e_ = e;
}

inline void Filter_2Pole::calculate(int sr, float freq, float res, float* w_div_2, float* d, float* e)
{
	auto r = 2.0f * (2.0f - std::min(0.96875f, res));

	*w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);

	*d = r + *w_div_2;
	*e = (*d * *w_div_2) + 1.0f;
}

}}}}
