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

}}}