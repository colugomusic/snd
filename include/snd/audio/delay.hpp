#pragma once

#include <array>
#include <functional>
#include <vector>

#pragma warning(push, 0)
#include <DSP/MLDSPGens.h>
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

#include <snd/interpolation.hpp>

namespace snd {
namespace audio {

template <size_t ROWS>
class Delay
{

public:

	Delay(int SR, size_t size)
		: SR_(SR)
		, size_(size)
	{
		for (int row = 0; row < ROWS; row++)
		{
			buffer_[row].resize(size * kFloatsPerDSPVector);
		}
	}

	ml::DSPVectorArray<ROWS> operator()(
		const ml::DSPVectorArray<ROWS>& dry,
		const ml::DSPVectorArray<ROWS>& time)
	{
		write(dry);

		const auto out = process(time);

		advance();

		return out;
	}

	void clear()
	{
		for (int row = 0; row < ROWS; row++)
		{
			std::fill(buffer_[row].begin(), buffer_[row].end(), 0.0f);
		}
	}

private:

	struct DelayTime
	{
		ml::DSPVectorArrayInt<ROWS> i;
		ml::DSPVectorArray<ROWS> f;
	};

	using InterpFrames = std::array<ml::DSPVectorArrayInt<ROWS>, 4>;
	using InterpSamples = std::array<ml::DSPVectorArray<ROWS>, 4>;

	struct InterpSpec
	{
		InterpSamples samples;
		ml::DSPVectorArray<ROWS> f;
	};

	auto get_delay_time(ml::DSPVectorArray<ROWS> time)
	{
		static const ml::DSPVectorArray<ROWS> min { 2.0f };
		const ml::DSPVectorArray<ROWS> max { float(size_) * kFloatsPerDSPVector };

		time = ml::clamp(time, min, max);

		DelayTime out;

		out.i = ml::truncateFloatToInt(time);
		out.f = ml::fractionalPart(time);

		return out;
	}

	auto get_delayed_frame(const DelayTime& time)
	{
		const auto index = ml::truncateFloatToInt(ml::repeatRows<ROWS>(ml::columnIndex()));
		const auto vector_frame = ml::DSPVectorArrayInt<ROWS>(int(write_vector_) * kFloatsPerDSPVector);
		const auto just_written_frame = index + vector_frame;

		return just_written_frame - time.i;
	}
	
	auto get_wrapped_frame(const ml::DSPVectorArrayInt<ROWS>& frame)
	{
		ml::DSPVectorArrayInt<ROWS> out;

		for (int row = 0; row < ROWS; row++)
		{
			const auto& frame_row = frame.constRow(row);
			auto& out_row = out.row(row);

			for (int i = 0; i < kFloatsPerDSPVector; i++)
			{
				out_row[i] = frame_row[i];

				if (out_row[i] < 0)
				{
					out_row[i] += int(size_ * kFloatsPerDSPVector);
				}
			}
		}

		return out;
	}

	auto get_interp_frames(const DelayTime& time)
	{
		const auto delayed_frame = get_delayed_frame(time);

		InterpFrames out;

		out[0] = get_wrapped_frame(delayed_frame - ml::DSPVectorArrayInt<ROWS>(1));
		out[1] = get_wrapped_frame(delayed_frame);
		out[2] = get_wrapped_frame(delayed_frame + ml::DSPVectorArrayInt<ROWS>(1));
		out[3] = get_wrapped_frame(delayed_frame + ml::DSPVectorArrayInt<ROWS>(2));

		return out;
	}

	auto get_interp_samples(const InterpFrames& frames)
	{
		InterpSamples out;

		for (int part = 0; part < 4; part++)
		{
			for (int row = 0; row < ROWS; row++)
			{
				const auto& buffer_row = buffer_[row];
				const auto& frames_row = frames[part].constRow(row);
				auto& out_row = out[part].row(row);

				for (int i = 0; i < kFloatsPerDSPVector; i++)
				{
					const auto frame = frames_row[i];

					out[part].row(row)[i] = buffer_row[frame];
				}
			}
		}

		return out;
	}

	auto get_delayed_signal(const DelayTime& time)
	{
		ml::DSPVectorArray<ROWS> out;

		const auto interp_frames = get_interp_frames(time);
		const auto interp_samples = get_interp_samples(interp_frames);

		return snd::interpolation::interp_4pt(interp_samples[0], interp_samples[1], interp_samples[2], interp_samples[3], time.f);
	}

	void write(const ml::DSPVectorArray<ROWS>& dry)
	{
		for (int row = 0; row < ROWS; row++)
		{
			ml::store(dry.constRow(row), buffer_[row].data() + (write_vector_ * kFloatsPerDSPVector));
		}
	}

	ml::DSPVectorArray<ROWS> process(const ml::DSPVectorArray<ROWS>& time)
	{
		const auto delay_time = get_delay_time(time);
		const auto delayed_signal = get_delayed_signal(delay_time);

		return delayed_signal;
	}

	void advance()
	{
		if (++write_vector_ >= size_) write_vector_ = 0;
	}

	int SR_ { 44100 };
	size_t write_vector_ { 0 };
	size_t size_;
	std::array<std::vector<float>, ROWS> buffer_;
};

} // audio
} // snd