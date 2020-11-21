#include "audio/filter/serial/2-pole.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace filter {
namespace serial {

void Filter_2Pole::operator()(float in)
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

void Filter_2Pole::set(float w_div_2, float d, float e)
{
	w_div_2_ = w_div_2;
	d_ = d;
	e_ = e;
}

void Filter_2Pole::calculate(int sr, float freq, float res, float* w_div_2, float* d, float* e)
{
	auto r = 2.0f * (2.0f - std::min(0.96875f, res));

	*w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);

	*d = r + *w_div_2;
	*e = (*d * *w_div_2) + 1.0f;
}

}}}}