#pragma once

#include <algorithm>
#include <vector>
#include <snd/types.hpp>
#include <snd/misc.hpp>

namespace snd {
namespace storage {

template <class T, class Allocator = std::allocator<T>>
using ChannelData = std::vector<T, Allocator>;

template <class T, class Allocator = std::allocator<T>>
class FrameData
{
	using Data = std::vector<ChannelData<T, Allocator>>;

	Data data_;
	ChannelCount num_channels_ = 0;
	FrameCount num_frames_ = 0;

public:

	FrameData() = default;
	FrameData(FrameData&&) = default;
	FrameData& operator=(FrameData&&) = default;

	FrameData(ChannelCount num_channels, FrameCount num_frames)
		: data_(num_channels)
		, num_channels_(num_channels)
		, num_frames_(num_frames)
	{
		std::for_each(data_.begin(), data_.end(), [num_frames](ChannelData<T, Allocator>& data) { data.resize(num_frames); });
	}

	bool is_empty() const { return data_.empty(); }

	ChannelCount get_num_channels() const { return num_channels_; }
	FrameCount get_num_frames() const { return num_frames_; }

	ChannelData<T, Allocator>& operator[](ChannelCount channel) { return data_[channel]; }
	const ChannelData<T, Allocator>& operator[](ChannelCount channel) const { return data_[channel]; }

	float read_frame(ChannelCount channel, FrameCount idx)
	{
		if (idx >= num_frames_) return 0.0f;

		return data_[channel][idx];
	}

	float read_frame_interp(ChannelCount channel, float idx)
	{
		const auto idx_floor = std::uint64_t(std::floor(idx));
		const auto idx_ceil = std::uint64_t(std::ceil(idx));
		const auto idx_t = idx - idx_floor;

		const auto value_floor = read_frame(channel, idx_floor);
		const auto value_ceil = read_frame(channel, idx_ceil);
		
		return lerp(value_floor, value_ceil, idx_t);
	}

	typename Data::iterator begin() const { return data_.begin(); }
	typename Data::iterator end() const { return data_.end(); }
};

}}
