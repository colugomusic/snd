#pragma once

#include <snd/types.hpp>
#include "frame_data.hpp"

namespace snd {
namespace storage {

template <class Input, class T, class Allocator>
void deinterleave(const Input& src, FrameCount read_frame_begin, FrameCount read_frame_end, FrameData<T, Allocator>& dst, FrameCount write_frame_begin)
{
	const auto num_channels = dst.get_num_channels();

	auto read_frame = read_frame_begin;
	auto write_frame = write_frame_begin;

	while (read_frame != read_frame_end)
	{
		for (ChannelCount c = 0; c < num_channels; c++)
		{
			dst[c][write_frame] = src[(read_frame * num_channels) + c];
		}

		read_frame++;
		write_frame++;
	}
}

template <typename ReadFrameFn, class Output>
void interleave(ReadFrameFn read_frame_fn, ChannelCount num_channels, FrameCount read_frame_begin, FrameCount read_frame_end, Output& dst, FrameCount write_frame_begin)
{
	auto read_frame = read_frame_begin;
	auto write_frame = write_frame_begin;

	while (read_frame != read_frame_end)
	{
		for (ChannelCount c = 0; c < num_channels; c++)
		{
			dst[(write_frame * num_channels) + c] = read_frame_fn(c, read_frame);
		}

		read_frame++;
		write_frame++;
	}
}

}}
