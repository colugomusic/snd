#include "control/smoother.h"
#include <algorithm>
#include <cmath>
#include "misc.h"

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace control {

Smoother::Smoother(int SR, float time, std::function<void(float)> callback)
	: sample_rate_(SR)
	, time_(time)
	, ramp_inc_(calculate_ramp_inc(SR, time))
	, dup_filter_(callback)
{
}

float Smoother::calculate_ramp_inc(int SR, float time)
{
	auto clock_rate = (1.0f / kFloatsPerDSPVector) * SR;

	return kFloatsPerDSPVector * (1.0f / std::max(1.0f, time * clock_rate));
}

void Smoother::operator()(float in)
{
	in_ = in;
	ramp_ = 1.0f;

	if (ramp_ > 0.0f)
	{
		ramp_ -= ramp_inc_;

		if (ramp_ < 0.0f) ramp_ = 0.0f;

		auto out = lerp(in_, prev_val_, ramp_);

		prev_val_ = out;

		dup_filter_(out);
	}
}

void Smoother::set_sample_rate(int sr)
{
	sample_rate_ = sr;

	ramp_inc_ = calculate_ramp_inc(sr, time_);
}

void Smoother::copy(const Smoother& rhs)
{
	sample_rate_ = rhs.sample_rate_;
	time_ = rhs.time_;
	ramp_inc_ = rhs.ramp_inc_;
	prev_val_ = rhs.prev_val_;
	ramp_ = rhs.ramp_;
	in_ = rhs.in_;

	dup_filter_.copy(rhs.dup_filter_);
}

void Smoother::reset()
{
	prev_val_ = in_;

	dup_filter_.reset();
}

}}