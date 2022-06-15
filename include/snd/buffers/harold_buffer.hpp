#pragma once

//
// required libraries:
//	- cameron314/readerwriterqueue
//	- colugomusic/stupid
//

#include <cassert>
#include <memory>
#include <vector>
#include <snd/buffers/stanley_buffer_pool.hpp>
#include <snd/samples/sample_mipmap.hpp>

namespace snd {

template <size_t SUB_BUFFER_SIZE = STANLEY_BUFFER_DEFAULT_SIZE, class Allocator = std::allocator<float>>
class HaroldBuffer
{
public:

	using buffer_pool_t = StanleyBufferPool<SUB_BUFFER_SIZE, Allocator>;
	using buffer_t = typename buffer_pool_t::buffer_t;
	using channel_t = uint16_t;

	HaroldBuffer(buffer_pool_t* buffer_pool, uint32_t required_size);

	~HaroldBuffer();

	auto clear_mipmap() -> void;
	auto get_size() const { return size_; }
	auto get_actual_size() const { return actual_size_; }
	auto get_buffer_count() const { return buffers_.size(); }
	auto get_allocated_buffers() const { return allocated_buffers_; }
	auto allocate_buffers() -> bool;
	auto generate_mipmaps() -> bool;
	auto is_ready() const -> bool;
	auto resize(uint32_t required_size) -> bool;
	auto read(channel_t channel, uint32_t frame) const -> float;
	auto write(channel_t channel, uint32_t frame, float value) -> void;
	auto write_aligned(channel_t channel, uint32_t frame_beg, uint32_t frames_to_write, uint32_t chunk_size, std::function<void(float* data)> writer) -> void;

	//
	// The range specified by frame_beg/frames_to_write
	// must fall entirely within a single sub-buffer
	//
	auto write_sub_buffer(channel_t channel, uint32_t frame_beg, uint32_t frames_to_write, std::function<void(float* data)> writer) -> void;

	auto read_mipmap(channel_t channel, float frame, float bin_size) const -> snd::SampleMipmap::LODFrame;
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

	auto acquire_buffers() -> void;
	auto get_buffer(uint32_t frame) -> Buffer*;
	auto get_buffer(uint32_t frame) const -> const Buffer*;

	static auto get_local_frame(uint32_t frame) -> uint32_t;

	std::vector<Buffer> buffers_;
	buffer_pool_t* buffer_pool_;
	int allocated_buffers_{};
	bool buffer_dirt_flag_{};
};

template <size_t SUB_BUFFER_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::HaroldBuffer(buffer_pool_t* buffer_pool, uint32_t required_size)
	: size_{ required_size }
	, buffer_pool_{ buffer_pool }
{
	acquire_buffers();
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::~HaroldBuffer()
{
	for (auto& buffer : buffers_)
	{
		buffer_pool_->release(std::move(buffer.ptr));
	}
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::acquire_buffers() -> void
{
	while (actual_size_ < size_)
	{
		Buffer buffer{ buffer_pool_->acquire(2) };

		buffers_.push_back(std::move(buffer));

		actual_size_ += SUB_BUFFER_SIZE;
	}
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::clear_mipmap() -> void
{
	for (auto& buffer : buffers_)
	{
		buffer.ptr->clear_mipmap();
	}
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::is_ready() const -> bool
{
	return allocated_buffers_ >= buffers_.size();
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::resize(uint32_t required_size) -> bool
{
	if (required_size > actual_size_) return false;

	size_ = required_size;

	return true;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::get_local_frame(uint32_t frame) -> uint32_t
{
	return frame % SUB_BUFFER_SIZE;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::get_buffer(uint32_t frame) -> Buffer*
{
	if (frame >= size_) return {};

	const auto buffer_index{ frame / SUB_BUFFER_SIZE };

	assert(buffer_index < buffers_.size());

	auto& buffer{ buffers_[buffer_index] };

	if (!buffer.ptr->is_ready()) return {};

	return &buffer;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::get_buffer(uint32_t frame) const -> const Buffer*
{
	return const_cast<HaroldBuffer*>(this)->get_buffer(frame);
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::read(channel_t channel, uint32_t frame) const -> float
{
	auto buffer{ get_buffer(frame) };

	if (!buffer) return 0.0f;

	return buffer->ptr->read(channel, get_local_frame(frame));
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::write(channel_t channel, uint32_t frame, float value) -> void
{
	auto buffer{ get_buffer(frame) };

	if (!buffer) return;

	buffer->ptr->write(channel, get_local_frame(frame), value);
	buffer->dirty = true;
	buffer_dirt_flag_ = true;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::write_sub_buffer(channel_t channel, uint32_t frame_beg, uint32_t frames_to_write, std::function<void(float* data)> writer) -> void
{
	assert((frame_beg / SUB_BUFFER_SIZE) == ((frame_beg + (frames_to_write - 1)) / SUB_BUFFER_SIZE));

	auto buffer{ get_buffer(frame_beg) };

	if (!buffer) return;

	buffer->ptr->write(channel, frame_beg, frames_to_write, writer);
	buffer->dirty = true;
	buffer_dirt_flag_ = true;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::write_aligned(channel_t channel, uint32_t frame_beg, uint32_t frames_to_write, uint32_t chunk_size, std::function<void(float* data)> writer) -> void
{
	auto frames_remaining{ frames_to_write };

	while (frames_remaining > 0)
	{
		const auto chunk_frames_to_write{ std::min(frames_remaining, chunk_size) };

		write_sub_buffer(channel, frame_beg, chunk_frames_to_write, writer);

		frame_beg += chunk_size;
		frames_remaining -= chunk_frames_to_write;
	}
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::allocate_buffers() -> bool
{
	if (allocated_buffers_ >= buffers_.size())
	{
		return false;
	}

	for (auto i{ allocated_buffers_ }; i < buffers_.size(); i++)
	{
		const auto& buffer{ buffers_[i] };

		allocated_buffers_++;

		// If no actual memory was allocated, continue iterating
		if (buffer.ptr->allocate()) break;
	}

	return true;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::generate_mipmaps() -> bool
{
	bool mipmap_generated{};

	for (const auto& buffer : buffers_)
	{
		auto result{ buffer.ptr->process_mipmap<snd::th::GUI>() };

		if (result) mipmap_generated = true;
	}

	return mipmap_generated;
}

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::write_audio_mipmap_data() -> void
{
	if (!buffer_dirt_flag_) return;

	bool still_dirty{ false };

	for (auto& buffer : buffers_)
	{
		if (buffer.dirty)
		{
			if (buffer.ptr->process_mipmap<snd::th::AUDIO>())
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

template <size_t SUB_BUFFER_SIZE, class Allocator>
auto HaroldBuffer<SUB_BUFFER_SIZE, Allocator>::read_mipmap(channel_t channel, float frame, float bin_size) const -> snd::SampleMipmap::LODFrame
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

	const auto frame_a{ buffer_a.read_mipmap(channel, local_frame_a, bin_size) };
	const auto frame_b{ buffer_b.read_mipmap(channel, local_frame_b, bin_size) };

	return snd::SampleMipmap::LODFrame::lerp(frame_a, frame_b, t);
}

} // snd
