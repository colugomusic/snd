#pragma once

//
// required libraries:
//	- colugomusic/stupid
//

#include <array>
#include <atomic>
#include <cassert>
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
template <size_t SIZE = STANLEY_BUFFER_DEFAULT_SIZE, class Allocator = std::allocator<float>>
class StanleyBuffer
{
public:

	template <class T> static constexpr auto is_power_of_two(T x) { return (SIZE & (SIZE -1)) == 0; }

	static_assert(is_power_of_two(SIZE));

	using row_t = typename DeferredBuffer<float, SIZE>::row_t;

	StanleyBuffer(row_t row_count);

	// May not actually need to allocate memory, but will
	// always set 'ready' state to true
	// Returns true if memory was actually allocated
	// Or false if no new memory was allocated
	auto allocate() -> bool;

	// Doesn't free memory
	auto release() -> void;

	auto clear_mipmap() -> void;
	auto get_row_count() const { return row_count_; }
	auto is_ready() const -> bool;
	auto read(row_t row, uint32_t frame) const -> float;
	auto read(row_t row, uint32_t frame_beg, uint32_t frame_count, std::function<void(const float*)> reader) const -> bool;
	auto write(row_t row, uint32_t frame, float value) -> void;
	auto write(row_t row, uint32_t frame_beg, uint32_t frame_count, std::function<void(float* value)> writer) -> void;

	auto read_mipmap(row_t row, uint64_t frame, float bin_size) const -> snd::SampleMipmap::LODFrame;

	// Call in the audio thread after new data has been written
	auto process_mipmap_audio() -> bool;

	// Call in the GUI thread if the buffer is visible and updating.
	// If audio data didn't change then this does nothing
	auto process_mipmap_gui() -> bool;

private:

	static constexpr snd::SampleMipmap::Region EMPTY_REGION
	{
		std::numeric_limits<uint64_t>::max(),
		std::numeric_limits<uint64_t>::min()
	};

	std::atomic<bool> ready_;
	row_t row_count_;

	struct
	{
		std::unique_ptr<snd::SampleMipmap> mipmap;
		snd::SampleMipmap::Region valid_region{ EMPTY_REGION };
	} gui_;

	struct Audio
	{
		Audio(row_t row_count);

		DeferredBuffer<float, SIZE, Allocator> buffer;
		snd::SampleMipmap::Region dirty_region{};
		snd::SampleMipmap::Region valid_region{ EMPTY_REGION };
	} audio_;

	static constexpr auto GUI{ 0 };
	static constexpr auto AUDIO{ 1 };

	struct
	{
		stupid::BeachBall ball{ AUDIO };
		stupid::BeachBallPlayer<GUI> gui{ &ball };
		stupid::BeachBallPlayer<AUDIO> audio{ &ball };

		struct
		{
			std::vector<std::vector<snd::SampleMipmap::Frame>> staging_buffers;
			snd::SampleMipmap::Region dirty_region{};
		} mipmap;
	} beach_;
};

template <size_t SIZE, class Allocator>
StanleyBuffer<SIZE, Allocator>::Audio::Audio(row_t row_count)
	: buffer{ row_count }
{
}

template <size_t SIZE, class Allocator>
StanleyBuffer<SIZE, Allocator>::StanleyBuffer(row_t row_count)
	: row_count_{ row_count }
	, audio_ { row_count }
{
	beach_.mipmap.staging_buffers.resize(row_count_);
	ready_.store(false, std::memory_order_relaxed);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::allocate() -> bool
{
	if (!audio_.buffer.is_ready())
	{
		assert(!gui_.mipmap);

		audio_.buffer.allocate();

		for (row_t row{}; row < row_count_; row++)
		{
			beach_.mipmap.staging_buffers[row].resize(SIZE);
		}

		gui_.mipmap = std::make_unique<snd::SampleMipmap>(row_count_, SIZE);

		ready_.store(true, std::memory_order_release);
		return true;
	}

	ready_.store(true, std::memory_order_relaxed);
	return false;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::release() -> void
{
	audio_.valid_region = EMPTY_REGION;
	gui_.valid_region = EMPTY_REGION;
	beach_.mipmap.dirty_region = EMPTY_REGION;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::clear_mipmap() -> void
{
	gui_.valid_region = EMPTY_REGION;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::is_ready() const -> bool
{
	return ready_.load(std::memory_order_acquire);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::read(row_t row, uint32_t frame) const -> float
{
	assert(is_ready());

	if (audio_.valid_region.beg >= audio_.valid_region.end) return 0.0f;
	if (frame < audio_.valid_region.beg || frame >= audio_.valid_region.end) return 0.0f;

	return audio_.buffer.read(row, frame);
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::write(row_t row, uint32_t frame, float value) -> void
{
	assert(is_ready());

	audio_.buffer.write(row, frame, value);

	if (frame < audio_.dirty_region.beg) audio_.dirty_region.beg = frame;
	if (frame >= audio_.dirty_region.end) audio_.dirty_region.end = frame + 1;
	if (frame < audio_.valid_region.beg) audio_.valid_region.beg = frame;
	if (frame >= audio_.valid_region.end) audio_.valid_region.end = frame + 1;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::read(row_t row, uint32_t frame_beg, uint32_t frame_count, std::function<void(const float*)> reader) const -> bool
{
	assert(is_ready());
	assert(row < row_count_);

	if (audio_.valid_region.beg >= audio_.valid_region.end) return false;
	if (frame_beg < audio_.valid_region.beg || frame_beg + frame_count >= audio_.valid_region.end) return false;

	audio_.buffer.read(row, frame_beg, reader);

	return true;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::write(row_t row, uint32_t frame_beg, uint32_t frame_count, std::function<void(float* value)> writer) -> void
{
	assert(is_ready());
	assert(row < row_count_);

	audio_.buffer.write(row, frame_beg, writer);

	if (frame_beg < audio_.dirty_region.beg) audio_.dirty_region.beg = frame_beg;
	if (frame_beg + frame_count >= audio_.dirty_region.end) audio_.dirty_region.end = frame_beg + frame_count;
	if (frame_beg < audio_.valid_region.beg) audio_.valid_region.beg = frame_beg;
	if (frame_beg + frame_count >= audio_.valid_region.end) audio_.valid_region.end = frame_beg + frame_count;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::read_mipmap(row_t row, uint64_t frame, float bin_size) const -> snd::SampleMipmap::LODFrame
{
	static constexpr snd::SampleMipmap::LODFrame SILENT{ snd::SampleMipmap::SILENT, snd::SampleMipmap::SILENT };

	if (!gui_.mipmap) return SILENT;
	if (gui_.valid_region.beg >= gui_.valid_region.end) return SILENT;
	if (frame < gui_.valid_region.beg || frame >= gui_.valid_region.end) return SILENT;

	return gui_.mipmap->read(gui_.mipmap->bin_size_to_lod(bin_size), row, float(frame));
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::process_mipmap_audio() -> bool
{
	if (!beach_.audio.ensure()) return false;

	if (audio_.dirty_region.beg >= audio_.dirty_region.end) return true;

	const auto num_dirty_frames{ audio_.dirty_region.end - audio_.dirty_region.beg };

	for (uint32_t row{}; row < row_count_; row++)
	{
		for (uint64_t i{}; i < num_dirty_frames; i++)
		{
			beach_.mipmap.staging_buffers[row][audio_.dirty_region.beg + i] =
				snd::SampleMipmap::encode(audio_.buffer.read(row, i + audio_.dirty_region.beg));
		}
	}

	beach_.mipmap.dirty_region = audio_.dirty_region;
	audio_.dirty_region = EMPTY_REGION;

	beach_.audio.throw_ball();

	return true;
}

template <size_t SIZE, class Allocator>
auto StanleyBuffer<SIZE, Allocator>::process_mipmap_gui() -> bool
{
	if (!beach_.gui.ensure()) return false;

	if (beach_.mipmap.dirty_region.beg >= beach_.mipmap.dirty_region.end)
	{
		beach_.gui.throw_ball();
		return true;
	}

	const auto num_dirty_frames{ beach_.mipmap.dirty_region.end - beach_.mipmap.dirty_region.beg };

	for (uint32_t row{}; row < row_count_; row++)
	{
		const auto writer = [=](snd::SampleMipmap::Frame* data)
		{
			for (uint64_t i{}; i < num_dirty_frames; i++)
			{
				data[i] = beach_.mipmap.staging_buffers[row][i + beach_.mipmap.dirty_region.beg];
			}
		};

		gui_.mipmap->write(row, beach_.mipmap.dirty_region.beg, writer);
	}

	if (beach_.mipmap.dirty_region.beg < gui_.valid_region.beg) gui_.valid_region.beg = beach_.mipmap.dirty_region.beg;
	if (beach_.mipmap.dirty_region.end > gui_.valid_region.end) gui_.valid_region.end = beach_.mipmap.dirty_region.end;

	gui_.mipmap->update(beach_.mipmap.dirty_region);

	beach_.mipmap.dirty_region = EMPTY_REGION;

	beach_.gui.throw_ball();

	return true;
}

} // snd
