#include "threads/msg/msg_channel.h"

namespace snd {
namespace threads {
namespace msg {

Channel::Channel(size_t queue_size)
	: queue_(queue_size)
{
}

void Channel::run_tasks(int max)
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
bool Channel::run<true>(Task task)
{
	return queue_.enqueue(task);
}

template <>
bool Channel::run<false>(Task task)
{
	return queue_.try_enqueue(task);
}

}}}