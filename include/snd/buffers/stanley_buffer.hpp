#pragma once

//
// required libraries:
//	- colugomusic/stupid
//

#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <vector>
#include <stupid/stupid.hpp>
#include <snd/buffers/deferred_buffer.hpp>
#include <snd/samples/sample_mipmap.hpp>

namespace snd {

static constexpr auto STANLEY_BUFFER_DEFAULT_SIZE{ 1 << 14 };

//
// A buffer of floating point audio data and a mipmap for
// displaying it visually.
// 
// Mipmap data can be simultaneously updated while the
// buffer is being written to
// 
// Memory is not allocated until allocate() is called
//
template <size_t SIZE = STANLEY_BUFFER_DEFAULT_SIZE, class Allocator = ::std::allocator<float>>
struct StanleyBuffer {
private:
	friend struct AudioAccess;
	friend struct NonRealtimeAccess;
	static constexpr auto NON_REALTIME = 0;
	static constexpr auto AUDIO        = 1;
	template <class T> static constexpr auto is_power_of_two(T x) { return (SIZE & (SIZE -1)) == 0; }
	static_assert(is_power_of_two(SIZE));
	static constexpr snd::SampleMipmap::Region EMPTY_REGION {
		std::numeric_limits<uint64_t>::max(),
		std::numeric_limits<uint64_t>::min()
	};
public:
	using row_t = typename DeferredBuffer<float, SIZE>::row_t;
	using frame_t = typename DeferredBuffer<float, SIZE>::index_t;
	// Audio thread can access the buffer through here
	struct AudioAccess {
		AudioAccess(StanleyBuffer* self);
		auto read(row_t row, frame_t frame) const -> float;
		template <typename ReaderFn>
		auto read(row_t row, frame_t frame_beg, frame_t frame_count, ReaderFn&& reader) const -> void;
		auto write(row_t row, frame_t frame, float value) -> void;
		template <typename WriterFn>
		auto write(row_t row, frame_t frame_beg, frame_t frame_count, WriterFn&& writer) -> void;
		// Call after new data has been written
		auto process_mipmap() -> bool;
	private:
		StanleyBuffer* const SELF;
		stupid::beach_ball_player<AUDIO> beach_player_;
		snd::SampleMipmap::Region dirty_region_{};
	} audio;
	// Non-realtime thread can access the buffer through here
	struct NonRealtimeAccess {
		NonRealtimeAccess(StanleyBuffer* self);
		// May not actually need to allocate memory, but will
		// always set 'ready' state to true
		// Returns true if memory was actually allocated
		// Or false if no new memory was allocated
		auto allocate() -> bool;
		// Doesn't free memory
		auto release() -> void;
		auto clear_mipmap() -> void;
		auto read_mipmap(row_t row, frame_t frame, float bin_size) const -> snd::SampleMipmap::LODFrame;
		// Call if the buffer is visible and updating.
		// If audio data didn't change then this does nothing
		auto process_mipmap() -> bool;
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
		auto SCARY__read(row_t row, frame_t frame_beg, frame_t frame_count, std::function<void(const float*)> reader) const -> void;
	private:
		StanleyBuffer* const SELF;
		stupid::beach_ball_player<NON_REALTIME> beach_player_;
		std::unique_ptr<snd::SampleMipmap> mipmap_;
	} non_realtime;
	const row_t row_count;
	StanleyBuffer(row_t row_count_);
	auto is_ready() const -> bool;
private:
	struct CriticalSection {
		CriticalSection(row_t row_count) : buffer{ row_count } {}
		std::atomic<bool> ready{ false };
		// The actual audio buffer which is read and written by
		// the audio thread.
		//
		// This is accessed once by the GUI thread in non_realtime.allocate(),
		// before the audio thread starts accessing it
		DeferredBuffer<float, SIZE, Allocator> buffer;
		// Other synchronization happens via beach ball
		struct {
			stupid::beach_ball ball{ AUDIO };
			struct {
				std::vector<std::vector<snd::SampleMipmap::Frame>> staging_buffers;
				snd::SampleMipmap::Region dirty_region{};
			} mipmap;
		} beach;
	} critical_{ row_count };
};

template <size_t SIZE, class Allocator>
StanleyBuffer<SIZE, Allocator>::StanleyBuffer(row_t row_count_)
	: row_count{ row_count_ }
	, audio{ this }
	, non_realtime{ this }
{
	critical_.beach.mipmap.staging_buffers.resize(row_count);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::is_ready() const -> bool {
	return critical_.ready.load(std::memory_order_acquire);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Audio thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <size_t SIZE, class Allocator>
StanleyBuffer<SIZE, Allocator>::AudioAccess::AudioAccess(StanleyBuffer* self)
	: SELF{ self }
	, beach_player_{ &self->critical_.beach.ball }
{
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::read(row_t row, frame_t frame) const -> float {
	assert(SELF->is_ready()); 
	return SELF->critical_.buffer.read(row, frame);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::write(row_t row, frame_t frame, float value) -> void {
	assert(SELF->is_ready()); 
	SELF->critical_.buffer.write(row, frame, value); 
	if (frame < dirty_region_.beg) dirty_region_.beg = frame;
	if (frame >= dirty_region_.end) dirty_region_.end = frame + 1;
}

template <size_t SIZE, class Allocator>
template <typename ReaderFn>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::read(row_t row, frame_t frame_beg, frame_t frame_count, ReaderFn&& reader) const -> void {
	assert(SELF->is_ready());
	assert(row < SELF->row_count); 
	SELF->critical_.buffer.read(row, frame_beg, std::move(reader));
}

template <size_t SIZE, class Allocator>
template <typename WriterFn>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::write(row_t row, frame_t frame_beg, frame_t frame_count, WriterFn&& writer) -> void {
	assert(SELF->is_ready());
	assert(row < SELF->row_count); 
	SELF->critical_.buffer.write(row, frame_beg, std::move(writer)); 
	if (frame_beg < dirty_region_.beg) dirty_region_.beg = frame_beg;
	if (frame_beg + frame_count >= dirty_region_.end) dirty_region_.end = frame_beg + frame_count;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::process_mipmap() -> bool {
	if (!beach_player_.ensure()) return false; 
	if (dirty_region_.beg >= dirty_region_.end) return true; 
	const auto num_dirty_frames = dirty_region_.end - dirty_region_.beg;
	for (row_t row{}; row < SELF->row_count; row++) {
		for (frame_t i{}; i < num_dirty_frames; i++) {
			SELF->critical_.beach.mipmap.staging_buffers[row][dirty_region_.beg + i] =
				snd::SampleMipmap::encode(SELF->critical_.buffer.read(row, i + dirty_region_.beg));
		}
	} 
	SELF->critical_.beach.mipmap.dirty_region = dirty_region_;
	dirty_region_ = EMPTY_REGION; 
	beach_player_.throw_ball(); 
	return true;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// GUI thread
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <size_t SIZE, class Allocator>
StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::NonRealtimeAccess(StanleyBuffer* self)
	: SELF{ self }
	, beach_player_{ &self->critical_.beach.ball }
{
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::allocate() -> bool {
	if (!SELF->critical_.buffer.is_ready()) {
		assert(!mipmap_); 
		SELF->critical_.buffer.allocate(); 
		for (row_t row{}; row < SELF->row_count; row++) {
			SELF->critical_.buffer.fill(row, 0.0f);
			SELF->critical_.beach.mipmap.staging_buffers[row].resize(SIZE);
		} 
		mipmap_ = std::make_unique<snd::SampleMipmap>(SELF->row_count, SIZE); 
		SELF->critical_.ready.store(true, std::memory_order_release);
		return true;
	} 
	SELF->critical_.ready.store(true, std::memory_order_relaxed);
	return false;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::release() -> void {
	for (row_t row{}; row < SELF->row_count; row++) {
		SELF->critical_.buffer.fill(row, 0.0f);
	} 
	SELF->critical_.beach.mipmap.dirty_region = EMPTY_REGION;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::clear_mipmap() -> void {
	if (!mipmap_) return; 
	mipmap_->clear();
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::process_mipmap() -> bool {
	if (!beach_player_.ensure()) return false; 
	if (SELF->critical_.beach.mipmap.dirty_region.beg >= SELF->critical_.beach.mipmap.dirty_region.end) {
		beach_player_.throw_ball();
		return true;
	} 
	const auto num_dirty_frames = SELF->critical_.beach.mipmap.dirty_region.end - SELF->critical_.beach.mipmap.dirty_region.beg; 
	for (row_t row{}; row < SELF->row_count; row++) {
		const auto writer = [=](snd::SampleMipmap::Frame* data) {
			for (frame_t i{}; i < num_dirty_frames; i++) {
				data[i] = SELF->critical_.beach.mipmap.staging_buffers[row][i + SELF->critical_.beach.mipmap.dirty_region.beg];
			}
		}; 
		mipmap_->write(row, SELF->critical_.beach.mipmap.dirty_region.beg, writer);
	} 
	mipmap_->update(SELF->critical_.beach.mipmap.dirty_region); 
	SELF->critical_.beach.mipmap.dirty_region = EMPTY_REGION; 
	beach_player_.throw_ball(); 
	return true;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::read_mipmap(row_t row, frame_t frame, float bin_size) const -> snd::SampleMipmap::LODFrame {
	if (!mipmap_) return snd::SampleMipmap::SILENT_FRAME; 
	return mipmap_->read(mipmap_->bin_size_to_lod(bin_size), row, float(frame));
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::SCARY__read(row_t row, frame_t frame) const -> float {
	return audio_access_->read(row, frame);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::NonRealtimeAccess::SCARY__read(row_t row, frame_t frame_beg, frame_t frame_count, std::function<void(const float*)> reader) const -> void {
	return audio_access_->read(row, frame_beg, frame_count, reader);
}

} // snd
