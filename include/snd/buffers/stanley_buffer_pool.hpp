#pragma once

#include <cassert>
#include <deque>
#include <unordered_map>
#include "stanley_buffer.hpp"

namespace snd {

template <size_t SIZE = STANLEY_BUFFER_DEFAULT_SIZE, class Allocator = std::allocator<float>>
class StanleyBufferPool
{
public:

	using buffer_t = StanleyBuffer<SIZE, Allocator>;
	using row_t = typename buffer_t::row_t;

	auto acquire(row_t row_count) -> std::unique_ptr<buffer_t>;
	auto release(std::unique_ptr<buffer_t> sbuffer) -> void;

private:

	std::unordered_map<row_t, std::deque<std::unique_ptr<buffer_t>>> buffers_;
};

template <size_t SIZE, class Allocator>
auto StanleyBufferPool<SIZE, Allocator>::acquire(row_t row_count) -> std::unique_ptr<buffer_t>
{
	if (buffers_[row_count].empty())
	{
		return std::make_unique<buffer_t>(row_count);
	}

	auto out{ std::move(buffers_[row_count].front()) };

	buffers_[row_count].pop_front();

	return out;
}

template <size_t SIZE, class Allocator>
auto StanleyBufferPool<SIZE, Allocator>::release(std::unique_ptr<buffer_t> buffer) -> void
{
	buffer->gui.release();
	buffers_[buffer->row_count].push_back(std::move(buffer));
}

} // snd
