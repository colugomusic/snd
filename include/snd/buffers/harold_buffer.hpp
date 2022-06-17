#pragma once

//
// required libraries:
//	- colugomusic/stupid
//

#include <cassert>
#include <memory>
#include <vector>
#include <snd/buffers/stanley_buffer_pool.hpp>
#include <snd/samples/sample_mipmap.hpp>

namespace snd {

static constexpr size_t HAROLD_BUFFER_DEFAULT_ALLOC_SIZE{ 16 };

//
// A sequence of Stanley buffers
//
// Old stanley buffers are returned to the pool
// when this is destructed
//
template <
	// How large are the sub buffers?
	size_t SUB_BUFFER_SIZE = STANLEY_BUFFER_DEFAULT_SIZE,

	// How many sub buffers should we allocate at a time?
	// Allocation is going to block the thread (probably
	// your GUI thread) so this should be small enough
	// that the pause is not visible to the human eye
	size_t ALLOC_SIZE = HAROLD_BUFFER_DEFAULT_ALLOC_SIZE,

	class Allocator = std::allocator<float>>
class HaroldBuffer
{
public:

	using buffer_pool_t = StanleyBufferPool<SUB_BUFFER_SIZE, Allocator>;
	using buffer_t = typename buffer_pool_t::buffer_t;
	using row_t = typename buffer_t::row_t;

	HaroldBuffer(buffer_pool_t* buffer_pool, row_t row_count, uint32_t required_size);

	~HaroldBuffer();

	// The required size requested by the client
	auto get_size() const { return size_; }

	// The total capacity of the underlying sub buffers
	auto get_actual_size() const { return actual_size_; }

	// Total number of sub buffers
	auto get_buffer_count() const { return buffers_.size(); }

	// Total number of sub buffers which have been allocated
	auto get_allocated_buffers() const { return allocated_buffers_; }

	// Clear the visual mipmap data
	auto clear_mipmap() -> void;

	// True if all sub buffers have been allocated
	auto is_ready() const -> bool;

	//
	// Keep calling this in the GUI thread to allocate sub
	// buffers a little bit at a time.
	//
	// returns false when all sub buffers have been allocated
	//
	// Allocation progress (e.g. for displaying progress
	// bars) can be calculated as:
	//
	//	float(get_allocated_buffers()) / get_buffer_count()
	//
	auto allocate_buffers() -> bool;

	//
	// Call this continuously in the GUI thread if you know
	// the buffer is visible and changing. It doesn't do
	// anything if the top-level mipmap data hasn't changed.
	//
	auto generate_mipmaps() -> bool;

	// We never acquire new sub buffers, so resize() returns
	// false if there are not enough sub buffers to support
	// the requested size (in which case you should throw
	// this Harold buffer away and create a new one of the
	// required size
	auto resize(uint32_t required_size) -> bool;

	// Reading from a region of the buffer which has not been
	// allocated yet will return zero
	auto read(row_t row, uint32_t frame) const -> float;

	// Writing to a region of the buffer which has not been
	// allocated yet will not do anything
	auto write(row_t row, uint32_t frame, float value) -> void;

	//
	// Read data in such a way that each chunk of data read
	// will always belong to the same sub buffer.
	//
	// For example if the size of each sub buffer is a
	// multiple of 64 then you can read in chunks of 64
	// frames
	//
	// If the sub buffer for a chunk has not been allocated
	// yet then chunk_not_ready is called instead of the
	// reader
	//
	auto read_aligned(
		row_t row, 
		uint32_t frame_beg, 
		uint32_t frames_to_read, 
		uint32_t chunk_size, 
		std::function<void(const float* data)> reader,
		std::function<void()> chunk_not_ready) const -> void;

	//
	// Write data in such a way that each chunk of data
	// written will always belong to the same sub buffer.
	//
	// For example if the size of each sub buffer is a
	// multiple of 64 then you can write in chunks of 64
	// frames
	//
	auto write_aligned(
		row_t row, 
		uint32_t frame_beg, 
		uint32_t frames_to_write, 
		uint32_t chunk_size, 
		std::function<void(float* data)> writer) -> void;

	//
	// The range specified by frame_beg/frames_to_read
	// must fall entirely within a single sub-buffer
	//
	auto read_sub_buffer(
		row_t row, 
		uint32_t frame_beg, 
		uint32_t frames_to_read, 
		std::function<void(const float* data)> reader) const -> bool;

	//
	// The range specified by frame_beg/frames_to_write
	// must fall entirely within a single sub-buffer
	//
	auto write_sub_buffer(
		row_t row, 
		uint32_t frame_beg, 
		uint32_t frames_to_write, 
		std::function<void(float* data)> writer) -> bool;

	// Read final generated mipmap data
	auto read_mipmap(row_t row, float frame, float bin_size) const -> snd::SampleMipmap::LODFrame;

	//
	// Call this in your audio thread after you finish
	// writing audio data to the buffer
	//
	// Preferably call this once per audio callback. We
	// will write mipmap data for all dirty regions
	//
	auto write_audio_mipmap_data() -> void;

private:

	struct Buffer
	{
		bool dirty{};
		std::unique_ptr<buffer_t> ptr;

		Buffer(std::unique_ptr<buffer_t> ptr_) : ptr{ std::move(ptr_) } {}
	};

	uint32_t size_{};
	uint32_t actual_size_{};

	auto acquire_buffers(row_t row_count) -> void;
	auto get_buffer(uint32_t frame) -> Buffer*;
	auto get_buffer(uint32_t frame) const -> const Buffer*;

	static auto get_local_frame(uint32_t frame) -> uint32_t;

	std::vector<Buffer> buffers_;
	buffer_pool_t* buffer_pool_;
	int allocated_buffers_{};
	bool buffer_dirt_flag_{};
};

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::HaroldBuffer(buffer_pool_t* buffer_pool, row_t row_count, uint32_t required_size)
	: size_ { required_size }
	, buffer_pool_{ buffer_pool }
{
	acquire_buffers(row_count);
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::~HaroldBuffer()
{
	for (auto& buffer : buffers_)
	{
		if (!buffer.ptr->is_ready()) continue;

		buffer_pool_->release(std::move(buffer.ptr));
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::acquire_buffers(row_t row_count) -> void
{
	while (actual_size_ < size_)
	{
		Buffer buffer{ buffer_pool_->acquire(row_count) };

		buffers_.push_back(std::move(buffer));

		actual_size_ += SUB_BUFFER_SIZE;
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::clear_mipmap() -> void
{
	for (auto& buffer : buffers_)
	{
		buffer.ptr->gui.clear_mipmap();
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::is_ready() const -> bool
{
	return allocated_buffers_ >= buffers_.size();
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::resize(uint32_t required_size) -> bool
{
	if (required_size > actual_size_) return false;

	size_ = required_size;

	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::get_local_frame(uint32_t frame) -> uint32_t
{
	return frame % SUB_BUFFER_SIZE;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::get_buffer(uint32_t frame) -> Buffer*
{
	if (frame >= size_) return {};

	const auto buffer_index{ frame / SUB_BUFFER_SIZE };

	assert(buffer_index < buffers_.size());

	auto& buffer{ buffers_[buffer_index] };

	if (!buffer.ptr->is_ready()) return {};

	return &buffer;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::get_buffer(uint32_t frame) const -> const Buffer*
{
	return const_cast<HaroldBuffer*>(this)->get_buffer(frame);
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::read(row_t row, uint32_t frame) const -> float
{
	auto buffer{ get_buffer(frame) };

	if (!buffer) return 0.0f;

	return buffer->ptr->audio.read(row, get_local_frame(frame));
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::read_aligned(
	row_t row, 
	uint32_t frame_beg, 
	uint32_t frames_to_read, 
	uint32_t chunk_size, 
	std::function<void(const float* data)> reader,
	std::function<void()> chunk_not_ready) const -> void
{
	auto frames_remaining{ frames_to_read };

	while (frames_remaining > 0)
	{
		const auto chunk_frames_to_read{ std::min(frames_remaining, chunk_size) };

		if (!read_sub_buffer(row, frame_beg, chunk_frames_to_read, reader))
		{
			chunk_not_ready();
		}

		frame_beg += chunk_size;
		frames_remaining -= chunk_frames_to_read;
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::write(row_t row, uint32_t frame, float value) -> void
{
	auto buffer{ get_buffer(frame) };

	if (!buffer) return;

	buffer->ptr->audio.write(row, get_local_frame(frame), value);
	buffer->dirty = true;
	buffer_dirt_flag_ = true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::read_sub_buffer(
	row_t row, 
	uint32_t frame_beg, 
	uint32_t frames_to_read, 
	std::function<void(const float* data)> reader) const -> bool
{
	assert((frame_beg / SUB_BUFFER_SIZE) == ((frame_beg + (frames_to_read - 1)) / SUB_BUFFER_SIZE));

	auto buffer{ get_buffer(frame_beg) };

	if (!buffer) return false;

	buffer->ptr->audio.read(row, get_local_frame(frame_beg), frames_to_read, reader);

	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::write_sub_buffer(
	row_t row, 
	uint32_t frame_beg, 
	uint32_t frames_to_write, 
	std::function<void(float* data)> writer) -> bool
{
	assert((frame_beg / SUB_BUFFER_SIZE) == ((frame_beg + (frames_to_write - 1)) / SUB_BUFFER_SIZE));

	auto buffer{ get_buffer(frame_beg) };

	if (!buffer) return false;

	buffer->ptr->audio.write(row, get_local_frame(frame_beg), frames_to_write, writer);
	buffer->dirty = true;
	buffer_dirt_flag_ = true;

	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::write_aligned(row_t row, uint32_t frame_beg, uint32_t frames_to_write, uint32_t chunk_size, std::function<void(float* data)> writer) -> void
{
	auto frames_remaining{ frames_to_write };

	while (frames_remaining > 0)
	{
		const auto chunk_frames_to_write{ std::min(frames_remaining, chunk_size) };

		write_sub_buffer(row, frame_beg, chunk_frames_to_write, writer);

		frame_beg += chunk_size;
		frames_remaining -= chunk_frames_to_write;
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::allocate_buffers() -> bool
{
	if (allocated_buffers_ >= buffers_.size())
	{
		return false;
	}

	//
	// StanleyBuffer::allocate() does not necessarily need to
	// allocate actual memory because it may be being re-used
	// in which case it is already ready to go.
	//
	// We try to do exactly ALLOC_SIZE actual memory allocations
	// here so that this method takes more or less the same
	// amount of time each time it is called
	//
	const auto unallocated_buffers{ buffers_.size() - allocated_buffers_ };
	auto remaining{ std::min(unallocated_buffers, ALLOC_SIZE) };

	while (remaining > 0)
	{
		if (allocated_buffers_ >= buffers_.size()) return true;

		const auto& buffer{ buffers_[allocated_buffers_++] };

		// Returns false if no actual memory was allocated
		if (buffer.ptr->gui.allocate())
		{
			remaining--;
		}
	}

	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::generate_mipmaps() -> bool
{
	bool mipmap_generated{};

	for (const auto& buffer : buffers_)
	{
		auto result{ buffer.ptr->gui.process_mipmap() };

		if (result) mipmap_generated = true;
	}

	return mipmap_generated;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::write_audio_mipmap_data() -> void
{
	if (!buffer_dirt_flag_) return;

	bool still_dirty{ false };

	for (auto& buffer : buffers_)
	{
		if (buffer.dirty)
		{
			if (buffer.ptr->audio.process_mipmap())
			{
				buffer.dirty = false;
			}
			else
			{
				still_dirty = true;
			}
		}
	}

	if (!still_dirty)
	{
		buffer_dirt_flag_ = false;
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::read_mipmap(row_t row, float frame, float bin_size) const -> snd::SampleMipmap::LODFrame
{
	const auto index_a{ static_cast<uint32_t>(std::floor(frame)) };
	const auto index_b{ static_cast<uint32_t>(std::ceil(frame)) };
	const auto t{ frame - index_a };

	const auto buffer_index_a{ index_a / SUB_BUFFER_SIZE };
	const auto buffer_index_b{ index_b / SUB_BUFFER_SIZE };

	assert(buffer_index_a < buffers_.size());
	assert(buffer_index_b < buffers_.size());

	const auto local_frame_a{ index_a % SUB_BUFFER_SIZE };
	const auto local_frame_b{ index_b % SUB_BUFFER_SIZE };

	auto& buffer_a{ *buffers_[buffer_index_a].ptr };
	auto& buffer_b{ *buffers_[buffer_index_b].ptr };

	const auto frame_a{ buffer_a.gui.read_mipmap(row, local_frame_a, bin_size) };
	const auto frame_b{ buffer_b.gui.read_mipmap(row, local_frame_b, bin_size) };

	return snd::SampleMipmap::LODFrame::lerp(frame_a, frame_b, t);
}

} // snd
