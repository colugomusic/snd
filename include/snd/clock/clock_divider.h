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

	ClockDivider& copy(const ClockDivider& rhs);
};

}}