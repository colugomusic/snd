#pragma once

#include <cassert>
#include <functional>
#include <vector>

namespace snd {

template <class Value, size_t SIZE_, class Allocator = std::allocator<Value>>
class DeferredBuffer
{
public:

	static constexpr auto SIZE{ SIZE_ };

	using row_t = uint16_t;

	DeferredBuffer(row_t row_count = 1)
		: row_count_{ row_count }
		, rows_{ row_count }
	{
	}

	auto allocate() -> void
	{
		for (row_t row{}; row < row_count_; row++)
		{
			assert(rows_[row].empty());

			rows_[row].resize(SIZE);
		}
	}

	auto read(row_t row, size_t index) const -> float
	{
		assert(is_ready());
		assert(row < row_count_);

		return rows_[row][index];
	}

	auto read(row_t row, size_t index_beg, std::function<void(const Value*)> reader) const -> void
	{
		assert(is_ready());
		assert(row < row_count_);

		reader(&(rows_[row][index_beg]));
	}

	auto write(row_t row, size_t index, float value) -> void
	{
		assert(is_ready());
		assert(row < row_count_);

		rows_[row][index] = value;
	}

	auto write(row_t row, size_t index_beg, std::function<void(Value* value)> writer) -> void
	{
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