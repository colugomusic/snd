#pragma once

#include <array>
#include "msg_channel.h"

namespace snd {
namespace threads {
namespace msg {

template <size_t SIZE>
class Comms
{
private:

	std::array<Channel, SIZE> channels_;

public:

	template <class ... Sizes>
	Comms(Sizes... queue_sizes)
		: channels_{ queue_sizes... }
	{
	}

	template <size_t CHANNEL, bool MAY_ALLOCATE = false>
	bool run(Channel::Task task);

	template <size_t CHANNEL>
	void run_tasks(int max = 0);
};

template <size_t SIZE>
template <size_t CHANNEL, bool MAY_ALLOCATE>
bool Comms<SIZE>::run(Channel::Task task)
{
	return channels_[CHANNEL].run<MAY_ALLOCATE>(task);
}

template <size_t SIZE>
template <size_t CHANNEL>
void Comms<SIZE>::run_tasks(int max)
{
	return channels_[CHANNEL].run_tasks(max);
}

}}}