#pragma once

#include <functional>
#include <string>
#include "types.h"

struct WavpackContext;

namespace snd {
namespace storage {
namespace wavpack {

class Reader
{
	std::string path_;

	WavpackContext* context_ = nullptr;
	ChannelCount num_channels_ = 0;
	FrameCount num_frames_ = 0;
	SampleRate sample_rate_ = 0;
	BitDepth bit_depth_ = 0;
	std::function<std::uint32_t(float* buffer, std::uint32_t read_size)> chunk_reader_;

public:

	struct Callbacks
	{
		using ShouldAbortFunc = std::function<bool()>;
		using ReturnChunkFunc = std::function<void(snd::FrameCount frame, const float* data, std::uint32_t size)>;

		ShouldAbortFunc should_abort;
		ReturnChunkFunc return_chunk;
	};

	Reader(const std::string& utf8_path);
	~Reader();

	ChannelCount get_num_channels() const { return num_channels_; }
	FrameCount get_num_frames() const { return num_frames_; }
	SampleRate get_sample_rate() const { return sample_rate_; }
	BitDepth get_bit_depth() const { return bit_depth_; }

	bool try_read_header();
	void read_frames(Callbacks callbacks, std::uint32_t chunk_size);
};

}}}