#include "audio/filter/1-pole.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace filter {

Filter_1Pole::Filter_1Pole()
	: g_(0.f)
	, zdfbk_val_(0.f)
	, lp_(0.f)
	, hp_(0.f)
{
}

void Filter_1Pole::process_frame(float in)
{
	auto a = in - zdfbk_val_;
	auto b = a * g_;
	
	lp_ = b + zdfbk_val_;
	hp_ = in - lp_;

	zdfbk_val_ = b + lp_;
}

void Filter_1Pole::set_g(float g)
{
	g_ = g;
}

float Filter_1Pole::calculate_g(float sr, float freq)
{
	auto w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);

	return w_div_2 / (w_div_2 + 1.f);
}

}}}