#pragma once

//
// required libraries:
//	- cameron314/readerwriterqueue
//

#include <deque>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <readerwriterqueue.h>

namespace snd {
namespace th {

static constexpr auto GUI{ 0 };
static constexpr auto AUDIO{ 1 };
static constexpr auto WORKER{ 2 };
static constexpr auto MIDI{ 3 };

namespace task_queues {

using Task = std::function<void()>;

//
// Generic queue for tasks to be processed
//
// - Tasks may only be pushed from the specified PUSH_THREAD.
// 
// - Tasks may only be processed in the specified PROCESS_THREAD.
//
// - Tasks can optionally expire. Expired tasks are not processed.
//   The expiry mechanism is specified by the ExpiryPolicy.
//
// - The queue can be single threaded or multithreaded, as
//   specified by the QueuePolicy.
//
// - The THREAD parameter just exists to force clients at the
//   call site to specify which thread they are executing in.
//   Nothing stops them from lying.
//
// - If these classes look completely bizarre to you then google
//   for "policy based class design". This won't stop them from
//   looking bizarre because policy based class design is
//   for idiots
//
template <class TaskType, class ExpiryPolicy, class QueuePolicy, int PUSH_THREAD, int PROCESS_THREAD>
class TaskQueue : public ExpiryPolicy, public QueuePolicy
{
public:

	template <int THREAD>
	auto push(TaskType task) -> void
	{
		static_assert(THREAD == PUSH_THREAD);

		QueuePolicy::push(task);
	}

	template <int THREAD>
	auto process_all() -> void
	{
		static_assert(THREAD == PROCESS_THREAD);

		TaskType task;

		while (QueuePolicy::pop(&task))
		{
			if (ExpiryPolicy::is_expired(task))
			{
				continue;
			}

			task();
		}

		ExpiryPolicy::done_processing();
	}
};

class NoExpiryPolicy
{
	template <class TaskType, class ExpiryPolicy, class QueuePolicy, int WORKER_THREAD, int PROCESS_THREAD>
	friend class TaskQueue;

	template <class TaskType>
	auto is_expired(const TaskType& task) const -> bool { return false; }

	auto done_processing() -> void {}
};

//
// A task with an associated object
//
template <class ObjectID = int64_t> struct ObjectTask
{
	ObjectID object;
	Task task;

	auto operator()() { task(); }
};

//
// When objects are deleted the client must explicitly call
// deleted() in the PROCESS_THREAD with the object's ID
//
template <class ObjectID, int PROCESS_THREAD>
class ObjectDeletionTaskExpiryPolicy
{
	template <class TaskType, class ExpiryPolicy, class QueuePolicy, int WORKER_THREAD, int PROCESS_THREAD>
	friend class TaskQueue;

public:

	//
	// A sane place to call this would be
	// in the destructor of the object
	//
	template <int THREAD>
	auto deleted(ObjectID object) -> void
	{
		static_assert(THREAD == PROCESS_THREAD);

		deleted_objects_.insert(object);
	}

private:

	auto is_expired(const ObjectTask<ObjectID>& task) const -> bool
	{
		return deleted_objects_.find(task.object) != deleted_objects_.end();
	}

	auto done_processing() -> void
	{
		deleted_objects_.clear();
	}

	std::unordered_set<ObjectID> deleted_objects_;
};

template <int PROCESS_THREAD>
class ShutdownExpiryPolicy
{
	template <class TaskType, class ExpiryPolicy, class QueuePolicy, int WORKER_THREAD, int PROCESS_THREAD>
	friend class TaskQueue;

public:

	//
	// No more tasks will be processed
	// after this is called
	//
	template <int THREAD>
	auto shutdown() -> void
	{
		static_assert(THREAD == PROCESS_THREAD);

		stopped_ = true;
	}

private:

	template <class TaskType>
	auto is_expired(const TaskType& task) const -> bool
	{
		return stopped_;
	}

	auto done_processing() -> void {}

	bool stopped_{};
};

template <class TaskType>
class SingleThreadedQueuePolicy
{
	template <class TaskType, class ExpiryPolicy, class QueuePolicy, int WORKER_THREAD, int PROCESS_THREAD>
	friend class TaskQueue;

	auto push(TaskType task) -> void
	{
		queue_.push_back(task);
	}

	auto pop(TaskType* task) -> bool
	{
		if (queue_.empty()) return false;

		*task = queue_.front();

		queue_.pop_front();

		return true;
	}

private:

	std::deque<TaskType> queue_;
};

template <class TaskType>
class LockingQueuePolicy
{
	template <class TaskType, class ExpiryPolicy, class QueuePolicy, int WORKER_THREAD, int PROCESS_THREAD>
	friend class TaskQueue;

	auto push(TaskType task) -> void
	{
		std::lock_guard<std::mutex> lock{ mutex_ };

		queue_.push_back(task);
	}

	auto pop(TaskType* task) -> bool
	{
		std::lock_guard<std::mutex> lock{ mutex_ };

		if (queue_.empty()) return false;

		*task = queue_.front();

		queue_.pop_front();

		return true;
	}

private:

	std::mutex mutex_;
	std::deque<TaskType> queue_;
};

template <class TaskType, size_t SIZE>
class LockFreeQueuePolicy
{
	template <class TaskType, class ExpiryPolicy, class QueuePolicy, int WORKER_THREAD, int PROCESS_THREAD>
	friend class TaskQueue;

	//
	// SIZE should be set so that the call to try_emplace
	// never fails.
	//
	// In release builds more memory is allocated when we run
	// out of space in the queue, which should never happen
	// because that is a failure case which should have been
	// caught during development.
	//
	auto push(TaskType task) -> void
	{
#		if _DEBUG
			const auto success{ queue_.try_emplace(task) };

			assert(success);
#		else
			queue_.emplace(task);
#		endif
	}

	auto pop(TaskType* task) -> bool
	{
		return queue_.try_dequeue(*task);
	}

private:

	moodycamel::ReaderWriterQueue<TaskType> queue_{ SIZE };
};

//
// Push tasks from the audio thread to be executed in the
// GUI thread (lock-free)
//
template <size_t SIZE>
class AudioToGui : public TaskQueue
<
	Task,
	ShutdownExpiryPolicy<th::GUI>,
	LockFreeQueuePolicy<Task, SIZE>,
	th::AUDIO,
	th::GUI
> {};

//
// Push object tasks from the audio thread to be executed
// in the GUI thread (lock-free)
//
template <class ObjectID, size_t SIZE>
class AudioObjectToGui : public TaskQueue
<
	ObjectTask<ObjectID>,
	ObjectDeletionTaskExpiryPolicy<ObjectID, th::GUI>,
	LockFreeQueuePolicy<ObjectTask<ObjectID>, SIZE>,
	th::AUDIO,
	th::GUI
> {};

//
// Push tasks from the GUI thread to be executed in the
// audio thread (lock-free)
//
template <size_t SIZE>
class GuiToAudio : public TaskQueue
<
	Task,
	ShutdownExpiryPolicy<th::GUI>,
	LockFreeQueuePolicy<Task, SIZE>,
	th::GUI,
	th::AUDIO
> {};

//
// Push tasks in the GUI thread to be executed in the
// GUI thread (i.e. single-threaded, no synchronization)
//
class Gui : public TaskQueue
<
	Task,
	ShutdownExpiryPolicy<th::GUI>,
	SingleThreadedQueuePolicy<Task>,
	th::GUI,
	th::GUI
> {};

//
// Push object tasks in the GUI thread to be executed in
// the GUI thread (i.e. single-threaded, no
// synchronization)
//
template <class ObjectID>
class GuiObject : public TaskQueue
<
	ObjectTask<ObjectID>,
	ObjectDeletionTaskExpiryPolicy<ObjectID, th::GUI>,
	SingleThreadedQueuePolicy<ObjectTask<ObjectID>>,
	th::GUI,
	th::GUI
> {};

//
// Push tasks from a worker thread to be executed in the
// GUI thread (synchronization using mutex)
//
class WorkerToGui : public TaskQueue
<
	Task,
	ShutdownExpiryPolicy<th::GUI>,
	LockingQueuePolicy<Task>,
	th::WORKER,
	th::GUI
> {};

//
// Push object tasks from a worker thread to be executed
// in the GUI thread (synchronization using mutex)
//
template <class ObjectID>
class WorkerObjectToGui : public TaskQueue
<
	ObjectTask<ObjectID>,
	ObjectDeletionTaskExpiryPolicy<ObjectID, th::GUI>,
	LockingQueuePolicy<ObjectTask<ObjectID>>,
	th::WORKER,
	th::GUI
> {};

//
// MIDI thread is locking for now.
//
class MidiToGui : public TaskQueue
<
	Task,
	ShutdownExpiryPolicy<th::GUI>,
	LockingQueuePolicy<Task>,
	th::MIDI,
	th::GUI
> {};

} // task_queues
} // th
} // snd