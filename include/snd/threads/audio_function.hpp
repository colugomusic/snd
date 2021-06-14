#pragma once

#include <atomic>
#include <functional>

namespace snd {
namespace threads {

// A simple mechanism for signalling a function to be
// called in the audio thread.
//
// Intended for things like updating progress bars.
//
// Shouldn't be used for anything that requires precise
// memory ordering.
template <class F>
class AudioFunction
{
public:

	AudioFunction() = default;

	AudioFunction(std::function<F> function)
		: function_(function)
	{
	}

	void set(std::function<F> function)
	{
		function_ = function;
	}

	// call operator
	// the function will be called at least once as
	// long as the audio thread is running
	//
	// the function may be called more than once
	void operator()() { flag_ = true; }

	// Should be called by the audio thread
	// The function will be called if the flag
	// was set
	template <class ... Args>
	void respond(Args... args)
	{
		if (flag_)
		{
			flag_ = false;

			// (flag may already be reset here)
			
			function_(args...);
		}
	}

private:

	std::function<F> function_;
	std::atomic<bool> flag_;
};

} // threads
} // snd
