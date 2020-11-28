#include "wavpack_reader.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <wavpack.h>

namespace snd {
namespace storage {
namespace wavpack {

Reader::~Reader()
{
	if (context_)
	{
		WavpackCloseFile(context_);
	}
}

bool Reader::try_read_header()
{
	context_ = open();

	if (!context_) return false;

	num_channels_ = WavpackGetNumChannels(context_);
	num_frames_ = FrameCount(WavpackGetNumSamples64(context_));
	sample_rate_ = WavpackGetSampleRate(context_);
	bit_depth_ = WavpackGetBitsPerSample(context_);

	const auto mode = WavpackGetMode(context_);

	if ((mode & MODE_FLOAT) == MODE_FLOAT)
	{
		chunk_reader_ = [this](float* buffer, std::uint32_t read_size)
		{
			return WavpackUnpackSamples(context_, reinterpret_cast<int32_t*>(buffer), read_size) == read_size;
		};
	}
	else
	{
		chunk_reader_ = [this](float* buffer, std::uint32_t read_size)
		{
			const auto divisor = (1 << (bit_depth_ - 1)) - 1;

			std::vector<std::int32_t> frames(size_t(num_channels_) * read_size);

			const auto frames_read = WavpackUnpackSamples(context_, frames.data(), read_size);

			if (frames_read != read_size) return false;

			for (std::uint32_t i = 0; i < read_size * num_channels_; i++)
			{
				buffer[i] = float(frames[i]) / divisor;
			}

			return true;
		};
	}

    return true;
}

void Reader::read_frames(Callbacks callbacks, std::uint32_t chunk_size)
{
	if (!context_) try_read_header();

	if (!context_) throw std::runtime_error("Read error");

	FrameCount frame = 0;

	while (frame < num_frames_)
	{
		if (callbacks.should_abort()) break;

		std::vector<float> interleaved_frames;

		auto read_size = chunk_size;

		if (frame + read_size >= num_frames_)
		{
			read_size = num_frames_ - frame;
		}

		interleaved_frames.resize(size_t(read_size) * num_channels_);

		if (!chunk_reader_(interleaved_frames.data(), read_size)) throw std::runtime_error("Read error");

		callbacks.return_chunk(frame, interleaved_frames.data(), read_size);

		frame += read_size;
	}
}

}}}
