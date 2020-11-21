#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <functional>
#include <string>
#include <snd/types.h>
#include "audio_file_format.h"
#include "frame_data.h"

namespace snd {
namespace storage {

class AudioFileReader
{
public:
	
	struct Callbacks
	{
		using ShouldAbortFunc = std::function<bool()>;
		using ReturnChunkFunc = std::function<void(snd::FrameCount frame, const float* data, std::uint32_t size)>;

		ShouldAbortFunc should_abort;
		ReturnChunkFunc return_chunk;
	};

private:

	struct FormatHandler
	{
		using TryReadHeaderFunc = std::function<bool()>;
		using ReadFramesFunc = std::function<void(Callbacks, std::uint32_t)>;

		AudioFileFormat format = AudioFileFormat::None;
		TryReadHeaderFunc try_read_header;
		ReadFramesFunc read_frames;
	};

	FormatHandler flac_handler_;
	FormatHandler mp3_handler_;
	FormatHandler wav_handler_;
	FormatHandler wavpack_handler_;
	FormatHandler active_format_handler_;

	FormatHandler make_flac_handler();
	FormatHandler make_mp3_handler();
	FormatHandler make_wav_handler();
	FormatHandler make_wavpack_handler();

	std::string utf8_path_;
	AudioFileFormat format_hint_ = AudioFileFormat::None;
	ChannelCount num_channels_ = 0;
	FrameCount num_frames_ = 0;
	SampleRate sample_rate_ = 0;
	BitDepth bit_depth_ = 0;

	std::array<FormatHandler, 4> make_format_attempt_order(AudioFileFormat format);

public:

	AudioFileReader(const std::string& utf8_path, AudioFileFormat format_hint = AudioFileFormat::None);

	void read_header();

	void read_frames(Callbacks callbacks, std::uint32_t chunk_size);

	ChannelCount get_num_channels() const { return num_channels_; }
	FrameCount get_num_frames() const { return num_frames_; }
	SampleRate get_sample_rate() const { return sample_rate_; }
	BitDepth get_bit_depth() const { return bit_depth_; }
	AudioFileFormat get_format() const { return active_format_handler_.format; }
};

}}