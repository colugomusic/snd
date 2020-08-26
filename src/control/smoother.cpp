#include "control/smoother.h"
#include <algorithm>
#include <cmath>
#include "misc.h"

namespace snd {
namespace control {

Smoother::Smoother(int SR, float time, float update_rate, std::function<void(float)> callback)
	: sample_rate_(SR)
	, time_(time)
	, update_rate_(update_rate)
	, clock_divisor_(calculate_clock_div(SR, update_rate))
	, clock_rate_(calculate_clock_rate(SR, clock_divisor_))
	, clock_divider_(clock_divisor_, std::bind(&Smoother::update, this))
	, ramp_inc_(calculate_ramp_inc(time, clock_rate_))
	, dup_filter_(callback)
{
}

int Smoother::calculate_clock_div(int SR, float update_rate)
{
	return int(std::floor((1.0f / update_rate) * SR)) + 1;
}

float Smoother::calculate_clock_rate(int SR, int clock_div)
{
	return (1.0f / clock_div) * SR;
}

float Smoother::calculate_ramp_inc(float time, float clock_rate)
{
	return 1.0f / std::max(1.0f, time * clock_rate);
}

void Smoother::update()
{
	if (ramp_ > 0.0f)
	{
		ramp_ -= ramp_inc_;

		if (ramp_ < 0.0f) ramp_ = 0.0f;

		auto out = lerp(in_, prev_val_, ramp_);

		prev_val_ = out;

		dup_filter_(out);
	}
}

void Smoother::operator()(float in)
{
	in_ = in;
	ramp_ = 1.0f;

	clock_divider_();
}

void Smoother::set_sample_rate(int sr)
{
	sample_rate_ = sr;

	clock_divisor_ = calculate_clock_div(sr, update_rate_);
	clock_rate_ = calculate_clock_rate(sr, clock_divisor_);
	clock_divider_.set_divisor(clock_divisor_);
}

void Smoother::copy(const Smoother& rhs)
{
	sample_rate_ = rhs.sample_rate_;
	time_ = rhs.time_;
	update_rate_ = rhs.update_rate_;
	clock_divisor_ = rhs.clock_divisor_;
	clock_rate_ = rhs.clock_rate_;
	ramp_inc_ = rhs.ramp_inc_;
	prev_val_ = rhs.prev_val_;
	ramp_ = rhs.ramp_;
	in_ = rhs.in_;

	clock_divider_.copy(rhs.clock_divider_);
	dup_filter_.copy(rhs.dup_filter_);
}

void Smoother::reset()
{
	prev_val_ = in_;

	clock_divider_.reset();
	dup_filter_.reset();
}

}}