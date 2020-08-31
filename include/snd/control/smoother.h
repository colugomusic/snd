#pragma once

#include <limits>
#include "../clock/clock_divider.h"
#include "../dup_filter.h"

namespace snd {
namespace control {

class Smoother
{
	int sample_rate_;
	float time_;
	float ramp_inc_;
	float prev_val_ = 0.0;
	float ramp_ = 0.0f;
	float in_ = 0.0f;
	DupFilter<float> dup_filter_;

	static float calculate_ramp_inc(int SR, float time);

public:

	Smoother(int SR, float time, std::function<void(float)> callback);

	void operator()(float in);

	void set_sample_rate(int sr);

	void copy(const Smoother& rhs);
	void reset();
};

}}