#pragma once

#ifdef SND_WITH_MOODYCAMEL

#include "../threading.hpp"
#include <optional>
#include <variant>
#pragma warning(push, 0)
#include <DSP/MLDSPBuffer.h>
#include <DSP/MLDSPGens.h>
#pragma warning(pop)

namespace snd {
namespace audio {

namespace impl { struct stream_player_t; }

struct stream_player {
	struct listener_t {
		virtual ~listener_t() = default;
		virtual auto on_stopped() -> void = 0;
		virtual auto on_more_frames_needed() -> void = 0;
		virtual auto on_progress(float) -> void = 0;
	};
	struct source_t {
		// number of frames after SR conversion, not necessarily
		// equal to the number of frames in the file
		std::uint64_t num_frames     = 0;
		std::uint64_t start_position = 0;
		int num_channels             = 0;
		int SR                       = 0;
	};
	stream_player(ml::DSPBuffer* source_buffer, std::unique_ptr<listener_t>&& listener, th::message_queue_reporter reporter);
	[[nodiscard]] auto audio__process() -> ml::DSPVectorArray<2>;
	auto main__is_playing() const -> bool;
	auto main__play(source_t source) -> void;
	auto main__stop() -> void;
	auto main__ui_frame() -> void;
private:
	std::unique_ptr<impl::stream_player_t> impl;
};

namespace impl {

static constexpr auto INITIAL_QUEUE_SIZE = 10;

namespace from_audio {

struct i_need_more_frames {};
struct progress { float value; };
struct stopped {};

using message = std::variant<i_need_more_frames, progress, stopped>;
using message_q = th::message_queue<INITIAL_QUEUE_SIZE, message>;

} // from_audio

namespace from_main {

struct get_progress {};
struct play { stream_player::source_t source; };
struct stop {};

using message = std::variant<get_progress, play, stop>;
using message_q = th::message_queue<INITIAL_QUEUE_SIZE, message>;

} // from_main

struct audio_t {
	enum class state_t { playing, stopping, stopped };
	struct fadeout_t {
		int vectors_total     = 0;
		int vectors_remaining = 0;
	};
	fadeout_t fadeout;
	stream_player::source_t source;
	th::message_queue_reporter reporter;
	std::optional<from_main::play> play_requested;
	ml::DSPBuffer* source_buffer = nullptr;
	state_t state                = state_t::stopped;
	uint64_t position            = 0;
	uint32_t frames_available    = 0;
	bool frames_requested        = false;
	float progress               = 0.0f;
	float amp                    = 1.0f;
};

struct critical_t {
	from_audio::message_q msgs_from_audio;
	from_main::message_q msgs_from_main;
};

struct main_t {
	std::unique_ptr<stream_player::listener_t> listener;
	bool is_playing = false;
};

struct stream_player_t {
	audio_t audio;
	critical_t critical;
	main_t main;
};

namespace audio {

static constexpr auto FADEOUT_MS = 100.f;

inline
auto send(impl::stream_player_t* impl, from_audio::message msg) -> void {
	th::realtime::send(&impl->critical.msgs_from_audio, std::move(msg), impl->audio.reporter);
}

inline
auto begin_stopping(impl::stream_player_t* impl) -> void {
	const auto fadeout_frames = float(impl->audio.source.SR) * (FADEOUT_MS / 1000);
	impl->audio.fadeout.vectors_total = int(fadeout_frames / kFloatsPerDSPVector);
	impl->audio.fadeout.vectors_remaining = impl->audio.fadeout.vectors_total;
	impl->audio.state = audio_t::state_t::stopping;
}

inline
auto begin_playing(impl::stream_player_t* impl) -> void {
	impl->audio.state = audio_t::state_t::playing;
	impl->audio.progress = 0;
	impl->audio.position = impl->audio.source.start_position;
	impl->audio.frames_requested = false;
	impl->audio.source         = impl->audio.play_requested->source;
	impl->audio.play_requested = std::nullopt;
}

inline
auto finish_playing(impl::stream_player_t* impl) -> void {
	impl->audio.state = audio_t::state_t::stopped;
	send(impl, from_audio::stopped{});
}

[[nodiscard]] inline
auto play(impl::stream_player_t* impl) -> ml::DSPVectorArray<2> {
	ml::DSPVectorArray<2> out;
	const auto samples_available         =  static_cast<uint32_t>(impl->audio.source_buffer->getReadAvailable());
	const auto previous_frames_available = impl->audio.frames_available;
	const auto current_frames_available  = samples_available / impl->audio.source.num_channels;
	if (current_frames_available > previous_frames_available) {
		impl->audio.frames_requested = false;
	}
	if (current_frames_available <= 0) {
		finish_playing(impl);
		return out;
	}
	if (current_frames_available < kFloatsPerDSPVector) {
		for (int c = 0; c < impl->audio.source.num_channels; c++) {
			impl->audio.source_buffer->read(out.row(c).getBuffer(), current_frames_available);
		}
		impl->audio.position += current_frames_available;
		impl->audio.progress = float(impl->audio.position) / (impl->audio.source.num_frames - 1);
		finish_playing(impl);
		return out;
	}
	for (int c = 0; c < impl->audio.source.num_channels; c++) {
		impl->audio.source_buffer->read(out.row(c));
	}
	if (impl->audio.source.num_channels == 1) {
		out.row(1) = out.row(0);
	}
	impl->audio.position += kFloatsPerDSPVector;
	impl->audio.progress = float(impl->audio.position) / (impl->audio.source.num_frames - 1);
	const auto minimum_frames = std::min(std::uint64_t(kFloatsPerDSPVector * 1000), impl->audio.source.num_frames);
	if (!impl->audio.frames_requested && (current_frames_available - kFloatsPerDSPVector) < minimum_frames) {
		send(impl, from_audio::i_need_more_frames{});
		impl->audio.frames_requested = true;
	}
	impl->audio.frames_available = current_frames_available;
	return out;
}

[[nodiscard]] inline
auto stop(impl::stream_player_t* impl) -> ml::DSPVectorArray<2> {
	const auto fadeout_frames = impl->audio.fadeout.vectors_total * kFloatsPerDSPVector;
	const auto x = (((ml::kUnityRampVec - 1.0f) * kFloatsPerDSPVector) + float(kFloatsPerDSPVector * (impl->audio.fadeout.vectors_total - impl->audio.fadeout.vectors_remaining))) / float(fadeout_frames);
	const auto amp = 1.0f - x;
	impl->audio.fadeout.vectors_remaining--;
	const auto out = play(impl) * ml::repeatRows<2>(amp);
	if (impl->audio.fadeout.vectors_remaining <= 0) {
		finish_playing(impl);
	}
	return out;
}

inline
auto receive(impl::stream_player_t* impl, from_main::get_progress) -> void {
	send(impl, from_audio::progress{impl->audio.progress});
}

inline
auto receive(impl::stream_player_t* impl, from_main::play msg) -> void {
	impl->audio.play_requested = msg;
	if (impl->audio.state == audio_t::state_t::stopped) {
		begin_playing(impl);
		return;
	}
	if (impl->audio.state == audio_t::state_t::playing) {
		begin_stopping(impl);
		return;
	}
}

inline
auto receive(impl::stream_player_t* impl, from_main::stop) -> void {
	if (impl->audio.state == audio_t::state_t::playing) {
		begin_stopping(impl);
		return;
	}
}

inline
auto receive_msgs(impl::stream_player_t* impl) -> void {
	th::receive(&impl->critical.msgs_from_main, [impl](const from_main::message& msg) {
		std::visit([impl](auto&& msg) { receive(impl, msg); }, msg);
	});
}

[[nodiscard]] inline
auto process(impl::stream_player_t* impl) -> ml::DSPVectorArray<2> {
	receive_msgs(impl);
	ml::DSPVectorArray<2> out;
	switch(impl->audio.state) {
		case audio_t::state_t::playing: out = play(impl); break;
		case audio_t::state_t::stopping: out = stop(impl); break;
		case audio_t::state_t::stopped: default: out = ml::DSPVectorArray<2>(); break;
	}
	return out;
}

} // audio

namespace main {

inline
auto receive(impl::stream_player_t* impl, from_audio::i_need_more_frames) -> void {
	impl->main.listener->on_more_frames_needed();
}

inline
auto receive(impl::stream_player_t* impl, from_audio::progress msg) -> void {
	impl->main.listener->on_progress(msg.value);
}

inline
auto receive(impl::stream_player_t* impl, from_audio::stopped) -> void {
	impl->main.listener->on_stopped();
	impl->main.is_playing = false;
}

inline
auto receive_msgs(impl::stream_player_t* impl) -> void {
	th::receive(&impl->critical.msgs_from_audio, [impl](const from_audio::message& msg) {
		std::visit([impl](auto&& msg) { receive(impl, msg); }, msg);
	});
}

inline
auto send(impl::stream_player_t* impl, from_main::message msg) -> void {
	th::non_realtime::send(&impl->critical.msgs_from_main, std::move(msg));
}

inline
auto is_playing(const impl::stream_player_t& impl) -> bool {
	return impl.main.is_playing;
}

inline
auto play(impl::stream_player_t* impl, stream_player::source_t source) -> void {
	send(impl, from_main::play{source});
	impl->main.is_playing = true;
}

inline
auto stop(impl::stream_player_t* impl) -> void {
	if (!is_playing(*impl)) { return; }
	send(impl, from_main::stop{});
}

inline
auto ui_frame(impl::stream_player_t* impl) -> void {
	if (!is_playing(*impl)) { return; }
	send(impl, from_main::get_progress{});
	receive_msgs(impl);
}

} // main

} // impl

stream_player::stream_player(ml::DSPBuffer* source_buffer, std::unique_ptr<listener_t>&& listener, th::message_queue_reporter reporter)
	: impl { std::make_unique<impl::stream_player_t>() }
{
	impl->audio.reporter = reporter;
	impl->audio.source_buffer = source_buffer;
	impl->main.listener = std::move(listener);
}

auto stream_player::audio__process() -> ml::DSPVectorArray<2>   { return impl::audio::process(this->impl.get()); } 
auto stream_player::main__is_playing() const -> bool            { return impl::main::is_playing(*this->impl); }
auto stream_player::main__play(stream_player::source_t source) -> void { return impl::main::play(this->impl.get(), source); } 
auto stream_player::main__stop() -> void                        { return impl::main::stop(this->impl.get()); } 
auto stream_player::main__ui_frame() -> void                    { return impl::main::ui_frame(this->impl.get()); }

} // audio
} // snd

#endif // SND_WITH_MOODYCAMEL
