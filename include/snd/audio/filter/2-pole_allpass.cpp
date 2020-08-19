#include "2-pole_allpass.h"
#include <algorithm>
#include <cmath>

namespace snd {
namespace audio {
namespace filter {

static float clip_freq(float sr, float freq)
{
	return std::min(sr / 2.125f, std::max(sr / 24576.0f, freq));
}

static float omega(float sr, float freq)
{
	return freq * (6.28319f / sr);
}

void Filter_2Pole_Allpass::process_frame(float in)
{
	const auto a = in * (bqap_.b0 * bqap_.a0);
	const auto b = zdfbk_val_0_ * (bqap_.b1 * bqap_.a0);
	const auto c = zdfbk_val_1_ * (bqap_.b2 * bqap_.a0);
	const auto d = zdfbk_val_2_ * -(bqap_.a1 * bqap_.a0);
	const auto e = zdfbk_val_3_ * -(bqap_.a2 * bqap_.a0);

	ap_ = a + b + c + d + e;

	zdfbk_val_3_ = zdfbk_val_2_;
	zdfbk_val_2_ = ap_;
	zdfbk_val_1_ = zdfbk_val_0_;
	zdfbk_val_0_ = in;
}

void Filter_2Pole_Allpass::set(const BQAP& bqap)
{
	bqap_ = bqap;
}

void Filter_2Pole_Allpass::calculate(float sr, float freq, float res, BQAP* bqap)
{
	const auto o = omega(sr, clip_freq(sr, freq));
	const auto sn = std::sin(o);
	const auto cs = std::cos(o);

	const auto alfa = sn * (1.0f - res);

	bqap->a0 = bqap->b2 = 1.0f + alfa;
	bqap->a1 = bqap->b1 = cs * -2.0f;
	bqap->a2 = bqap->b0 = 1.0f - alfa;

	bqap->a0 = 1.0f / bqap->a0;
}

}}}