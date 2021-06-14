#pragma once

#include <cstdint>
#include <functional>
#include <map>

namespace snd {
namespace threads {

class BufferSyncSignal
{
public:

	std::uint32_t value() const { return value_; }

	// Should be called once at the start of each audio buffer
	void operator()()
	{
		value_++;
	}

private:

	std::uint32_t value_ = 0;
};

} // threads
} // snd
