#include "audio/filter/1-pole.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace filter {

void Filter_1Pole::operator()(const ml::DSPVector& in)
{
	for (int i = 0; i < kFloatsPerDSPVector; i++)
	{
		auto a = in[i] - zdfbk_val_;
		auto b = a * g_;

		lp_[i] = b + zdfbk_val_;
		hp_[i] = in[i] - lp_[i];

		zdfbk_val_ = b + lp_[i];
	}
}

void Filter_1Pole::set_g(float g)
{
	g_ = g;
}

float Filter_1Pole::calculate_g(int sr, float freq)
{
	auto w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(float(sr), freq);

	return w_div_2 / (w_div_2 + 1.f);
}

}}}