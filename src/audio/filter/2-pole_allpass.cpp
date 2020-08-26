#include "audio/filter/2-pole_allpass.h"
#include <algorithm>
#include <cmath>

namespace snd {
namespace audio {
namespace filter {

static double clip_freq(double sr, double freq)
{
	const auto min = sr / 24576.0;
	const auto max = sr / 2.125;

	if (freq < min) return min;
	if (freq > max) return max;

	return freq;
}

static double omega(double sr, double freq)
{
	return freq * (6.28319 / sr);
}

Filter_2Pole_Allpass::Filter_2Pole_Allpass(const BQAP* data)
	: data_(data ? data : &bqap_)
{
}

float Filter_2Pole_Allpass::operator()(float in)
{
	const auto a = double(in) * (data_->a2 * data_->a0);
	const auto b = zdfbk_val_0_ * (data_->a1 * data_->a0);
	const auto c = zdfbk_val_1_ * (data_->b2 * data_->a0);
	const auto d = zdfbk_val_2_ * -(data_->a1 * data_->a0);
	const auto e = zdfbk_val_3_ * -(data_->a2 * data_->a0);

	const auto ap = a + b + c + d + e;

	zdfbk_val_3_ = zdfbk_val_2_;
	zdfbk_val_2_ = ap;
	zdfbk_val_1_ = zdfbk_val_0_;
	zdfbk_val_0_ = double(in);

	return float(ap);
}

void Filter_2Pole_Allpass::copy(const Filter_2Pole_Allpass& rhs)
{
	if (data_ == &bqap_)
	{
		bqap_ = rhs.bqap_;
	}

	zdfbk_val_0_ = rhs.zdfbk_val_0_;
	zdfbk_val_1_ = rhs.zdfbk_val_1_;
	zdfbk_val_2_ = rhs.zdfbk_val_2_;
	zdfbk_val_3_ = rhs.zdfbk_val_3_;
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
	const auto o = omega(double(sr), clip_freq(double(sr), double(freq)));
	const auto sn = std::sin(o);
	const auto cs = std::cos(o);

	const auto alfa = sn * (1.0 - res);

	bqap->b2 = 1.0 + alfa;
	bqap->a1 = cs * -2.0;
	bqap->a2 = 1.0 - alfa;

	bqap->a0 = 1.0 / bqap->b2;
}

}}}