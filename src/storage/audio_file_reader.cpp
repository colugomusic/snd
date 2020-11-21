#include "storage/audio_file_reader.h"
#include <array>
#include <stdexcept>
#define DR_FLAC_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#include <dr_flac.h>
#include <dr_wav.h>
#include <dr_mp3.h>
#include <snd/storage/interleaving.h>
#include "wavpack/wavpack_reader.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include <utf8.h>

namespace snd {
namespace storage {

static void generic_dr_libs_frame_reader_loop(
	AudioFileReader::Callbacks callbacks,
	std::function<bool(float*, std::uint32_t)> read_func,
	std::uint32_t chunk_size,
	ChannelCount num_channels,
	FrameCount num_frames)
{
	FrameCount frame = 0;

	while (frame < num_frames)
	{
		if (callbacks.should_abort()) break;

		std::vector<float> interleaved_frames;

		auto read_size = chunk_size;

		if (frame + read_size >= num_frames)
		{
			read_size = num_frames - frame;
		}

		interleaved_frames.resize(size_t(read_size) * num_channels);

		if (!read_func(interleaved_frames.data(), read_size)) throw std::runtime_error("Read error");

		callbacks.return_chunk(frame, interleaved_frames.data(), read_size);

		frame += read_size;
	}
}

auto AudioFileReader::make_flac_handler() -> FormatHandler
{
	const auto open_file =
	#ifdef _WIN32
		[](const std::string& utf8_path)
		{
			return drflac_open_file_w((const wchar_t*)(utf8::utf8to16(utf8_path).c_str()), nullptr);
		};
	#else
		[](const std::string& utf8_path)
		{
			return drflac_open_file(utf8_path.c_str(), nullptr);
		};
	#endif

	const auto try_read_header = [this, open_file]() -> bool
	{
		auto flac = open_file(utf8_path_);

		if (!flac) return false;

		num_channels_ = flac->channels;
		num_frames_ = FrameCount(flac->totalPCMFrameCount);
		sample_rate_ = flac->sampleRate;
		bit_depth_ = flac->bitsPerSample;

		drflac_close(flac);

		return true;
	};

	const auto read_frames = [this, open_file](Callbacks callbacks, std::uint32_t chunk_size)
	{
		auto flac = open_file(utf8_path_);

		if (!flac) throw std::runtime_error("Read error");

		const auto read_func = [&flac](float* buffer, std::uint32_t read_size)
		{
			return drflac_read_pcm_frames_f32(flac, read_size, buffer) == read_size;
		};

		generic_dr_libs_frame_reader_loop(callbacks, read_func, chunk_size, num_channels_, num_frames_);

		drflac_close(flac);
	};

	return { AudioFileFormat::FLAC, try_read_header, read_frames };
}

auto AudioFileReader::make_mp3_handler() -> FormatHandler
{
	const auto init_file =
	#ifdef _WIN32
		[](drmp3* mp3, const std::string& utf8_path)
		{
			return drmp3_init_file_w(mp3, (const wchar_t*)(utf8::utf8to16(utf8_path).c_str()), nullptr);
		};
	#else
		[](drmp3* mp3, const std::string& utf8_path)
		{
			return drmp3_init_file(mp3, utf8_path.c_str(), nullptr);
		};
	#endif

	const auto try_read_header = [this, init_file]()
	{
		drmp3 mp3;

		if (!init_file(&mp3, utf8_path_)) return false;

		num_channels_ = mp3.channels;
		num_frames_ = FrameCount(drmp3_get_pcm_frame_count(&mp3));
		sample_rate_ = mp3.sampleRate;
		bit_depth_ = 32;

		drmp3_uninit(&mp3);

		return true;
	};

	const auto read_frames = [this, init_file](Callbacks callbacks, std::uint32_t chunk_size)
	{
		drmp3 mp3;

		if (!init_file(&mp3, utf8_path_)) throw std::runtime_error("Read error");

		const auto read_func = [&mp3](float* buffer, std::uint32_t read_size)
		{
			return drmp3_read_pcm_frames_f32(&mp3, read_size, buffer) == read_size;
		};

		generic_dr_libs_frame_reader_loop(callbacks, read_func, chunk_size, num_channels_, num_frames_);

		drmp3_uninit(&mp3);
	};

	return { AudioFileFormat::MP3, try_read_header, read_frames };
}

auto AudioFileReader::make_wav_handler() -> FormatHandler
{
	const auto init_file =
	#ifdef _WIN32
		[](drwav* wav, const std::string& utf8_path)
		{
			return drwav_init_file_w(wav, (const wchar_t*)(utf8::utf8to16(utf8_path).c_str()), nullptr);
		};
	#else
		[](drwav* wav, const std::string& utf8_path)
		{
			return drwav_init_file(wav, utf8_path.c_str(), nullptr);
		};
	#endif

	const auto try_read_header = [this, init_file]()
	{
		drwav wav;

		if (!init_file(&wav, utf8_path_)) return false;

		num_channels_ = wav.channels;
		num_frames_ = FrameCount(wav.totalPCMFrameCount);
		sample_rate_ = wav.sampleRate;
		bit_depth_ = wav.bitsPerSample;

		drwav_uninit(&wav);

		return true;
	};

	const auto read_frames = [this, init_file](Callbacks callbacks, std::uint32_t chunk_size)
	{
		drwav wav;

		if (!init_file(&wav, utf8_path_)) throw std::runtime_error("Read error");

		const auto read_func = [&wav](float* buffer, std::uint32_t read_size)
		{
			return drwav_read_pcm_frames_f32(&wav, read_size, buffer) == read_size;
		};

		generic_dr_libs_frame_reader_loop(callbacks, read_func, chunk_size, num_channels_, num_frames_);

		drwav_uninit(&wav);
	};

	return { AudioFileFormat::WAV, try_read_header, read_frames };
}

auto AudioFileReader::make_wavpack_handler() -> FormatHandler
{
	const auto try_read_header = [this]() -> bool
	{
		wavpack::Reader reader(utf8_path_);

		if (!reader.try_read_header()) return false;

		num_channels_ = reader.get_num_channels();
		num_frames_ = reader.get_num_frames();
		sample_rate_ = reader.get_sample_rate();
		bit_depth_ = reader.get_bit_depth();

		return true;
	};

	const auto read_frames = [this](Callbacks callbacks, std::uint32_t chunk_size)
	{
		wavpack::Reader reader(utf8_path_);

		if (!reader.try_read_header()) throw std::runtime_error("Read Error");

		wavpack::Reader::Callbacks reader_callbacks;

		reader_callbacks.return_chunk = callbacks.return_chunk;
		reader_callbacks.should_abort = callbacks.should_abort;

		reader.read_frames(reader_callbacks, chunk_size);
	};

	return { AudioFileFormat::WavPack, try_read_header, read_frames };
}

auto AudioFileReader::make_format_attempt_order(AudioFileFormat format_hint) -> std::array<FormatHandler, 4>
{
	switch (format_hint)
	{
		case AudioFileFormat::FLAC: return { flac_handler_, wav_handler_, mp3_handler_, wavpack_handler_ };
		case AudioFileFormat::MP3: return { mp3_handler_, wav_handler_, flac_handler_, wavpack_handler_ };
		case AudioFileFormat::WavPack: return { wavpack_handler_, wav_handler_, mp3_handler_, flac_handler_ };
		default:
		case AudioFileFormat::WAV: return { wav_handler_, mp3_handler_, flac_handler_, wavpack_handler_ };
	}
}

AudioFileReader::AudioFileReader(const std::string& utf8_path, AudioFileFormat format_hint)
	: flac_handler_(make_flac_handler())
	, mp3_handler_(make_mp3_handler())
	, wav_handler_(make_wav_handler())
	, wavpack_handler_(make_wavpack_handler())
	, utf8_path_(utf8_path)
	, format_hint_(format_hint)
{
}

void AudioFileReader::read_header()
{
	const auto formats_to_try = make_format_attempt_order(format_hint_);

	for (auto format : formats_to_try)
	{
		if (format.try_read_header())
		{
			active_format_handler_ = format;
			return;
		}
	}

	throw std::runtime_error("File format not recognized");
}

void AudioFileReader::read_frames(Callbacks callbacks, std::uint32_t chunk_size)
{
	if (active_format_handler_.format == AudioFileFormat::None) read_header();

	active_format_handler_.read_frames(callbacks, chunk_size);
}

}}