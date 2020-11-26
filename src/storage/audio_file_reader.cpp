#include "storage/audio_file_reader.h"
#include <array>
#include <stdexcept>
#include "dr_libs/dr_libs_utils.h"
#include <snd/storage/interleaving.h>
#include "wavpack/wavpack_file_reader.h"
#include "wavpack/wavpack_memory_reader.h"

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

static AudioFileReader::Format read_header_info(drflac* flac)
{
	AudioFileReader::Format out;

	out.num_channels = flac->channels;
	out.num_frames = FrameCount(flac->totalPCMFrameCount);
	out.sample_rate = flac->sampleRate;
	out.bit_depth = flac->bitsPerSample;

	return out;
}

static AudioFileReader::Format read_header_info(drmp3* mp3)
{
	AudioFileReader::Format out;

	out.num_channels = mp3->channels;
	out.num_frames = FrameCount(drmp3_get_pcm_frame_count(mp3));
	out.sample_rate = mp3->sampleRate;
	out.bit_depth = 32;

	return out;
}

static AudioFileReader::Format read_header_info(drwav* wav)
{
	AudioFileReader::Format out;

	out.num_channels = wav->channels;
	out.num_frames = FrameCount(wav->totalPCMFrameCount);
	out.sample_rate = wav->sampleRate;
	out.bit_depth = wav->bitsPerSample;

	return out;
}

static AudioFileReader::Format read_header_info(const wavpack::Reader& reader)
{
	AudioFileReader::Format out;

	out.num_channels = reader.get_num_channels();
	out.num_frames = reader.get_num_frames();
	out.sample_rate = reader.get_sample_rate();
	out.bit_depth = reader.get_bit_depth();

	return out;
}

static void read_frame_data(drflac* flac, AudioFileReader::Callbacks callbacks, const AudioFileReader::Format& format, std::uint32_t chunk_size)
{
	const auto read_func = [flac](float* buffer, std::uint32_t read_size)
	{
		return drflac_read_pcm_frames_f32(flac, read_size, buffer) == read_size;
	};

	generic_dr_libs_frame_reader_loop(callbacks, read_func, chunk_size, format.num_channels, format.num_frames);
}

static void read_frame_data(drmp3* mp3, AudioFileReader::Callbacks callbacks, const AudioFileReader::Format& format, std::uint32_t chunk_size)
{
	const auto read_func = [mp3](float* buffer, std::uint32_t read_size)
	{
		return drmp3_read_pcm_frames_f32(mp3, read_size, buffer) == read_size;
	};

	generic_dr_libs_frame_reader_loop(callbacks, read_func, chunk_size, format.num_channels, format.num_frames);
}

static void read_frame_data(drwav* wav, AudioFileReader::Callbacks callbacks, const AudioFileReader::Format& format, std::uint32_t chunk_size)
{
	const auto read_func = [wav](float* buffer, std::uint32_t read_size)
	{
		return drwav_read_pcm_frames_f32(wav, read_size, buffer) == read_size;
	};

	generic_dr_libs_frame_reader_loop(callbacks, read_func, chunk_size, format.num_channels, format.num_frames);
}

auto AudioFileReader::make_flac_handler(const std::string& utf8_path) -> FormatHandler
{
	const auto try_read_header = [this, utf8_path]() -> bool
	{
		auto flac = dr_libs::flac::open_file(utf8_path);
		
		if (!flac) return false;

		format_ = read_header_info(flac);

		drflac_close(flac);

		return true;
	};

	const auto read_frames = [this, utf8_path](Callbacks callbacks, std::uint32_t chunk_size)
	{
		auto flac = dr_libs::flac::open_file(utf8_path);

		if (!flac) throw std::runtime_error("Read error");

		read_frame_data(flac, callbacks, format_, chunk_size);

		drflac_close(flac);
	};

	return { AudioFileFormat::FLAC, try_read_header, read_frames };
}

auto AudioFileReader::make_flac_handler(const void* data, std::size_t data_size) -> FormatHandler
{
	const auto try_read_header = [this, data, data_size]() -> bool
	{
		auto flac = drflac_open_memory(data, data_size, nullptr);

		if (!flac) return false;

		format_ = read_header_info(flac);

		drflac_close(flac);

		return true;
	};

	const auto read_frames = [this, data, data_size](Callbacks callbacks, std::uint32_t chunk_size)
	{
		auto flac = drflac_open_memory(data, data_size, nullptr);

		if (!flac) throw std::runtime_error("Read error");

		read_frame_data(flac, callbacks, format_, chunk_size);

		drflac_close(flac);
	};

	return { AudioFileFormat::FLAC, try_read_header, read_frames };
}

auto AudioFileReader::make_mp3_handler(const std::string& utf8_path) -> FormatHandler
{
	const auto try_read_header = [this, utf8_path]()
	{
		drmp3 mp3;

		if (!dr_libs::mp3::init_file(&mp3, utf8_path)) return false;

		format_ = read_header_info(&mp3);

		drmp3_uninit(&mp3);

		return true;
	};

	const auto read_frames = [this, utf8_path](Callbacks callbacks, std::uint32_t chunk_size)
	{
		drmp3 mp3;

		if (!dr_libs::mp3::init_file(&mp3, utf8_path)) throw std::runtime_error("Read error");

		read_frame_data(&mp3, callbacks, format_, chunk_size);

		drmp3_uninit(&mp3);
	};

	return { AudioFileFormat::MP3, try_read_header, read_frames };
}

auto AudioFileReader::make_mp3_handler(const void* data, std::size_t data_size) -> FormatHandler
{
	const auto try_read_header = [this, data, data_size]() -> bool
	{
		drmp3 mp3;

		if (!drmp3_init_memory(&mp3, data, data_size, nullptr)) return false;

		format_ = read_header_info(&mp3);

		drmp3_uninit(&mp3);

		return true;
	};

	const auto read_frames = [this, data, data_size](Callbacks callbacks, std::uint32_t chunk_size)
	{
		drmp3 mp3;

		if (!drmp3_init_memory(&mp3, data, data_size, nullptr)) throw std::runtime_error("Read error");

		read_frame_data(&mp3, callbacks, format_, chunk_size);

		drmp3_uninit(&mp3);
	};

	return { AudioFileFormat::MP3, try_read_header, read_frames };
}

auto AudioFileReader::make_wav_handler(const std::string& utf8_path) -> FormatHandler
{
	const auto try_read_header = [this, utf8_path]()
	{
		drwav wav;

		if (!dr_libs::wav::init_file(&wav, utf8_path)) return false;

		format_ = read_header_info(&wav);

		drwav_uninit(&wav);

		return true;
	};

	const auto read_frames = [this, utf8_path](Callbacks callbacks, std::uint32_t chunk_size)
	{
		drwav wav;

		if (!dr_libs::wav::init_file(&wav, utf8_path)) throw std::runtime_error("Read error");

		read_frame_data(&wav, callbacks, format_, chunk_size);

		drwav_uninit(&wav);
	};

	return { AudioFileFormat::WAV, try_read_header, read_frames };
}

 auto AudioFileReader::make_wav_handler(const void* data, std::size_t data_size) -> FormatHandler
{
	const auto try_read_header = [this, data, data_size]()
	{
		drwav wav;

		if (!drwav_init_memory(&wav, data, data_size, nullptr)) return false;

		format_ = read_header_info(&wav);

		drwav_uninit(&wav);

		return true;
	};

	const auto read_frames = [this, data, data_size](Callbacks callbacks, std::uint32_t chunk_size)
	{
		drwav wav;

		if (!drwav_init_memory(&wav, data, data_size, nullptr)) throw std::runtime_error("Read error");

		read_frame_data(&wav, callbacks, format_, chunk_size);

		drwav_uninit(&wav);
	};

	return { AudioFileFormat::WAV, try_read_header, read_frames };
}

auto AudioFileReader::make_wavpack_handler(const std::string& utf8_path) -> FormatHandler
{
	const auto try_read_header = [this, utf8_path]() -> bool
	{
		wavpack::FileReader reader(utf8_path);

		if (!reader.try_read_header()) return false;

		format_ = read_header_info(reader);

		return true;
	};

	const auto read_frames = [this, utf8_path](Callbacks callbacks, std::uint32_t chunk_size)
	{
		wavpack::FileReader reader(utf8_path);
		wavpack::Reader::Callbacks reader_callbacks;

		reader_callbacks.return_chunk = callbacks.return_chunk;
		reader_callbacks.should_abort = callbacks.should_abort;

		reader.read_frames(reader_callbacks, chunk_size);
	};

	return { AudioFileFormat::WavPack, try_read_header, read_frames };
}

auto AudioFileReader::make_wavpack_handler(const void* data, std::size_t data_size) -> FormatHandler
{
	const auto try_read_header = [this, data, data_size]() -> bool
	{
		wavpack::MemoryReader reader(data, data_size);

		if (!reader.try_read_header()) return false;

		format_ = read_header_info(reader);

		return true;
	};

	const auto read_frames = [this, data, data_size](Callbacks callbacks, std::uint32_t chunk_size)
	{
		wavpack::MemoryReader reader(data, data_size);
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
	: flac_handler_(make_flac_handler(utf8_path))
	, mp3_handler_(make_mp3_handler(utf8_path))
	, wav_handler_(make_wav_handler(utf8_path))
	, wavpack_handler_(make_wavpack_handler(utf8_path))
	, format_hint_(format_hint)
{
}

AudioFileReader::AudioFileReader(const void* data, std::size_t data_size, AudioFileFormat format_hint)
	: flac_handler_(make_flac_handler(data, data_size))
	, wavpack_handler_(make_wavpack_handler(data, data_size))
	, format_hint_(format_hint)
{
}

void AudioFileReader::read_header()
{
	const auto formats_to_try = make_format_attempt_order(format_hint_);

	for (auto format : formats_to_try)
	{
		if (format && format.try_read_header())
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