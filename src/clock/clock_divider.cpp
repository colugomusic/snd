#include "clock/clock_divider.h"

namespace snd {
namespace clock {

ClockDivider::ClockDivider(int divisor, std::function<void()> callback)
	: counter_(divisor)
	, divisor_(divisor)
	, callback_(callback)
{
}

void ClockDivider::operator()()
{
	if (counter_ >= divisor_)
	{
		callback_();
		counter_ = 0;
	}

	counter_++;
}

void ClockDivider::set_divisor(int divisor)
{
	divisor_ = divisor;
	counter_ = divisor;
}

void ClockDivider::reset()
{
	counter_ = divisor_;
}

ClockDivider& ClockDivider::copy(const ClockDivider& rhs)
{
	counter_ = rhs.counter_;
	divisor_ = rhs.divisor_;

	return *this;
}

}}