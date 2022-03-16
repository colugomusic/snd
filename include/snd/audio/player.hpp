#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPBuffer.h>
#include <DSP/MLDSPGens.h>
#pragma warning(pop)
#include <stupid/stupid.hpp>

namespace snd {
namespace audio {

class Player
{
public:

	struct Callbacks
	{
		std::function<void()> stopped;
		std::function<void()> request_more_frames;
		std::function<void(float)> return_progress;
	};

	struct Config
	{
		stupid::SyncSignal* sync_signal {};

		Callbacks callbacks;
	};

	struct Source
	{
		int SR {};
		ml::DSPBuffer* buffer {};

		// number of frames after SR conversion, not necessarily
		// equal to the number of frames in the file
		std::uint64_t num_frames {};
		std::uint64_t start_position {};
		int num_channels {};
	};

	Player(Config config);

	auto set_source(Source source) -> void;
	auto play() -> void;
	auto stop() -> void;
	auto request_progress() -> void;
	
	auto operator()() -> ml::DSPVectorArray<2>;

private:

	struct Static
	{
		Callbacks callbacks;
	} static_;

	struct SyncData
	{
		Source source;
	};

	stupid::QuickSync<SyncData> sync_;

	struct Audio
	{
		stupid::AtomicTrigger request_play;
		stupid::AtomicTrigger request_stop;
		stupid::AtomicTrigger request_progress;

		auto operator()(const Static& static_data, const SyncData& sync_data) -> ml::DSPVectorArray<2>;

	private:

		auto play(const Static& static_data, const SyncData& sync_data) -> ml::DSPVectorArray<2>;
		auto stop(const Static& static_data, const SyncData& sync_data) -> ml::DSPVectorArray<2>;

		auto update_state(const Static& static_data, const SyncData& sync_data) -> void;
		auto begin_stopping(const Static& static_data, const SyncData& sync_data) -> void;
		auto begin_playing(const Static& static_data, const SyncData& sync_data) -> void;
		auto finish_playing(const Static& static_data) -> void;

		static constexpr auto FADEOUT_MS { 100.f };

		enum class State { Playing, Stopping, Stopped } state_ { State::Stopped };

		std::uint64_t position_ {};
		std::uint32_t frames_available_ {};
		bool frames_requested_ {};
		float progress_ {};
		float amp_ { 1.0f };

		struct FadeOut
		{
			int vectors_total {};
			int vectors_remaining {};
		} fadeout_;
	} audio_;
};

inline Player::Player(Config config)
	: sync_ { *config.sync_signal }
	, static_ { config.callbacks }
{
}

inline auto Player::set_source(Source source) -> void
{
	sync_.sync_new([source](SyncData* data) { data->source = source; });
}

inline auto Player::play() -> void
{
	audio_.request_play();
}

inline auto Player::stop() -> void
{
	audio_.request_stop();
}

inline auto Player::request_progress() -> void
{
	audio_.request_progress();
}

inline auto Player::operator()() -> ml::DSPVectorArray<2>
{
	return audio_(static_, sync_.get_data());
}

inline auto Player::Audio::begin_playing(const Static& static_data, const SyncData& sync_data) -> void
{
	state_ = Audio::State::Playing;
	progress_ = 0;
	position_ = sync_data.source.start_position;
	frames_requested_ = false;
}

inline auto Player::Audio::begin_stopping(const Static& static_data, const SyncData& sync_data) -> void
{
	const auto fadeout_frames { float(sync_data.source.SR) * (FADEOUT_MS / 1000) };

	fadeout_.vectors_total = int(fadeout_frames / kFloatsPerDSPVector);
	fadeout_.vectors_remaining = fadeout_.vectors_total;

	state_ = Audio::State::Stopping;
}

inline auto Player::Audio::update_state(const Static& static_data, const SyncData& sync_data) -> void
{
	switch (state_)
	{
		case Audio::State::Playing:
		{
			if (request_stop)
			{
				begin_stopping(static_data, sync_data);
			}

			return;
		}

		case Audio::State::Stopping:
		{
			if (fadeout_.vectors_remaining <= 0)
			{
				finish_playing(static_data);
			}

			return;
		}

		case Audio::State::Stopped:
		{
			if (request_stop)
			{
				finish_playing(static_data);
				return;
			}

			if (request_play)
			{
				begin_playing(static_data, sync_data);
			}

			return;
		}

		default: return;
	}
}

inline auto Player::Audio::finish_playing(const Static& static_data) -> void
{
	state_ = State::Stopped;
	static_data.callbacks.stopped();
}

inline auto Player::Audio::play(const Static& static_data, const SyncData& sync_data) -> ml::DSPVectorArray<2>
{
	ml::DSPVectorArray<2> out;

	const auto samples_available { std::uint32_t(sync_data.source.buffer->getReadAvailable()) };
	const auto previous_frames_available { frames_available_ };
	const auto current_frames_available { samples_available / sync_data.source.num_channels };

	if (current_frames_available > previous_frames_available)
	{
		frames_requested_ = false;
	}

	if (current_frames_available <= 0)
	{
		finish_playing(static_data);
		return out;
	}

	if (current_frames_available < kFloatsPerDSPVector)
	{
		for (int c = 0; c < sync_data.source.num_channels; c++)
		{
			sync_data.source.buffer->read(out.row(c).getBuffer(), current_frames_available);
		}

		position_ += current_frames_available;
		progress_ = float(position_) / (sync_data.source.num_frames - 1);

		finish_playing(static_data);
		return out;
	}

	for (int c = 0; c < sync_data.source.num_channels; c++)
	{
		sync_data.source.buffer->read(out.row(c));
	}

	if (sync_data.source.num_channels == 1)
	{
		out.row(1) = out.row(0);
	}

	position_ += kFloatsPerDSPVector;
	progress_ = float(position_) / (sync_data.source.num_frames - 1);

	const auto minimum_frames { std::min(std::uint64_t(kFloatsPerDSPVector * 1000), sync_data.source.num_frames) };

	if (!frames_requested_ && (current_frames_available - kFloatsPerDSPVector) < minimum_frames)
	{
		static_data.callbacks.request_more_frames();
		frames_requested_ = true;
	}

	frames_available_ = current_frames_available;

	return out;
}

inline auto Player::Audio::stop(const Static& static_data, const SyncData& sync_data) -> ml::DSPVectorArray<2>
{
	const auto fadeout_frames { fadeout_.vectors_total * kFloatsPerDSPVector };
	const auto x { (((ml::kUnityRampVec - 1.0f) * kFloatsPerDSPVector) + float(kFloatsPerDSPVector * (fadeout_.vectors_total - fadeout_.vectors_remaining))) / float(fadeout_frames) };
	const auto amp { 1.0f - x };

	fadeout_.vectors_remaining--;

	return play(static_data, sync_data) * ml::repeatRows<2>(amp);
}

inline auto Player::Audio::operator()(const Static& static_data, const SyncData& sync_data) -> ml::DSPVectorArray<2>
{
	update_state(static_data, sync_data);

	ml::DSPVectorArray<2> out;

	switch(state_)
	{
		case State::Playing: out = play(static_data, sync_data); break;
		case State::Stopping: out = stop(static_data, sync_data); break;
		case State::Stopped: default: out = ml::DSPVectorArray<2>(); break;
	}

	if (request_progress)
	{
		static_data.callbacks.return_progress(progress_);
	}

	return out;
}

} // audio
} // snd
