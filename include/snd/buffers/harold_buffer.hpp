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
	class Allocator = ::std::allocator<float>
>
struct HaroldBuffer { 
	using buffer_pool_t = StanleyBufferPool<SUB_BUFFER_SIZE, Allocator>;
	using buffer_t      = typename buffer_pool_t::buffer_t;
	using row_t         = typename buffer_t::row_t;
	using frame_t       = uint64_t;
private: 
	struct Buffer {
		bool dirty{};
		std::unique_ptr<buffer_t> ptr; 
		Buffer(std::unique_ptr<buffer_t> ptr_) : ptr{ std::move(ptr_) } {}
	}; 
public: 
	HaroldBuffer(std::shared_ptr<buffer_pool_t> buffer_pool, row_t row_count, frame_t required_size);
	~HaroldBuffer(); 
	// Audio thread should only access
	// the buffer through this interface
	struct AudioAccess { 
		AudioAccess(HaroldBuffer* self) : SELF{ self } {} 
		// Reading from a region of the buffer which has not been
		// allocated yet will return zero
		auto read(row_t row, frame_t frame) const -> float; 
		// Writing to a region of the buffer which has not been
		// allocated yet will not do anything
		auto write(row_t row, frame_t frame, float value) -> void; 
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
		template <typename ReaderFn, typename ChunkNotReadyFn>
		auto read_aligned(
			row_t row, 
			frame_t frame_beg, 
			frame_t frames_to_read, 
			frame_t chunk_size,
			ReaderFn&& reader,
			ChunkNotReadyFn&& chunk_not_ready) const -> void;
		// Write data in such a way that each chunk of data
		// written will always belong to the same sub buffer.
		//
		// For example if the size of each sub buffer is a
		// multiple of 64 then you can write in chunks of 64
		// frames
		template <typename WriterFn>
		auto write_aligned(
			row_t row, 
			frame_t frame_beg, 
			frame_t frames_to_write, 
			frame_t chunk_size, 
			WriterFn&& writer) -> void;
		// The range specified by frame_beg/frames_to_read
		// must fall entirely within a single sub-buffer
		template <typename ReaderFn>
		auto read_sub_buffer(
			row_t row, 
			frame_t frame_beg, 
			frame_t frames_to_read, 
			ReaderFn&& reader) const -> bool;
		// The range specified by frame_beg/frames_to_write
		// must fall entirely within a single sub-buffer
		template <typename WriterFn>
		auto write_sub_buffer(
			row_t row, 
			frame_t frame_beg, 
			frame_t frames_to_write, 
			WriterFn&& writer) -> bool;
		// Optionally call this after we finish writing
		// audio data to the buffer
		//
		// Preferably call this once per audio callback
		//
		// This will write the top-level mipmap data
		// for all dirty regions
		auto write_mipmap_data() -> void; 
	private: 
		auto get_buffer(frame_t frame) -> Buffer*;
		auto get_buffer(frame_t frame) const -> const Buffer*; 
		static auto get_local_frame(frame_t frame) -> frame_t; 
		HaroldBuffer* const SELF;
		bool buffer_dirt_flag_{};
	} audio { this }; 
	// Non-realtime thread should only access
	// the buffer through this interface
	struct NonRealtimeAccess {
		NonRealtimeAccess(HaroldBuffer* self, frame_t required_size); 
		// The required size requested by the client
		auto get_size() const { return size_; } 
		// The total capacity of the underlying sub buffers
		auto get_actual_size() const { return SELF->actual_size_; } 
		// Total number of sub buffers
		auto get_buffer_count() const { return SELF->critical_.buffers.size(); } 
		// Total number of sub buffers which have been allocated
		auto get_allocated_buffers() const { return allocated_buffers_; } 
		// Clear the visual mipmap data. This is fast because it
		// just marks out a dirty region
		auto clear_mipmap() -> void; 
		// True if all sub buffers have been allocated
		auto is_ready() const -> bool; 
		// Keep calling this in the non-realtime thread to allocate sub
		// buffers a little bit at a time.
		//
		// returns false when all sub buffers have been allocated
		//
		// Allocation progress (e.g. for displaying progress
		// bars) can be calculated as:
		//
		//	float(get_allocated_buffers()) / get_buffer_count()
		auto allocate_buffers() -> bool;
		// Call this continuously in the non-realtime thread if you know
		// the buffer is visible and changing. It doesn't do
		// anything if the top-level mipmap data hasn't changed.
		auto generate_mipmaps() -> bool;
		// We never acquire new sub buffers, so resize() returns
		// false if there are not enough sub buffers to support
		// the requested size (in which case you should throw
		// this Harold buffer away and create a new one of the
		// required size
		auto resize(frame_t required_size) -> bool; 
		// Read final generated mipmap data
		auto read_mipmap(row_t row, float frame, float bin_size) const -> snd::mipmap::frame<>; 
		// We are not going to do any synchronization here
		// for the client.
		//
		// Calling this while the audio thread is writing
		// is ok as long as the audio thread is writing to
		// a different part of the buffer.
		//
		// It is the client's responsibility to coordinate
		// this
		auto SCARY__read(row_t row, frame_t frame) const -> float;
		template <typename ReaderFn, typename ChunkNotReadyFn>
		auto SCARY__read_aligned(
			row_t row, 
			frame_t frame_beg, 
			frame_t frames_to_read, 
			frame_t chunk_size, 
			ReaderFn&& reader,
			ChunkNotReadyFn&& chunk_not_ready) const -> void;
	private: 
		HaroldBuffer* const SELF;
		int allocated_buffers_{};
		frame_t size_{};
	} non_realtime; 
private: 
	std::shared_ptr<buffer_pool_t> buffer_pool_;
	frame_t actual_size_{}; 
	auto acquire_buffers(row_t row_count) -> void; 
	struct {
		std::vector<Buffer> buffers;
	} critical_;
	friend struct AudioAccess;
	friend struct NonRealtimeAccess;
};

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::HaroldBuffer(std::shared_ptr<buffer_pool_t> buffer_pool, row_t row_count, frame_t required_size)
	: non_realtime { this, required_size }
	, buffer_pool_{ buffer_pool }
{
	acquire_buffers(row_count);
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::~HaroldBuffer() {
	for (auto& buffer : critical_.buffers) {
		if (!buffer.ptr->is_ready()) continue; 
		buffer_pool_->release(std::move(buffer.ptr));
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::acquire_buffers(row_t row_count) -> void {
	while (actual_size_ < non_realtime.get_size()) {
		Buffer buffer{ buffer_pool_->acquire(row_count) }; 
		critical_.buffers.push_back(std::move(buffer)); 
		actual_size_ += SUB_BUFFER_SIZE;
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Audio thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::get_local_frame(frame_t frame) -> frame_t {
	return frame % SUB_BUFFER_SIZE;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::get_buffer(frame_t frame) -> Buffer* {
	if (frame >= SELF->actual_size_) return {}; 
	const auto buffer_index{ frame / SUB_BUFFER_SIZE }; 
	assert(buffer_index < SELF->critical_.buffers.size()); 
	auto& buffer{ SELF->critical_.buffers[buffer_index] }; 
	if (!buffer.ptr->is_ready()) return {}; 
	return &buffer;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::get_buffer(frame_t frame) const -> const Buffer* {
	return const_cast<HaroldBuffer::AudioAccess*>(this)->get_buffer(frame);
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::read(row_t row, frame_t frame) const -> float {
	auto buffer{ get_buffer(frame) }; 
	if (!buffer) return 0.0f; 
	return buffer->ptr->audio.read(row, get_local_frame(frame));
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
template <typename ReaderFn, typename ChunkNotReadyFn>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::read_aligned(
	row_t row, 
	frame_t frame_beg, 
	frame_t frames_to_read, 
	frame_t chunk_size,
	ReaderFn&& reader,
	ChunkNotReadyFn&& chunk_not_ready) const -> void
{
	auto frames_remaining{ frames_to_read }; 
	while (frames_remaining > 0) {
		const auto chunk_frames_to_read{ std::min(frames_remaining, chunk_size) }; 
		if (!read_sub_buffer(row, frame_beg, chunk_frames_to_read, reader)) {
			chunk_not_ready();
		} 
		frame_beg += chunk_size;
		frames_remaining -= chunk_frames_to_read;
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::write(row_t row, frame_t frame, float value) -> void {
	auto buffer{ get_buffer(frame) }; 
	if (!buffer) return; 
	buffer->ptr->audio.write(row, get_local_frame(frame), value);
	buffer->dirty = true;
	buffer_dirt_flag_ = true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
template <typename ReaderFn>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::read_sub_buffer(
	row_t row, 
	frame_t frame_beg, 
	frame_t frames_to_read, 
	ReaderFn&& reader) const -> bool
{
	assert((frame_beg / SUB_BUFFER_SIZE) == ((frame_beg + (frames_to_read - 1)) / SUB_BUFFER_SIZE)); 
	auto buffer{ get_buffer(frame_beg) }; 
	if (!buffer) return false; 
	buffer->ptr->audio.read(row, get_local_frame(frame_beg), frames_to_read, reader); 
	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
template <typename WriterFn>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::write_sub_buffer(
	row_t row, 
	frame_t frame_beg, 
	frame_t frames_to_write, 
	WriterFn&& writer) -> bool
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
template <typename WriterFn>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::write_aligned(
	row_t row, 
	frame_t frame_beg, 
	frame_t frames_to_write, 
	frame_t chunk_size, 
	WriterFn&& writer) -> void
{
	auto frames_remaining{ frames_to_write }; 
	while (frames_remaining > 0) {
		const auto chunk_frames_to_write{ std::min(frames_remaining, chunk_size) }; 
		write_sub_buffer(row, frame_beg, chunk_frames_to_write, writer); 
		frame_beg += chunk_size;
		frames_remaining -= chunk_frames_to_write;
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::AudioAccess::write_mipmap_data() -> void {
	if (!buffer_dirt_flag_) return; 
	bool still_dirty{ false }; 
	for (auto& buffer : SELF->critical_.buffers) {
		if (buffer.dirty) {
			if (buffer.ptr->audio.process_mipmap()) {
				buffer.dirty = false;
			}
			else {
				still_dirty = true;
			}
		}
	} 
	if (!still_dirty) {
		buffer_dirt_flag_ = false;
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Non-realtime thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::NonRealtimeAccess(HaroldBuffer* self, frame_t required_size)
	: SELF{ self }
	, size_{ required_size }
{
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::clear_mipmap() -> void {
	for (auto& buffer : SELF->critical_.buffers) {
		buffer.ptr->non_realtime.clear_mipmap();
	}
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::is_ready() const -> bool {
	return allocated_buffers_ >= SELF->critical_.buffers.size();
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::resize(frame_t required_size) -> bool {
	if (required_size > SELF->actual_size_) return false; 
	size_ = required_size; 
	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::allocate_buffers() -> bool {
	if (allocated_buffers_ >= SELF->critical_.buffers.size()) {
		return false;
	} 
	// StanleyBuffer::allocate() does not necessarily need to
	// allocate actual memory because it may be being re-used
	// in which case it is already ready to go.
	//
	// We try to do exactly ALLOC_SIZE actual memory allocations
	// here so that this method takes more or less the same
	// amount of time each time it is called
	const auto unallocated_buffers = SELF->critical_.buffers.size() - allocated_buffers_;
	auto remaining                 = std::min(unallocated_buffers, ALLOC_SIZE);
	while (remaining > 0) {
		if (allocated_buffers_ >= SELF->critical_.buffers.size()) return true; 
		const auto& buffer{ SELF->critical_.buffers[allocated_buffers_++] }; 
		// Returns false if no actual memory was allocated
		if (buffer.ptr->non_realtime.allocate()) {
			remaining--;
		}
	} 
	return true;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::generate_mipmaps() -> bool {
	bool mipmap_generated{}; 
	for (const auto& buffer : SELF->critical_.buffers) {
		auto result{ buffer.ptr->non_realtime.process_mipmap() }; 
		if (result) mipmap_generated = true;
	} 
	return mipmap_generated;
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::read_mipmap(row_t row, float frame, float bin_size) const -> snd::mipmap::frame<> {
	const auto index_a = static_cast<frame_t>(std::floor(frame));
	const auto index_b = static_cast<frame_t>(std::ceil(frame));
	const auto t       = frame - index_a;
	const auto buffer_index_a = index_a / SUB_BUFFER_SIZE;
	const auto buffer_index_b = index_b / SUB_BUFFER_SIZE;
	assert(buffer_index_a < SELF->critical_.buffers.size());
	assert(buffer_index_b < SELF->critical_.buffers.size()); 
	const auto local_frame_a = index_a % SUB_BUFFER_SIZE;
	const auto local_frame_b = index_b % SUB_BUFFER_SIZE ;
	auto& buffer_a = *SELF->critical_.buffers[buffer_index_a].ptr;
	auto& buffer_b = *SELF->critical_.buffers[buffer_index_b].ptr ;
	const auto frame_a = buffer_a.non_realtime.read_mipmap(row, local_frame_a, bin_size);
	const auto frame_b = buffer_b.non_realtime.read_mipmap(row, local_frame_b, bin_size);
	return snd::mipmap::lerp(frame_a, frame_b, t);
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::SCARY__read(row_t row, frame_t frame) const -> float {
	return SELF->audio.read(row, frame);
}

template <size_t SUB_BUFFER_SIZE, size_t ALLOC_SIZE, class Allocator>
template <typename ReaderFn, typename ChunkNotReadyFn>
auto HaroldBuffer<SUB_BUFFER_SIZE, ALLOC_SIZE, Allocator>::NonRealtimeAccess::SCARY__read_aligned(
	row_t row, 
	frame_t frame_beg, 
	frame_t frames_to_read, 
	frame_t chunk_size, 
	ReaderFn&& reader,
	ChunkNotReadyFn&& chunk_not_ready) const -> void
{
	return SELF->audio.read_aligned(row, frame_beg, frames_to_read, chunk_size, reader, chunk_not_ready);
}

} // snd
