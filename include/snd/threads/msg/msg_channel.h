#pragma once

#include <functional>
#include <readerwriterqueue.h>

namespace snd {
namespace threads {
namespace msg {

class Channel
{
public:

	using Task = std::function<void()>;

private:

	using Queue = moodycamel::ReaderWriterQueue<Task>;

	Queue queue_;

public:

	Channel(size_t queue_size = 15);

	template <bool MAY_ALLOCATE = false>
	bool run(Task task);

	void run_tasks(int max = 0);
};

inline Channel::Channel(size_t queue_size)
	: queue_(queue_size)
{
}

inline void Channel::run_tasks(int max)
{
	Task task;

	int count = 0;

	while (queue_.try_dequeue(task))
	{
		task();

		if (max > 0 && ++count >= max) break;
	}
}

template <>
inline bool Channel::run<true>(Task task)
{
	return queue_.enqueue(task);
}

template <>
inline bool Channel::run<false>(Task task)
{
	return queue_.try_enqueue(task);
}

}}}
