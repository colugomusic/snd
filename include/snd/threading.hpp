#pragma once

namespace snd {
namespace th {

struct error_queue_beyond_reasonable_size {};
struct warning_queue_full {};

template <size_t INITIAL_QUEUE_SIZE>
static constexpr auto WORRISOME_QUEUE_SIZE = INITIAL_QUEUE_SIZE * 8;

namespace realtime {

template <typename QueueImpl, typename T, typename Reporter>
auto send(QueueImpl* q, T&& value, Reporter* reporter) -> void {
	if (!try_enqueue(q, std::forward<T>(value))) {
		if (size_approx(*q) > QueueImpl::WORRISOME_QUEUE_SIZE) {
			send(reporter, error_queue_beyond_reasonable_size{});
			return;
		}
		send(reporter, warning_queue_full{});
		enqueue(q, std::forward<T>(value));
	}
}

} // realtime

namespace non_realtime {

template <typename QueueImpl, typename T>
auto send(QueueImpl* q, T&& value) -> void {
	enqueue(q, std::forward<T>(value));
}

} // non_realtime

} // th
} // snd

#if defined(SND_WITH_MOODYCAMEL)

#ifdef SND_WITH_MOODYCAMEL
#include <readerwriterqueue.h>
#endif

namespace snd {
namespace th {

template <size_t INITIAL_QUEUE_SIZE, typename T>
struct mc_queue {
	static constexpr auto WORRISOME_QUEUE_SIZE = snd::th::WORRISOME_QUEUE_SIZE<INITIAL_QUEUE_SIZE>;
	using value_type = T;
	moodycamel::ReaderWriterQueue<T> q = moodycamel::ReaderWriterQueue<T>(INITIAL_QUEUE_SIZE);
};

template <size_t INITIAL_QUEUE_SIZE, typename T>
auto try_enqueue(mc_queue<INITIAL_QUEUE_SIZE, T>* q, T&& value) -> bool {
	return q->q.try_enqueue(std::forward<T>(value));
}

template <size_t INITIAL_QUEUE_SIZE, typename T>
auto enqueue(mc_queue<INITIAL_QUEUE_SIZE, T>* q, T&& value) -> void {
	q->q.enqueue(std::forward<T>(value));
}

template <size_t INITIAL_QUEUE_SIZE, typename T> [[nodiscard]]
auto size_approx(mc_queue<INITIAL_QUEUE_SIZE, T> const& q) -> size_t {
	return q.q.size_approx();
}

#endif

} // th
} // snd
