#include <algorithm>
#include "audio/filter/2-pole.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace filter {

Filter_2Pole::Filter_2Pole()
	: w_div_2_(0.f)
	, d_(0.f)
	, e_(0.f)
	, zdfbk_val_0_(0.f)
	, zdfbk_val_1_(0.f)
	, zdfbk_val_2_(0.f)
	, lp_(0.f)
	, bp_(0.f)
	, hp_(0.f)
{
}

void Filter_2Pole::process_frame(float in)
{
	auto a = in - zdfbk_val_0_;

	hp_ = a / e_;

	auto b = hp_ * w_div_2_;

	bp_ = b + zdfbk_val_2_;

	auto c = bp_ * w_div_2_;

	lp_ = c + zdfbk_val_1_;

	zdfbk_val_0_ = zdfbk_val_1_ + (zdfbk_val_2_ * d_);
	zdfbk_val_1_ = c + lp_;
	zdfbk_val_2_ = b + bp_;
}

void Filter_2Pole::set(float w_div_2, float d, float e)
{
	w_div_2_ = w_div_2;
	d_ = d;
	e_ = e;
}

void Filter_2Pole::calculate(float sr, float freq, float res, float* w_div_2, float* d, float* e)
{
	*w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);

	*d = res + *w_div_2;
	*e = (*d * *w_div_2) + 1.f;
}

}}}