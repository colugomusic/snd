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
	float update_rate_;
	int clock_divisor_;
	float clock_rate_;
	clock::ClockDivider clock_divider_;
	float ramp_inc_;
	float prev_val_ = 0.0;
	float ramp_ = 0.0f;
	float in_ = 0.0f;
	DupFilter<float> dup_filter_;

	static int calculate_clock_div(int SR, float update_rate);
	static float calculate_clock_rate(int SR, int clock_div);
	static float calculate_ramp_inc(float time, float clock_rate);

	void update();

public:

	Smoother(int SR, float time, float update_rate, std::function<void(float)> callback);

	void operator()(float in);

	void set_sample_rate(int sr);

	void copy(const Smoother& rhs);
	void reset();
};

}}