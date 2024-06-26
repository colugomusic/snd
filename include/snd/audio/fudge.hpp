#pragma once

#include "../ease.hpp"
#include "../misc.hpp"
#include "autocorrelation.hpp"
#include <array>
#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace fudge {

struct frame_flags {
	enum e {
		play  = 1 << 0,
		reset = 1 << 1,
	};
	int value = 0;
};

enum class channel_mode { stereo, stereo_swap, left, right, };
struct amp { ml::DSPVector value; };
struct channel_count { uint8_t value; };
struct ff { ml::DSPVector value; };
struct frame_increment { float value; };
struct harmonic_ratio { ml::DSPVector value; };
struct sample_frame_count { size_t value; };
struct sample_position { snd::transport::DSPVectorFramePosition value; };
struct size { ml::DSPVector value; };
struct spread { ml::DSPVector value; };
struct stereo_frame { float L = 0.0f; float R = 0.0f; };
struct SR { float value; };
struct uniformity { ml::DSPVector value; };

struct vector_info {
	fudge::amp amp;
	fudge::channel_count channel_count;
	fudge::channel_mode channel_mode;
	fudge::ff ff;
	fudge::frame_increment frame_increment;
	fudge::harmonic_ratio harmonic_ratio;
	fudge::sample_frame_count sample_frame_count;
	fudge::sample_position sample_position;
	fudge::size size;
	fudge::SR SR;
	fudge::uniformity uniformity;
	const poka::output* analysis = nullptr;
};

struct frame_info {
	fudge::frame_flags flags;
	int idx;
};

struct flip_flop { uint8_t value = 0; };

struct grain {
	bool fade_in    = true;
	bool on         = false;
	float duck      = 1.0f;
	float ff        = 1.0f;
	float frame     = 0.0f;
	float frame_amp = 0.0f;
	float size      = 1024.0f;
	float window    = 256.0f;
	std::array<float, 2> beg;
	grain() {
		beg[0] = 0.0f;
		beg[1] = 0.0f;
	}
};

struct particle {
	bool trig_primed    = false;
	float trigger_timer = 0.0f;
	fudge::flip_flop flip_flop;
	std::array<fudge::grain, 2> grains;
};

namespace detail {

template <typename Mask> [[nodiscard]]
auto is_flag_set(Mask mask, typename Mask::e flag) -> bool {
	return bool(mask.value & flag);
}

[[nodiscard]] inline
auto other_grain(particle* p, size_t grain_idx) -> grain& {
	return p->grains[1 - grain_idx];
}

[[nodiscard]] inline
auto flip(fudge::flip_flop x) -> fudge::flip_flop {
	return {uint8_t(1 - x.value)};
}

[[nodiscard]] inline
auto adjusted_channel_pos(fudge::particle* p, const fudge::vector_info& v, fudge::frame_info f, int channel) -> float {
	const auto pos = float(v.sample_position.value[f.idx]);
	const auto adjust_amount = 1.0f - v.uniformity.value[f.idx];
	if (adjust_amount < 0.000001f) return pos; 
	if (pos > v.sample_frame_count.value) return pos; 
	if (!v.analysis) return pos;
	const auto& other = other_grain(p, p->flip_flop.value);
	const auto other_pos = other.beg[channel];
	if (other_pos + other.frame < 0.0f) return pos;
	const auto other_pos_floor = int(std::floor(other_pos + other.frame));
	const auto diff = pos - (other_pos + other.frame);
	const auto abs_diff = std::abs(diff);
	const auto& channel_analysis_data = v.analysis[channel].frames.estimated_size;
	const auto other_wavecycle =
		other_pos_floor >= channel_analysis_data.size()
			? channel_analysis_data[channel_analysis_data.size() - 1]
			: channel_analysis_data[other_pos_floor];
	const auto a = int(std::floor(float(abs_diff) / other_wavecycle));
	float adjusted_pos;
	if (diff > 0.0f) {
		adjusted_pos = (other_pos + other.frame) + (a * other_wavecycle);
	}
	else {
		adjusted_pos = (other_pos + other.frame) - (a * other_wavecycle);
	}
	if (adjusted_pos > v.sample_frame_count.value) {
		adjusted_pos = pos;
	}
	return lerp(pos, adjusted_pos, adjust_amount);
}

[[nodiscard]] inline
auto get_stereo_positions(fudge::particle* p, const fudge::vector_info& v, fudge::frame_info f, bool adjust) -> stereo_frame {
	const auto pos = float(v.sample_position.value[f.idx]);
	if (!adjust) {
		return { pos, pos };
	}
	if (pos <= 0.0f) {
		return { pos, pos };
	}
	stereo_frame out;
	switch (v.channel_mode) {
		case channel_mode::stereo: {
			out.L = adjusted_channel_pos(p, v, f, 0);
			out.R = adjusted_channel_pos(p, v, f, 1);
			break;
		} 
		case channel_mode::left: {
			out.L = adjusted_channel_pos(p, v, f, 0);
			out.R = adjusted_channel_pos(p, v, f, 0);
			break;
		} 
		case channel_mode::right: {
			out.L = adjusted_channel_pos(p, v, f, 1);
			out.R = adjusted_channel_pos(p, v, f, 1);
			break;
		}
	} 
	if (std::abs(out.R - out.L) < 0.01f * v.SR.value) {
		out.R = out.L;
	} 
	return out;
}

[[nodiscard]] inline
auto get_mono_position(fudge::particle* p, const fudge::vector_info& v, fudge::frame_info f, bool adjust) -> float {
	const auto pos = float(v.sample_position.value[f.idx]);
	if (!adjust) {
		return pos;
	}
	if (pos <= 0.0f) {
		return pos; 
	}
	return adjusted_channel_pos(p, v, f, 0);
}

inline
auto trigger_next_grain(fudge::particle* p, const fudge::vector_info& v, fudge::frame_info f, bool adjust) -> void {
	constexpr auto MIN_GRAIN_SIZE = 3.0f;
	constexpr auto MAX_WINDOW_SIZE = 4096.0f;
	p->flip_flop       = flip(p->flip_flop);
	const auto fade_in = v.sample_position.value[f.idx] > 0;
	stereo_frame beg;
	if (v.channel_count.value > 1) {
		beg = get_stereo_positions(p, v, f, adjust);
	}
	else {
		beg.L = get_mono_position(p, v, f, adjust);
		beg.R = beg.L;
	}
	auto ratio = v.harmonic_ratio.value[f.idx];
	auto ff    = v.ff.value[f.idx];
	auto size  = std::max(MIN_GRAIN_SIZE, v.size.value[f.idx] * ff);
	p->grains[p->flip_flop.value].on        = true;
	p->grains[p->flip_flop.value].fade_in   = fade_in;
	p->grains[p->flip_flop.value].beg[0]    = beg.L;
	p->grains[p->flip_flop.value].beg[1]    = beg.R;
	p->grains[p->flip_flop.value].ff        = v.frame_increment.value * ff * ratio;
	p->grains[p->flip_flop.value].size      = size;
	p->grains[p->flip_flop.value].window    = std::min(MAX_WINDOW_SIZE, std::floor(size / 3));
	p->grains[p->flip_flop.value].frame     = 0.f;
	p->grains[p->flip_flop.value].frame_amp = fade_in ? 0.f : 1.f;
	p->grains[p->flip_flop.value].duck      = 1.f;
}

inline
auto reset(fudge::particle* p, const fudge::vector_info& v, fudge::frame_info f) -> void {
	p->grains[0].on = false;
	p->grains[1].on = false;
	p->trig_primed = false;
	p->trigger_timer = 0.0f;
	trigger_next_grain(p, v, f, false);
}

template <typename ReadSampleFn> [[nodiscard]]
auto read_mono_frame(const fudge::particle& p, ReadSampleFn read_sample_fn, const fudge::grain& grain) -> float {
	const auto pos = grain.beg[0] + grain.frame;
	return pos < 0.f ? 0.f : read_sample_fn(0, pos);
}

template <typename ReadSampleFn> [[nodiscard]]
auto read_stereo_frame(const fudge::particle& p, const fudge::vector_info& v, ReadSampleFn read_sample_fn, const fudge::grain& grain) -> stereo_frame {
	stereo_frame out;
	switch (v.channel_mode) {
		default:
		case channel_mode::stereo: {
			const auto pos_L = grain.beg[0] + grain.frame;
			const auto pos_R = grain.beg[1] + grain.frame; 
			out.L = pos_L < 0.f ? 0.f : read_sample_fn(0, pos_L);
			out.R = pos_R < 0.f ? 0.f : read_sample_fn(1, pos_R); 
			break;
		} 
		case channel_mode::stereo_swap: {
			const auto pos_L = grain.beg[0] + grain.frame;
			const auto pos_R = grain.beg[1] + grain.frame; 
			out.R = pos_L < 0.f ? 0.f : read_sample_fn(0, pos_L);
			out.L = pos_R < 0.f ? 0.f : read_sample_fn(1, pos_R); 
			break;
		} 
		case channel_mode::left: {
			const auto pos = grain.beg[0] + grain.frame; 
			out.L = pos < 0.f ? 0.f : read_sample_fn(0, pos);
			out.R = out.L; 
			break;
		} 
		case channel_mode::right: {
			const auto pos = grain.beg[1] + grain.frame; 
			out.R = pos < 0.f ? 0.f : read_sample_fn(1, pos);
			out.L = out.R; 
			break;
		}
	} 
	return out;
}

} // detail

template <typename ReadSampleFn> [[nodiscard]]
auto process(fudge::particle* p, const fudge::vector_info& v, fudge::frame_info f, ReadSampleFn read_sample_fn) -> stereo_frame {
	stereo_frame out;
	if (p->trig_primed && detail::is_flag_set(f.flags, f.flags.play)) {
		detail::reset(p, v, f);
	}
	else {
		if (detail::is_flag_set(f.flags, f.flags.reset)) {
			if (!detail::is_flag_set(f.flags, f.flags.play)) {
				p->trig_primed = true;
			}
			else {
				detail::reset(p, v, f);
			}
		}
		else if(p->trigger_timer >= std::floor(p->grains[p->flip_flop.value].size / 2)) {
			p->trigger_timer = 0.f;
			detail::trigger_next_grain(p, v, f, true);
		}
	}
	for (size_t grain_idx = 0; grain_idx < 2; grain_idx++) {
		stereo_frame grain_out;
		auto& grain = p->grains[grain_idx]; 
		if (!grain.on) {
			continue; 
		}
		if (grain.frame < grain.window) {
			if (grain.fade_in) {
				grain.frame_amp = ease::quadratic::in_out(1.f - ((grain.window - grain.frame) / grain.window)); 
				float other_grain_duck = 1.f - grain.frame_amp; 
				detail::other_grain(p, grain_idx).duck = other_grain_duck;
			}
		} 
		float self_duck = 1.f; 
		if (grain.frame > grain.size - grain.window) {
			self_duck = ease::quadratic::in_out(1.f - ((grain.frame - (grain.size - grain.window)) / grain.window));
		} 
		auto final_duck = std::min(grain.duck, self_duck);
		auto overall_amp = grain.frame_amp * final_duck * v.amp.value[f.idx];
		if (overall_amp > 0.f) {
			if (v.channel_count.value > 1) {
				grain_out = detail::read_stereo_frame(*p, v, read_sample_fn, grain); 
			}
			else {
				grain_out.L = detail::read_mono_frame(*p, read_sample_fn, grain);
				grain_out.R = grain_out.L;
			} 
			grain_out.L *= overall_amp;
			grain_out.R *= overall_amp;
		} 
		grain.frame += grain.ff; 
		if (grain.frame >= grain.size) {
			grain.on = false;
		} 
		out.L += grain_out.L;
		out.R += grain_out.R;
	} 
	p->trigger_timer += p->grains[p->flip_flop.value].ff;
	return out;
}

} // fudge
} // snd