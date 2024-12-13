#pragma once

#if defined(SND_WITH_MOODYCAMEL)

#include <readerwriterqueue.h>

namespace snd {
namespace th {

struct message_queue_reporter {
	std::function<void()> error_queue_beyond_reasonable_size;
	std::function<void()> warning_queue_full;
};

template <size_t INITIAL_QUEUE_SIZE>
static constexpr auto WORRISOME_QUEUE_SIZE = INITIAL_QUEUE_SIZE * 8;

template <size_t INITIAL_QUEUE_SIZE, typename T>
struct message_queue {
	using value_type = T;
	moodycamel::ReaderWriterQueue<T> q = moodycamel::ReaderWriterQueue<T>(INITIAL_QUEUE_SIZE);
};

namespace realtime {

template <size_t INITIAL_QUEUE_SIZE, typename T>
auto send(message_queue<INITIAL_QUEUE_SIZE, T>* q, const T& value, const message_queue_reporter& reporter) -> void {
	if (!q->q.try_enqueue(value)) {
		if (q->q.size_approx() > WORRISOME_QUEUE_SIZE<INITIAL_QUEUE_SIZE>) {
			reporter.error_queue_beyond_reasonable_size();
			return;
		}
		reporter.warning_queue_full();
		q->q.enqueue(value);
	}
}

} // realtime

namespace non_realtime {

template <size_t INITIAL_QUEUE_SIZE, typename T>
auto send(message_queue<INITIAL_QUEUE_SIZE, T>* q, const T& value) -> void {
	q->q.enqueue(value);
}

} // non_realtime

template <size_t INITIAL_QUEUE_SIZE, typename T, typename ReceiveFn>
auto receive(message_queue<INITIAL_QUEUE_SIZE, T>* q, ReceiveFn&& receive) -> void {
	T msg;
	while (q->q.try_dequeue(msg)) {
		receive(msg);
	}
}

template <size_t INITIAL_QUEUE_SIZE, typename T, typename ReceiveFn>
auto receive_one(message_queue<INITIAL_QUEUE_SIZE, T>* q, ReceiveFn&& receive) -> bool {
	T msg;
	if (q->q.try_dequeue(msg)) {
		receive(msg);
		return true;
	}
	return false;
}

} // th
} // snd

#endif // SND_WITH_MOODYCAMEL
