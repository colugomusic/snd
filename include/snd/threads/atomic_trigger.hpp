#pragma once

#include <atomic>

namespace snd {
namespace threads {

struct AtomicTrigger
{
	void operator()()
	{
		flag_ = true;
	}

	operator bool()
	{
		return flag_.exchange(false);
	}

private:

	std::atomic<bool> flag_ = false;
};

} // threads
} // snd
