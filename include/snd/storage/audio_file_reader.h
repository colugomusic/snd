#pragma once

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

	struct Format
	{
		ChannelCount num_channels = 0;
		FrameCount num_frames = 0;
		SampleRate sample_rate = 0;
		BitDepth bit_depth = 0;
	};

	AudioFileReader(const std::string& utf8_path, AudioFileFormat format_hint = AudioFileFormat::None);

	// Read from memory
	AudioFileReader(const void* data, std::size_t data_size, AudioFileFormat format_hint = AudioFileFormat::None);

	void read_header();
	void read_frames(Callbacks callbacks, std::uint32_t chunk_size);

	ChannelCount get_num_channels() const { return format_.num_channels; }
	FrameCount get_num_frames() const { return format_.num_frames; }
	SampleRate get_sample_rate() const { return format_.sample_rate; }
	BitDepth get_bit_depth() const { return format_.bit_depth; }
	AudioFileFormat get_format() const { return active_format_handler_.format; }

private:

	struct FormatHandler
	{
		using TryReadHeaderFunc = std::function<bool()>;
		using ReadFramesFunc = std::function<void(Callbacks, std::uint32_t)>;

		AudioFileFormat format = AudioFileFormat::None;

		TryReadHeaderFunc try_read_header;
		ReadFramesFunc read_frames;

		operator bool() const { return format != AudioFileFormat::None; }
	};

	FormatHandler flac_handler_;
	FormatHandler mp3_handler_;
	FormatHandler wav_handler_;
	FormatHandler wavpack_handler_;
	FormatHandler active_format_handler_;

	FormatHandler make_flac_handler(const std::string& utf8_path);
	FormatHandler make_flac_handler(const void* data, std::size_t data_size);
	FormatHandler make_mp3_handler(const std::string& utf8_path);
	FormatHandler make_mp3_handler(const void* data, std::size_t data_size);
	FormatHandler make_wav_handler(const std::string& utf8_path);
	FormatHandler make_wav_handler(const void* data, std::size_t data_size);
	FormatHandler make_wavpack_handler(const std::string& utf8_path);
	FormatHandler make_wavpack_handler(const void* data, std::size_t data_size);

	AudioFileFormat format_hint_ = AudioFileFormat::None;
	Format format_;

	std::array<FormatHandler, 4> make_format_attempt_order(AudioFileFormat format);
};

}}