#pragma once

#include <cassert>
#include <vector>

namespace snd {

template <class Value, size_t SIZE_, class Allocator = std::allocator<Value>>
struct DeferredBuffer {
	static constexpr auto SIZE = SIZE_;
	using row_t = uint16_t;
	using index_t = uint64_t;
	DeferredBuffer(row_t row_count = 1)
		: row_count_{ row_count }
		, rows_{ row_count }
	{
	}
	auto allocate() -> void {
		for (row_t row{}; row < row_count_; row++) {
			assert(rows_[row].empty()); 
			rows_[row].resize(SIZE);
		}
	} 
	auto fill(row_t row, float value) -> void {
		std::fill(rows_[row].begin(), rows_[row].end(), value);
	} 
	auto read(row_t row, index_t index) const -> float {
		assert(is_ready());
		assert(row < row_count_); 
		return rows_[row][index];
	} 
	template <typename ReaderFn>
	auto read(row_t row, index_t index_beg, ReaderFn&& reader) const -> void {
		assert(is_ready());
		assert(row < row_count_);
		reader(&(rows_[row][index_beg]));
	}
	auto write(row_t row, index_t index, float value) -> void {
		assert(is_ready());
		assert(row < row_count_);
		rows_[row][index] = value;
	}
	template <typename WriterFn>
	auto write(row_t row, index_t index_beg, WriterFn&& writer) -> void {
		assert(is_ready());
		assert(row < row_count_);
		writer(&(rows_[row][index_beg]));
	} 
	auto is_ready() const { return !rows_[0].empty(); }
private:
	row_t row_count_;
	std::vector<std::vector<Value, Allocator>> rows_;
};

} // snd