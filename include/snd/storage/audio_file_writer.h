#pragma once

#include <functional>
#include <string>
#include <snd/types.h>
#include "audio_file_format.h"
#include "frame_data.h"

namespace snd {
namespace storage {

class AudioFileWriter
{
public:

	struct Callbacks
	{
		using ShouldAbortFunc = std::function<bool()>;
		using ReportProgressFunc = std::function<void(snd::FrameCount frames_done)>;
		using GetNextChunkFunc = std::function<void(float*, FrameCount, std::uint32_t)>;

		ShouldAbortFunc should_abort;
		ReportProgressFunc report_progress;
		GetNextChunkFunc get_next_chunk;
	};

	struct Format
	{
		AudioFileFormat format = AudioFileFormat::WAV;
		ChannelCount num_channels = 0;
		FrameCount num_frames = 0;
		SampleRate sample_rate = 0;
		BitDepth bit_depth = 0;
	};

	struct StreamWriter
	{
		enum class SeekOrigin { Start, Current };

		using SeekFunc = std::function<bool(SeekOrigin, std::int64_t)>;
		using WriteBytesFunc = std::function<std::uint64_t(const void*, std::uint64_t)>;

		SeekFunc seek;
		WriteBytesFunc write_bytes;

		operator bool() const { return seek && write_bytes; }
	};

	AudioFileWriter(const std::string& utf8_path, const Format& format);
	AudioFileWriter(const StreamWriter& stream, const Format& format);

	void write(Callbacks callbacks, std::uint32_t chunk_size);

private:

	struct FormatHandler
	{
		using WriteFunc = std::function<void(Callbacks, std::uint32_t)>;

		WriteFunc write;
	};

	FormatHandler format_handler_;

	FormatHandler make_wav_handler(const StreamWriter& stream, const Format& format);
	FormatHandler make_wav_handler(const std::string& utf8_path, const Format& format);
	FormatHandler make_wavpack_handler(const StreamWriter& stream, const Format& format);
	FormatHandler make_wavpack_handler(const std::string& utf8_path, const Format& format);
	FormatHandler make_format_handler(const StreamWriter& stream, const Format& format);
	FormatHandler make_format_handler(const std::string& utf8_path, const Format& format);
};

}}