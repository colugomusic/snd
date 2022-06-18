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
class StanleyBuffer
{
	friend class AudioAccess;
	friend class GuiAccess;

	static constexpr auto GUI{ 0 };
	static constexpr auto AUDIO{ 1 };

	template <class T> static constexpr auto is_power_of_two(T x) { return (SIZE & (SIZE -1)) == 0; }

	static_assert(is_power_of_two(SIZE));

	static constexpr snd::SampleMipmap::Region EMPTY_REGION
	{
		std::numeric_limits<uint64_t>::max(),
		std::numeric_limits<uint64_t>::min()
	};

public:

	using row_t = typename DeferredBuffer<float, SIZE>::row_t;
	using frame_t = typename DeferredBuffer<float, SIZE>::index_t;

	//
	// Audio thread can access the buffer through here
	//
	class AudioAccess
	{
	public:

		AudioAccess(StanleyBuffer* self);

		auto read(row_t row, frame_t frame) const -> float;
		auto read(row_t row, frame_t frame_beg, frame_t frame_count, std::function<void(const float*)> reader) const -> bool;
		auto write(row_t row, frame_t frame, float value) -> void;
		auto write(row_t row, frame_t frame_beg, frame_t frame_count, std::function<void(float* value)> writer) -> void;

		// Call after new data has been written
		auto process_mipmap() -> bool;

	private:

		StanleyBuffer* const SELF;
		stupid::BeachBallPlayer<AUDIO> beach_player_;
		snd::SampleMipmap::Region dirty_region_{};
	} audio;

	//
	// GUI thread can access the buffer through here
	//
	class GuiAccess
	{
	public:

		GuiAccess(StanleyBuffer* self);

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

	private:

		StanleyBuffer* const SELF;
		stupid::BeachBallPlayer<GUI> beach_player_;
		std::unique_ptr<snd::SampleMipmap> mipmap_;
	} gui;

	const row_t row_count;

	StanleyBuffer(row_t row_count_);

	auto is_ready() const -> bool;

private:

	struct CriticalSection
	{
		CriticalSection(row_t row_count) : buffer{ row_count } {}

		std::atomic<bool> ready{ false };

		//
		// The actual audio buffer which is read and written by
		// the audio thread.
		//
		// This is accessed once by the GUI thread in gui.allocate(),
		// before the audio thread starts accessing it
		//
		DeferredBuffer<float, SIZE, Allocator> buffer;

		//
		// This region is read and written by the audio thread
		//
		// It is also written by the GUI thread once in gui.release()
		// 
		// The audio thread should have finished accessing this data
		// by the time gui.release() is called
		//
		snd::SampleMipmap::Region readable_region{ EMPTY_REGION };

		//
		// Other synchronization happens via beach ball
		//
		struct
		{
			stupid::BeachBall ball{ AUDIO };

			struct
			{
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
	, gui{ this }
{
	critical_.beach.mipmap.staging_buffers.resize(row_count);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::is_ready() const -> bool
{
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
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::read(row_t row, frame_t frame) const -> float
{
	assert(SELF->is_ready());

	const auto readable_region{ SELF->critical_.readable_region };

	if (readable_region.beg >= readable_region.end) return 0.0f;
	if (frame < readable_region.beg || frame >= readable_region.end) return 0.0f;

	return SELF->critical_.buffer.read(row, frame);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::write(row_t row, frame_t frame, float value) -> void
{
	assert(SELF->is_ready());

	SELF->critical_.buffer.write(row, frame, value);

	if (frame < dirty_region_.beg) dirty_region_.beg = frame;
	if (frame >= dirty_region_.end) dirty_region_.end = frame + 1;
	if (frame < SELF->critical_.readable_region.beg) SELF->critical_.readable_region.beg = frame;
	if (frame >= SELF->critical_.readable_region.end) SELF->critical_.readable_region.end = frame + 1;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::read(row_t row, frame_t frame_beg, frame_t frame_count, std::function<void(const float*)> reader) const -> bool
{
	assert(SELF->is_ready());
	assert(row < SELF->row_count);

	const auto readable_region{ SELF->critical_.readable_region };

	if (readable_region.beg >= readable_region.end) return false;
	if (frame_beg < readable_region.beg || frame_beg + frame_count >= readable_region.end) return false;

	SELF->critical_.buffer.read(row, frame_beg, reader);

	return true;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::write(row_t row, frame_t frame_beg, frame_t frame_count, std::function<void(float* value)> writer) -> void
{
	assert(SELF->is_ready());
	assert(row < SELF->row_count);

	SELF->critical_.buffer.write(row, frame_beg, writer);

	if (frame_beg < dirty_region_.beg) dirty_region_.beg = frame_beg;
	if (frame_beg + frame_count >= dirty_region_.end) dirty_region_.end = frame_beg + frame_count;
	if (frame_beg < SELF->critical_.readable_region.beg) SELF->critical_.readable_region.beg = frame_beg;
	if (frame_beg + frame_count >= SELF->critical_.readable_region.end) SELF->critical_.readable_region.end = frame_beg + frame_count;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::AudioAccess::process_mipmap() -> bool
{
	if (!beach_player_.ensure()) return false;

	if (dirty_region_.beg >= dirty_region_.end) return true;

	const auto num_dirty_frames{ dirty_region_.end - dirty_region_.beg };

	for (row_t row{}; row < SELF->row_count; row++)
	{
		for (frame_t i{}; i < num_dirty_frames; i++)
		{
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
StanleyBuffer<SIZE, Allocator>::GuiAccess::GuiAccess(StanleyBuffer* self)
	: SELF{ self }
	, beach_player_{ &self->critical_.beach.ball }
{
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::GuiAccess::allocate() -> bool
{
	if (!SELF->critical_.buffer.is_ready())
	{
		assert(!mipmap_);

		SELF->critical_.buffer.allocate();

		for (row_t row{}; row < SELF->row_count; row++)
		{
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
auto StanleyBuffer<SIZE, Allocator>::GuiAccess::release() -> void
{
	SELF->critical_.readable_region = EMPTY_REGION;
	SELF->critical_.beach.mipmap.dirty_region = EMPTY_REGION;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::GuiAccess::clear_mipmap() -> void
{
	if (!mipmap_) return;

	mipmap_->clear();
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::GuiAccess::process_mipmap() -> bool
{
	if (!beach_player_.ensure()) return false;

	if (SELF->critical_.beach.mipmap.dirty_region.beg >= SELF->critical_.beach.mipmap.dirty_region.end)
	{
		beach_player_.throw_ball();
		return true;
	}

	const auto num_dirty_frames{ SELF->critical_.beach.mipmap.dirty_region.end - SELF->critical_.beach.mipmap.dirty_region.beg };

	for (row_t row{}; row < SELF->row_count; row++)
	{
		const auto writer = [=](snd::SampleMipmap::Frame* data)
		{
			for (frame_t i{}; i < num_dirty_frames; i++)
			{
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
auto StanleyBuffer<SIZE, Allocator>::GuiAccess::read_mipmap(row_t row, frame_t frame, float bin_size) const -> snd::SampleMipmap::LODFrame
{
	if (!mipmap_) return snd::SampleMipmap::SILENT_FRAME;

	return mipmap_->read(mipmap_->bin_size_to_lod(bin_size), row, float(frame));
}

} // snd
