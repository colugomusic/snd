#pragma once

#include <functional>

namespace snd {
namespace clock {

class ClockDivider
{
	int counter_;
	int divisor_;
	std::function<void()> callback_;

public:

	ClockDivider(int divisor, std::function<void()> callback);

	void operator()();

	void set_divisor(int divisor);
	void reset();

	ClockDivider& copy(const ClockDivider& rhs);
};

inline ClockDivider::ClockDivider(int divisor, std::function<void()> callback)
	: counter_(divisor)
	, divisor_(divisor)
	, callback_(callback)
{
}

inline void ClockDivider::operator()()
{
	if (counter_ >= divisor_)
	{
		callback_();
		counter_ = 0;
	}

	counter_++;
}

inline void ClockDivider::set_divisor(int divisor)
{
	divisor_ = divisor;
	counter_ = divisor;
}

inline void ClockDivider::reset()
{
	counter_ = divisor_;
}

inline ClockDivider& ClockDivider::copy(const ClockDivider& rhs)
{
	counter_ = rhs.counter_;
	divisor_ = rhs.divisor_;

	return *this;
}

}}
