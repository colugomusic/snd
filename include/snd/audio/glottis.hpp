#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPFilters.h>
#include <DSP/MLDSPGens.h>
#include <DSP/MLDSPOps.h>
#pragma warning(pop)
#include "../convert.hpp"
#include "../flags.hpp"
#include "../misc.hpp"
#include "../simplex_noise.hpp"

namespace snd {
namespace audio{
namespace glottis {

struct input_flags {
	enum e {
		auto_attack,
	};
	int value = 0;
};

struct input {
	input_flags flags;
	float pitch = 0.0f;
	float buzz = 0.0f;
	ml::DSPVector aspirate_noise;
};

struct waveform {
	float te;
	float epsilon;
	float shift;
	float delta;
	float alpha;
	float omega;
	float e0;
};

struct dsp {
	glottis::waveform waveform;
	float total_time       = 0.0f;
	float time_in_waveform = 0.0f;
	float old_frequency    = 140.0f;
	float new_frequency    = 140.0f;
	float old_tenseness    = 0.6f;
	float new_tenseness    = 0.6f;
	float intensity        = 0.0f;
};

struct output {
	ml::DSPVector aspiration;
	ml::DSPVector voice;
	ml::DSPVector noise_modulator;
};

namespace detail {

static constexpr auto LAMBDA_0_1_FN(int i) -> float { return i / static_cast<float>(kFloatsPerDSPVector); }
static constexpr auto LAMBDA_0_63_FN(int i) -> float { return float(i); }
static constexpr auto LAMBDA_1_64_FN(int i) -> float { return float(i + 1); }
static constexpr auto RAMP_0_1  = ml::DSPVector{LAMBDA_0_1_FN};
static constexpr auto RAMP_0_63 = ml::DSPVector{LAMBDA_0_63_FN};
static constexpr auto RAMP_1_64 = ml::DSPVector{LAMBDA_1_64_FN};

[[nodiscard]] inline
auto make_waveform(float frequency, float tenseness, float time_in_waveform, float lambda) -> glottis::waveform {
	glottis::waveform out;
	const auto rd = std::clamp(3.0f * (1.0f - tenseness), 0.5f, 2.7f);
	const auto ra = -0.01f + 0.048f * rd;
	const auto rk = 0.224f + 0.118f * rd;
	const auto rg = (rk / 4.0f) * (0.5f + 1.2f * rk) / (0.11f * rd - ra * (0.5f + 1.2f * rk));
	const auto ta = ra;
	const auto tp = 1.0f / (2.0f * rg);
	out.te = tp + tp * rk;
	out.epsilon = 1.0f / ta;
	out.shift = std::exp(-out.epsilon * (1.0f - out.te));
	out.delta = 1.0f - out.shift;
	const auto rhs_integral = ((1.0f / out.epsilon) * (out.shift - 1.0f) + (1.0f - out.te) * out.shift) / out.delta;
	const auto total_lower_integral = -(out.te - tp) / 2.0f + rhs_integral;
	const auto total_upper_integral = -total_lower_integral;
	out.omega = float(M_PI) / tp;
	const auto s = std::sin(out.omega * out.te);
	const auto y = -float(M_PI) * s * total_upper_integral / (tp * 2.0f);
	const auto z = std::log(y);
	out.alpha = z / (tp / 2.0f - out.te);
	out.e0 = -1.0f / (s * std::exp(out.alpha * out.te));
	return out;
}

[[nodiscard]] inline
auto normalized_lf_waveform(const glottis::waveform& waveform, float t) -> float {
	if (t > waveform.te) { return (-std::exp(-waveform.epsilon * (t - waveform.te)) + waveform.shift) / waveform.delta; }
	else                 { return waveform.e0 * std::exp(waveform.alpha * t) * std::sin(waveform.omega * t); }
}

[[nodiscard]] inline
auto noise_modulator_step(float intensity, float tenseness, float time_in_waveform, float waveform_length) -> float {
	const auto voiced = 0.1f + 0.2f * std::max(0.0f, std::sin(float(M_PI) * 2.0f * time_in_waveform / waveform_length));
	return tenseness * intensity * voiced + (1.0f - tenseness * intensity) * 0.3f;
}

struct step_output {
	float voice;
	float noise_mod;
};

[[nodiscard]] inline
auto step(glottis::dsp* dsp, float frequency, float tenseness, float time_step, float lambda) -> step_output {
	const auto waveform_length = 1.0f / frequency;
	dsp->time_in_waveform += time_step;
	dsp->total_time       += time_step;
	if (dsp->time_in_waveform > waveform_length) {
		dsp->time_in_waveform -= waveform_length;
		dsp->waveform = make_waveform(frequency, tenseness, dsp->time_in_waveform, lambda);
	}
	step_output out;
	const auto t   = dsp->time_in_waveform / waveform_length;
	out.noise_mod  = noise_modulator_step(dsp->intensity, tenseness, dsp->time_in_waveform, waveform_length);
	out.voice      = normalized_lf_waveform(dsp->waveform, t);
	return out;
}

[[nodiscard]] inline
auto calculate_ui_tenseness(float pitch, float buzz) -> float {
	float out = std::clamp((((-pitch) + 24.0f) / 60.0f), 0.0f, 1.0f);
	if (buzz > 0.5f) {
		out = snd::lerp(out, 1.0f, (buzz - 0.5f) * 2.0f);
	}
	else {
		out = snd::lerp(0.0f, out, buzz * 2.0f);
	} 
	return out;
}

[[nodiscard]] inline
auto calculate_new_frequency(float ui_frequency, float old_frequency) -> float {
	if (ui_frequency > old_frequency) {
		return std::min(old_frequency * 1.1f, ui_frequency);
	}
	if (ui_frequency < old_frequency) {
		return std::max(old_frequency / 1.1f, ui_frequency);
	}
	return old_frequency;
}

[[nodiscard]] inline
auto calculate_new_tenseness(float ui_tenseness, float old_tenseness, float intensity, float total_time, float speed) -> float {
	auto out = ui_tenseness + 0.1f * simplex::noise1D(total_time * 0.46f) + 0.05f * simplex::noise1D(total_time * 0.36f);
	out     += ((3.0f - ui_tenseness) * speed) * (1.0f - intensity);
	return out;
}

} // detail

auto reset(glottis::dsp* dsp) -> void {
	*dsp = {};
	dsp->waveform = detail::make_waveform(0.0f, 0.0f, 0.0f, 0.0f);
}

[[nodiscard]] inline
auto make_dsp() -> glottis::dsp {
	glottis::dsp out;
	reset(&out);
	return out;
}

auto process(glottis::dsp* dsp, const glottis::input& input, float SR, float speed) -> output {
	const auto ui_frequency = snd::convert::pitch_to_frequency(input.pitch + 60.0f);
	const auto ui_tenseness = detail::calculate_ui_tenseness(input.pitch, input.buzz);
	const auto loudness     = std::pow(ui_tenseness, 0.25f);
	const auto auto_attack  = is_flag_set(input.flags, input.flags.auto_attack);
	const auto intensity    = auto_attack ? dsp->intensity : 1.0f;
	const auto frequency    = ml::lerp({dsp->old_frequency}, {dsp->new_frequency}, detail::RAMP_0_1);
	const auto tenseness    = ml::lerp({dsp->old_tenseness}, {dsp->new_tenseness}, detail::RAMP_0_1);
	glottis::output out;
	ml::DSPVector total_time;
	for (int i = 0; i < kFloatsPerDSPVector; i++) {
		const auto result      = detail::step(dsp, frequency[i], tenseness[i], 1.0f / float(SR), detail::RAMP_0_1[i]);
		out.voice[i]           = result.voice;
		out.noise_modulator[i] = result.noise_mod;
		total_time[i]          = dsp->total_time;
	}
	out.voice *= dsp->intensity * loudness;
	out.aspiration = intensity * (1.0f - std::sqrt(ui_tenseness)) * out.noise_modulator * input.aspirate_noise;
	out.aspiration *= 0.2f + 0.02f * simplex::noise1D(total_time * 1.99f);
	dsp->old_frequency = dsp->new_frequency;
	dsp->new_frequency = detail::calculate_new_frequency(ui_frequency, dsp->old_frequency);
	dsp->old_tenseness = dsp->new_tenseness;
	dsp->new_tenseness = detail::calculate_new_tenseness(ui_tenseness, dsp->old_tenseness, dsp->intensity, dsp->total_time, speed);
	dsp->intensity = std::clamp(dsp->intensity + (0.13f * speed), 0.0f, 1.0f);
	return out;
}

} // glottis
} // audio
} // snd
