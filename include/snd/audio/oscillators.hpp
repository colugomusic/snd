#pragma once

#include "../const_math.hpp"

namespace snd {
namespace osc {
namespace scalar {

static constexpr auto MULTIWAVE_SINE     = 0.0f;
static constexpr auto MULTIWAVE_TRIANGLE = 1.0f / 3.0f;
static constexpr auto MULTIWAVE_PULSE    = 2.0f / 3.0f;
static constexpr auto MULTIWAVE_SAW      = 1.0f;
static constexpr auto PHASOR_STEPS_PER_CYCLE = ::const_math::pow(2.0f, 32);
static constexpr auto PHASOR_MIDPOINT        = PHASOR_STEPS_PER_CYCLE / 2;
static constexpr auto PHASOR_CYCLES_PER_STEP = 1 / PHASOR_STEPS_PER_CYCLE;
static constexpr auto PHASOR_MIDPOINT_INT    = static_cast<uint32_t>(PHASOR_MIDPOINT);

struct Phasor {
	uint32_t phase = 0;
	float sync_out = -1.0f;
};

namespace detail {

inline
auto update_phase(Phasor* p, uint32_t inc, float sync_in) -> void {
	if (sync_in > 0.0f) {
		p->phase = PHASOR_MIDPOINT_INT + uint32_t(sync_in * inc);
	}
	else {
		p->phase += inc;
	}
}

inline
auto update_sync_value(Phasor* p, uint32_t prev_phase, uint32_t inc) -> void {
	if (prev_phase < PHASOR_MIDPOINT && p->phase >= PHASOR_MIDPOINT) {
		p->sync_out = float(p->phase - PHASOR_MIDPOINT) / inc;
		return;
	}
	p->sync_out = -1.0f;
}

} // detail

inline
auto reset(Phasor* p) -> void {
	p->phase = 0;
}

[[nodiscard]] inline
auto process(Phasor* p, float cycles_per_frame, float sync_in = -1.0f) -> float {
	const auto steps_per_sample     = cycles_per_frame * PHASOR_STEPS_PER_CYCLE;
	const auto inc                  = static_cast<uint32_t>(steps_per_sample);
	const auto prev_phase           = p->phase;
	update_phase(p, inc, sync_in);
	update_sync_value(p, prev_phase, inc);
	return static_cast<float>(p->phase) * PHASOR_CYCLES_PER_STEP;
}

[[nodiscard]] inline
auto polyblep(float phase, float freq) -> float {
	auto t = phase;
	const auto dt = freq;
	auto c = 0.0f;
	if (t < dt) {
		t = t / dt;
		c = t + t - t * t - 1.0f;
	}
	else if (t > 1.0f - dt) {
		t = (t - 1.0f) / dt;
		c = t * t + t + t + 1.0f;
	} 
	return c;
}

struct Oscillator {
	Phasor phasor;
	float value = 0.0f;
};

struct SineOsc : Oscillator {};
struct TriangleOsc : Oscillator {};
struct PulseOsc : Oscillator {};
struct SawOsc : Oscillator {};
struct MultiWaveOsc : Oscillator {};

[[nodiscard]] inline
auto phase_to_sine(float phase) -> float {
	constexpr auto sqrt2       = static_cast<float>(const_math::sqrt(2.0f));
	constexpr auto range       = sqrt2 - sqrt2 * sqrt2 * sqrt2 / 6.0f;
	constexpr auto scale       = 1.0f / range;
	constexpr auto domain      = sqrt2 * 4.0f;
	constexpr auto flip_offset = sqrt2 * 2.0f;
	constexpr auto one_sixth   = 1.0f / 6.0f;
	const auto omega           = phase * (domain) + (-sqrt2);
	const auto triangle        = omega > sqrt2 ? flip_offset - omega : omega;
	return scale * triangle * (1.0f - triangle * triangle * one_sixth);
}

[[nodiscard]] inline
auto phase_to_triangle(float phase) -> float {
	const auto omega    = 2.0f * phase;
	const auto triangle = phase > 0.5f ? 2.0f - omega : omega;
	return (2.0f * triangle) - 1.0f;
}

[[nodiscard]] inline
auto phase_to_pulse(float phase, float freq, float width) -> float {
	float dummy;
	const auto maskV = phase > width;
	auto pulse = phase > width ? -1.0f : 1.0f;
	pulse += polyBLEP(phase, freq);
	const auto down = std::modf(phase - width + 1.0f, &dummy);
	pulse -= polyBLEP(down, freq);
	return pulse;
}

[[nodiscard]] inline
auto phase_to_saw(float phase, float freq) -> void {
	const auto saw = phase * 2.0f - 1.0f;
	return saw - polyBLEP(phase, freq);
}

[[nodiscard]] inline
auto phase_to_multiwave(float phase, float freq, float width, float wave) -> float {
	if (wave < MultiWaveOsc::MULTIWAVE_TRIANGLE) {
		const auto triangle = phase_to_triangle(phase);
		const auto sine = phase_to_sine(phase); 
		const auto xfade = math::inverse_lerp(MultiWaveOsc::MULTIWAVE_SINE, MultiWaveOsc::MULTIWAVE_TRIANGLE, wave); 
		return math::lerp(sine, triangle, xfade);
	} 
	if (wave < MultiWaveOsc::MULTIWAVE_PULSE) {
		const auto triangle = phase_to_triangle(phase);
		const auto pulse = phase_to_pulse(phase, freq, width); 
		const auto xfade = math::inverse_lerp(MultiWaveOsc::MULTIWAVE_TRIANGLE, MultiWaveOsc::MULTIWAVE_PULSE, wave); 
		return math::lerp(triangle, pulse, xfade);
	} 
	const auto pulse = phase_to_pulse(phase, freq, width);
	const auto saw = phase_to_saw(phase, freq); 
	const auto xfade = math::inverse_lerp(MultiWaveOsc::MULTIWAVE_PULSE, MultiWaveOsc::MULTIWAVE_SAW, wave); 
	return math::lerp(pulse, saw, xfade);
}

[[nodiscard]] inline
auto process_sine_osc(Phasor* p, float freq, float sync = -1.0f) -> float {
	return phase_to_sine(process(p, freq, sync));
}

[[nodiscard]] inline
auto process_triangle_osc(Phasor* p, float freq, float sync = -1.0f) -> float {
	return phase_to_triangle(process(p, freq, sync));
}

[[nodiscard]] inline
auto process_pulse_osc(Phasor* p, float freq, float width, float sync = -1.0f) -> float {
	return phase_to_pulse(process(p, freq, sync), freq, width);
}

[[nodiscard]] inline
auto process_saw_osc(Phasor* p, float freq, float sync = -1.0f) -> float {
	return phase_to_saw(process(p, freq, sync), freq);
}

[[nodiscard]] inline
auto process_multiwave_osc(Phasor* p, float freq, float width, float wave, float sync = -1.0f) -> float {
	return phase_to_multiwave(process(p, freq, sync), freq, width, wave);
}

[[nodiscard]] inline
auto process(SineOsc* osc, float freq, float sync = -1.0f) -> float {
	return osc->value = process_sine_osc(&osc->phasor, freq, sync);
}

[[nodiscard]] inline
auto process(TriangleOsc* osc, float freq, float sync = -1.0f) -> float {
	return osc->value = process_triangle_osc(&osc->phasor, freq, sync);
}

[[nodiscard]] inline
auto process(PulseOsc* osc, float freq, float width, float sync = -1.0f) -> float {
	return osc->value = process_pulse_osc(&osc->phasor, freq, width, sync);
}

[[nodiscard]] inline
auto process(SawOsc* osc, float freq, float sync = -1.0f) -> float {
	return osc->value = process_saw_osc(&osc->phasor, freq, sync);
}

[[nodiscard]] inline
auto process(MultiWaveOsc* osc, float freq, float width, float wave, float sync = -1.0f) -> float {
	return osc->value = process_multiwave_osc(&osc->phasor, freq, width, wave, sync);
}

inline auto reset(SineOsc* osc) -> void { reset(osc->phasor); } 
inline auto reset(TriangleOsc* osc) -> void { reset(osc->phasor); } 
inline auto reset(PulseOsc* osc) -> void { reset(osc->phasor); }
inline auto reset(SawOsc* osc) -> void { reset(osc->phasor); }
inline auto reset(MultiWaveOsc* osc) -> void { reset(osc->phasor); }

} // scalar
} // osc
} // snd