#pragma once

#include <array>
#include <vector>
#include <snd/ease.hpp>
#include <snd/misc.hpp>

#pragma warning(push, 0)
#include <DSP/MLDSPBuffer.h>
#include <DSP/MLDSPFilters.h>
#pragma warning(pop)

namespace snd {
namespace wavebender {

static constexpr auto INIT = 4;

enum class CrossfadeMode {
	Static,
	Dynamic,
};

struct FrameWriteParams {
	int bubble = 0;
};

struct FrameReadParams {
	float tilt = 0.0f;
	float spike = 0.0f;
	float ff = 1.0f;
	float crossfade_size = 1.0f;
	CrossfadeMode crossfade_mode = CrossfadeMode::Dynamic;
};

struct Channel {
	std::array<std::vector<float>, 4> buffers;
	struct Span {
		float* buffer = nullptr;
		size_t size = 0;
	};
	struct {
		Span span;
		bool up = false;
		int counter = 0;
		ml::Lopass filter;
	} write; 
	struct {
		Span span;
	} stage; 
	struct {
		Span span;
		float frame = 0.0f;
	} source; 
	struct {
		Span span;
		float frame = 0.0f;
	} target; 
	struct {
		float source_speed_0 = 1.0f;
		float source_speed_1 = 1.0f;
		float target_speed_0 = 1.0f;
		float target_speed_1 = 1.0f;
		bool active = false;
		size_t length = 64;
		size_t index = 0;
	} xfade;
	struct {
		bool active = false;
		size_t length = 64;
		size_t index = 0;
	} fade_in;
	int init = 0;
};

inline
auto do_write(Channel* c, const FrameWriteParams& params, float filtered_value, float value) -> void {
	if (c->init == 0) {
		if (std::abs(value) < 0.000001f) return; 
		c->init++;
	}
	c->write.span.buffer[c->write.span.size++] = value; 
	if (c->write.span.size >= c->buffers[0].size()) {
		c->write.span.size = c->buffers[0].size();
		std::swap(c->write.span, c->stage.span);
		c->write.up = false;
		return;
	}
	if (c->write.up && filtered_value < 0) {
		c->write.up = false;
	}
	else if (!c->write.up && filtered_value > 0) {
		c->write.up = true; 
		if (c->write.span.size > 3) {
			c->write.counter++; 
			if (c->write.counter > params.bubble) {
				if (++c->init >= INIT) {
					std::swap(c->write.span, c->stage.span);
				} 
				c->write.span.size = 0;
				c->write.counter = 0;
			}
		}
	}
}

inline
auto start_fade_in(Channel* c, const FrameReadParams& params) -> void {
	const auto tmp = c->target.span;
	c->target.span = c->stage.span;
	c->target.frame = 0.0f;
	c->stage.span = c->source.span;
	c->stage.span.size = 0;
	c->source.span = tmp;
	c->source.frame = c->target.frame; 
	c->fade_in.active = true;
	c->fade_in.index = 0;
	c->fade_in.length = c->target.span.size * 2;
}

[[nodiscard]] inline
auto window(float x, float r = 0.5f) -> float {
	const auto top = 1.0f - r;
	if (x < r) {
		return snd::ease::parametric::in_out(x * (1.0f / r));
	}
	else if (x > top) {
		return 1.0f - snd::ease::parametric::in_out((x - top) * (1.0f / r));
	}
	else {
		return 1.0f;
	}
}

[[nodiscard]] inline
auto apply_tilt(float frame, float tilt, float spike, size_t size) -> float {
	const auto pos = frame / size;
	const auto tilted = std::pow(pos, std::pow(2.0f, tilt));
	const auto smoothed = snd::lerp(pos, tilted, window(pos));
	return snd::lerp(smoothed, tilted, spike) * size;
}

[[nodiscard]] inline
auto read(const Channel::Span& span, float pos) -> float {
	auto index_0 = size_t(std::floor(pos));
	auto index_1 = size_t(std::ceil(pos));
	const auto x = pos - index_0;
	index_0 = snd::wrap(index_0, span.size);
	index_1 = snd::wrap(index_1, span.size);
	if (index_0 == index_1) {
		return span.buffer[index_0];
	} 
	const auto value_0 = span.buffer[index_0];
	const auto value_1 = span.buffer[index_1];
	return snd::lerp(value_0, value_1, x);
}

[[nodiscard]] inline
auto do_xfade(Channel* c, const FrameReadParams& params) -> float {
	const auto x = snd::ease::quadratic::in_out(float(c->xfade.index) / (c->xfade.length - 1));
	auto source_value = read(c->source.span, apply_tilt(c->source.frame, params.tilt, params.spike, c->source.span.size));
	auto target_value = read(c->target.span, apply_tilt(c->target.frame, params.tilt, params.spike, c->target.span.size));
	const auto value = snd::lerp(source_value, target_value, x);
	const auto source_inc = snd::lerp(c->xfade.source_speed_0, c->xfade.source_speed_1, x) * params.ff;
	const auto target_inc = snd::lerp(c->xfade.target_speed_0, c->xfade.target_speed_1, x) * params.ff; 
	c->source.frame += source_inc;
	c->target.frame += target_inc; 
	const auto source_end = (c->source.span.size - 1);
	const auto target_end = (c->target.span.size - 1); 
	if (c->source.frame > source_end) c->source.frame -= source_end;
	if (c->target.frame > target_end) c->target.frame -= target_end; 
	c->xfade.index++; 
	if (c->xfade.index >= c->xfade.length) {
		c->xfade.active = false;
	} 
	return value;
}

inline
auto prepare_xfade(Channel* c, const FrameReadParams& params) -> void {
	const auto tmp = c->target.span;
	c->target.span = c->stage.span;
	c->target.frame = 0.0f;
	c->stage.span = c->source.span;
	c->stage.span.size = 0;
	c->source.span = tmp;
	c->source.frame = c->target.frame;
}

inline
auto start_xfade(Channel* c, const FrameReadParams& params) -> void {
	c->xfade.active = true;
	c->xfade.index = 0;
	if (params.crossfade_mode == CrossfadeMode::Dynamic) {
		c->xfade.length = size_t(float(snd::midpoint(c->source.span.size, c->target.span.size)) * params.crossfade_size);
	}
	else {
		c->xfade.length = size_t(64.0f * params.crossfade_size);
	} 
	c->xfade.source_speed_1 = float(c->source.span.size) / float(c->target.span.size);
	c->xfade.target_speed_0 = float(c->target.span.size) / float(c->source.span.size);
}

[[nodiscard]] inline
auto do_wet(Channel* c, const FrameReadParams& params) -> float {
	if (c->xfade.active) {
		return do_xfade(c, params);
	}
	auto value = read(c->target.span, apply_tilt(c->target.frame, params.tilt, params.spike, c->target.span.size));
	c->target.frame += params.ff; 
	const auto end = (c->target.span.size - 1); 
	if (c->target.frame > end) {
		c->target.frame -= end; 
		if (c->stage.span.size > 0) {
			prepare_xfade(c, params);
			start_xfade(c, params);
		}
	} 
	return value;
}

[[nodiscard]] inline
auto do_read(Channel* c, const FrameReadParams& params, float in) -> float {
	if (c->init == INIT) {
		start_fade_in(c, params);
		c->init++;
	}
	if (c->init < INIT) {
		return in;
	}
	float value = do_wet(c, params);
	if (c->fade_in.active) {
		const auto amp = snd::ease::quadratic::in_out(float(c->fade_in.index++) / c->fade_in.length);
		value = snd::lerp(in, value, amp);
		if (c->fade_in.index >= c->fade_in.length) {
			c->fade_in.active = false;
		}
	} 
	return value;
}

[[nodiscard]] inline
auto process(Channel* c, const FrameWriteParams& write_params, const FrameReadParams& read_params, float in, float filtered_in) -> float {
	do_write(c, write_params, filtered_in, in);
	return do_read(c, read_params, in);
}

[[nodiscard]] inline
auto apply_smoother(Channel* channel, ml::DSPVector in, float amount) -> ml::DSPVector {
	channel->write.filter._coeffs = ml::Lopass::makeCoeffs(amount, 1.0f);
	return channel->write.filter(in);
}

[[nodiscard]] inline
auto generate_frame(Channel* channel, float input, float smoothed_input, CrossfadeMode mode, int bubble, float spike, float tilt, float pitch, float xfade_size) -> float {
	FrameWriteParams write_params; 
	write_params.bubble = bubble;
	FrameReadParams read_params; 
	read_params.crossfade_size = xfade_size;
	read_params.crossfade_mode = mode;
	read_params.tilt = tilt;
	read_params.spike = spike;
	read_params.ff = pitch;
	return process(channel, write_params, read_params, input, smoothed_input);
}

inline
auto reset(Channel* c) -> void {
	c->init = 0;
	c->fade_in.active = false;
	c->xfade.active = false;
	c->write.span.buffer = c->buffers[0].data();
	c->write.span.size = 0;
	c->write.counter = 0;
	c->write.filter.clear();
	c->write.up = false;
	c->stage.span.buffer = c->buffers[1].data();
	c->stage.span.size = 0;
	c->source.span.buffer = c->buffers[2].data();
	c->source.span.size = 0;
	c->target.span.buffer = c->buffers[3].data();
	c->target.span.size = 0;
}

inline
auto init(Channel* channel, int SR) -> void {
	for (int i = 0; i < 4; i++) {
		channel->buffers[i].resize(SR);
	}
}

} // wavebender
} // snd
