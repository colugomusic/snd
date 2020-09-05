#include "audio/filter/2-pole_allpass.h"
#include <algorithm>
#include <cmath>
#include <misc.h>

namespace snd {
namespace audio {
namespace filter {

static float clip_freq(float sr, float freq)
{
	const auto min = sr / 24576.0f;
	const auto max = sr / 2.125f;

	if (freq < min) return min;
	if (freq > max) return max;

	return freq;
}

static float omega(float sr, float freq)
{
	return freq * (6.28319f / sr);
}

Filter_2Pole_Allpass::Filter_2Pole_Allpass(const BQAP* data)
	: data_(data ? data : &bqap_)
{
}

ml::DSPVector Filter_2Pole_Allpass::operator()(const ml::DSPVector& in)
{
	ml::DSPVector out;

	for (int i = 0; i < kFloatsPerDSPVector; i++)
	{
		const auto a = in[i] * data_->c0;
		const auto b = fbk_val_0_ * data_->c1;
		const auto c = fbk_val_1_ * data_->c2;
		const auto d = fbk_val_2_ * -data_->c1;
		const auto e = fbk_val_3_ * -data_->c0;

		auto ap = a + b + c + d + e;

		flush_denormal_to_zero(ap);

		fbk_val_3_ = fbk_val_2_;
		fbk_val_2_ = ap;
		fbk_val_1_ = fbk_val_0_;
		fbk_val_0_ = in[i];

		out[i] = ap;
	}

	return out;
}

void Filter_2Pole_Allpass::copy(const Filter_2Pole_Allpass& rhs)
{
	if (data_ == &bqap_)
	{
		bqap_ = rhs.bqap_;
	}

	fbk_val_0_ = rhs.fbk_val_0_;
	fbk_val_1_ = rhs.fbk_val_1_;
	fbk_val_2_ = rhs.fbk_val_2_;
	fbk_val_3_ = rhs.fbk_val_3_;
}

void Filter_2Pole_Allpass::set(const BQAP& bqap)
{
	bqap_ = bqap;
}

void Filter_2Pole_Allpass::set_external_data(const BQAP* data)
{
	data_ = data;
}

void Filter_2Pole_Allpass::calculate(int sr, float freq, float res, BQAP* bqap)
{
	const auto o = omega(float(sr), clip_freq(float(sr), freq));
	const auto sn = std::sin(o);
	const auto cs = std::cos(o);

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