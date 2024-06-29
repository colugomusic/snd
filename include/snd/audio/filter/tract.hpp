#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace filter {
namespace tract {

namespace detail {

static inline constexpr auto BLADE_START              = 10;
static inline constexpr auto GRID_OFFSET              = 1.7f;
static inline constexpr auto LIP_START                = 39;
static inline constexpr auto MOVEMENT_SPEED           = 15;
static inline constexpr auto N                        = 44;
static inline constexpr auto NOSE_LENGTH              = 28;
static inline constexpr auto NOSE_OFFSET              = 0.8f;
static inline constexpr auto NOSE_START               = N - NOSE_LENGTH + 1;
static inline constexpr auto TIP_START                = 32;
static inline constexpr auto TONGUE_LOWER_INDEX_BOUND = BLADE_START + 2;
static inline constexpr auto TONGUE_UPPER_INDEX_BOUND = TIP_START - 3;
static inline constexpr auto TONGUE_INDEX_CENTER      = 0.5f * (TONGUE_LOWER_INDEX_BOUND + TONGUE_UPPER_INDEX_BOUND);

static inline constexpr float LAMBDA1_FN(int i) { return i / static_cast<float>(kFloatsPerDSPVector); }
static inline constexpr float LAMBDA2_FN(int i) { return (float(i) + 0.5f) / static_cast<float>(kFloatsPerDSPVector); }

struct step_input {
	float glottis;
	struct {
		bool active;
		float noise;
		float intensity;
	} fricatives;
	struct {
		float position;
		float diameter;
	} throat;
	struct {
		float position;
		float diameter;
	} tongue;
};

struct step_transient {
	int position;
	float time_alive;
	float life_time;
	float strength;
	float exponent;
};

struct step_transients { 
	std::array<step_transient, 32> list;
	int count = 0;
};

struct step {
	step_transients transients;
	std::array<float, N> a = {};
	std::array<float, N> diameter = {};
	std::array<float, N> rest_diameter = {};
	std::array<float, N> target_diameter = {};
	std::array<float, N> new_diameter = {};
	std::array<float, N> L = {};
	std::array<float, N> R = {};
	std::array<float, N + 1> junction_output_L = {};
	std::array<float, N + 1> junction_output_R = {};
	std::array<float, N + 1> reflection = {};
	std::array<float, N + 1> new_reflection = {};
	std::array<float, NOSE_LENGTH> nose_a = {};
	std::array<float, NOSE_LENGTH> nose_L = {};
	std::array<float, NOSE_LENGTH> nose_R = {};
	std::array<float, NOSE_LENGTH> nose_diameter = {};
	std::array<float, NOSE_LENGTH + 1> nose_junction_output_L = {};
	std::array<float, NOSE_LENGTH + 1> nose_junction_output_R = {};
	std::array<float, NOSE_LENGTH + 1> nose_reflection = {};
	int last_obstruction      = -1;
	float fade                = 1.0f;
	float reflection_L        = 0.0f;
	float reflection_R        = 0.0f;
	float reflection_nose     = 0.0f;
	float new_reflection_L    = 0.0f;
	float new_reflection_R    = 0.0f;
	float new_reflection_nose = 0.0f;
	float glottal_reflection  = 0.75f;
	float lip_reflection      = -0.85f;
	float velum_target        = 0.01f;
};

inline
auto set_rest_diameter(detail::step* step, const step_input& input) -> void {
	for (int i = BLADE_START; i < LIP_START; i++) {
		const auto t = 1.1f * float(M_PI) * (input.tongue.position - i) / (TIP_START - BLADE_START);
		const auto fixed_tongue_diameter = 2.0f + (input.tongue.diameter - 2.0f) / 1.5f; 
		auto curve = (1.5f - fixed_tongue_diameter + GRID_OFFSET) * std::cos(t); 
		if (i == BLADE_START - 2 || i == LIP_START - 1) {
			curve *= 0.8f;
		} 
		if (i == BLADE_START || i == LIP_START - 2) {
			curve *= 0.94f;
		} 
		step->rest_diameter[i] = 1.5f - curve;
	}
}

inline
auto configure(detail::step* step, const step_input& input) -> void {
	set_rest_diameter(step, input);
	step->target_diameter = step->rest_diameter; 
	step->velum_target = 0.01f; 
	if (input.throat.position > NOSE_START && input.throat.diameter < -NOSE_OFFSET) {
		step->velum_target = 0.4f;
	} 
	if (input.throat.diameter < -0.85f - NOSE_OFFSET) {
		return;
	}
	auto diameter = input.throat.diameter - 0.3f; 
	if (diameter < 0.0f) diameter = 0.0f; 
	auto width = 2.0f; 
	if (input.throat.position < 25.0f) {
		width = 10.0f;
	}
	else if (input.throat.position >= TIP_START) {
		width = 5.0f;
	}
	else {
		width = 10.0f - 5.0f * (input.throat.position - 25) / (TIP_START - 25);
	} 
	if (input.throat.position >= 2 && input.throat.position < N && diameter < 3) {
		auto int_index = int(input.throat.position); 
		for (int i = -int(std::ceil(width)) - 1; i < width + 1; i++) {
			if (int_index + i < 0 || int_index + i >= N) {
				continue;
			}
			auto rel_pos = (int_index + i) - input.throat.position; 
			rel_pos = std::abs(rel_pos) - 0.5f; 
			float shrink; 
			if (rel_pos <= 0.0f) {
				shrink = 0.0f;
			}
			else if (rel_pos > width) {
				shrink = 1.0f;
			}
			else {
				shrink = 0.5f * (1.0f - std::cos(float(float(M_PI)) * rel_pos / width));
			} 
			if (diameter < step->target_diameter[int_index + i]) {
				step->target_diameter[int_index + i] = diameter + (step->target_diameter[int_index + i] - diameter) * shrink;
			}
		}
	}
}

inline
auto process_transients(detail::step* step, int SR) -> void {
	for (int i = 0; i < step->transients.count; i++) {
		auto& transient = step->transients.list[i];
		const auto amplitude = transient.strength * std::pow(2.0f, -transient.exponent * transient.time_alive); 
		step->R[transient.position] += amplitude / 2.0f;
		step->L[transient.position] += amplitude / 2.0f; 
		transient.time_alive += 1.0f / (SR * 2.0f);
	} 
	for (int i = step->transients.count - 1; i >= 0; i--) {
		auto& transient = step->transients.list[i]; 
		if (transient.time_alive > transient.life_time) {
			step->transients.count--;
		}
	}
}

inline
auto add_turbulence_noise_at_index(detail::step* step, float noise, float index, float diameter) -> void {
	const auto i = int(std::floor(index));
	const auto delta = index - i; 
	const auto thinness = std::clamp(8.0f * (0.7f - diameter), 0.0f, 1.0f);
	const auto openness = std::clamp(30.0f * (diameter - 0.3f), 0.0f, 1.0f);
	const auto noise0 = noise * (1.0f - delta) * thinness * openness;
	const auto noise1 = noise * delta * thinness * openness; 
	step->R[i + 1] += noise0 / 2;
	step->L[i + 1] += noise0 / 2;
	step->R[i + 2] += noise1 / 2;
	step->L[i + 2] += noise1 / 2;
}

inline
auto add_turbulence_noise(detail::step* step, const step_input& input) -> void {
	if (input.throat.position < 2.0f || input.throat.position >= N - 2) return;
	if (input.throat.diameter <= 0.0f) return;
	if (input.fricatives.intensity <= 0.0001f) return; 
	add_turbulence_noise_at_index(step, 0.66f * input.fricatives.noise * input.fricatives.intensity, input.throat.position, input.throat.diameter);
}

inline
auto process(detail::step* step, int SR, float lambda, const step_input& input, float* lip_out, float* nose_out) -> void {
	configure(step, input);
	if (input.fricatives.active) {
		process_transients(step, SR);
		add_turbulence_noise(step, input);
	}
	step->junction_output_R[0] = step->L[0] * step->glottal_reflection + input.glottis;
	step->junction_output_L[N] = step->R[N - 1] * step->lip_reflection; 
	for (int i = 1; i < N; i++) {
		const auto r = step->reflection[i] * (1.0f - lambda) + step->new_reflection[i] * lambda;
		const auto w = r * (step->R[i - 1] + step->L[i]); 
		step->junction_output_R[i] = step->R[i - 1] - w;
		step->junction_output_L[i] = step->L[i] + w;
	} 
	const int i = NOSE_START; 
	auto r = step->new_reflection_L * (1.0f - lambda) + step->reflection_L * lambda;
	step->junction_output_L[i] = r * step->R[i - 1] + (1.0f + r) * (step->nose_L[0] + step->L[i]);
	r = step->new_reflection_R * (1.0f - lambda) + step->reflection_R * lambda;
	step->junction_output_R[i] = r * step->L[i] + (1.0f + r) * (step->R[i - 1] + step->nose_L[0]);
	r = step->new_reflection_nose * (1.0f - lambda) + step->reflection_nose * lambda;
	step->nose_junction_output_R[0] = r * step->nose_L[0] + (1.0f + r) * (step->L[i] + step->R[i - 1]); 
	for (int i = 0; i < N; i++) {
		step->R[i] = step->junction_output_R[i] * 0.999f;
		step->L[i] = step->junction_output_L[i + 1] * 0.999f;
	} 
	*lip_out = step->R[N - 1]; 
	step->nose_junction_output_L[NOSE_LENGTH] = step->nose_R[NOSE_LENGTH - 1] * step->lip_reflection; 
	for (int i = 1; i < NOSE_LENGTH; i++) {
		const auto w = step->nose_reflection[i] * (step->nose_R[i - 1] + step->nose_L[i]); 
		step->nose_junction_output_R[i] = step->nose_R[i - 1] - w;
		step->nose_junction_output_L[i] = step->nose_L[i] + w;
	} 
	for (int i = 0; i < NOSE_LENGTH; i++) {
		step->nose_R[i] = step->nose_junction_output_R[i] * step->fade;
		step->nose_L[i] = step->nose_junction_output_L[i + 1] * step->fade;
	} 
	*nose_out = step->nose_R[NOSE_LENGTH - 1];
}

inline
auto calculate_reflections(detail::step* step) -> void {
	for (int i = 0; i < N; i++) {
		step->a[i] = step->diameter[i] * step->diameter[i];
	} 
	for (int i = 1; i < N; i++) {
		step->reflection[i] = step->new_reflection[i]; 
		if (step->a[i] < 0.000001f) {
			step->new_reflection[i] = 0.999f;
		}
		else {
			step->new_reflection[i] = (step->a[i - 1] - step->a[i]) / (step->a[i - 1] + step->a[i]);
		}
	} 
	step->reflection_L = step->new_reflection_L;
	step->reflection_R = step->new_reflection_R;
	step->reflection_nose = step->new_reflection_nose; 
	const auto sum = step->a[NOSE_START] + step->a[NOSE_START + 1] + step->nose_a[0]; 
	step->new_reflection_L = (2.0f * step->a[NOSE_START] - sum) / sum;
	step->new_reflection_R = (2.0f * step->a[NOSE_START + 1] - sum) / sum;
	step->new_reflection_nose = (2.0f * step->nose_a[0] - sum) / sum;
}

inline
auto calculate_nose_reflections(detail::step* step) -> void {
	for (int i = 0; i < NOSE_LENGTH; i++) {
		step->nose_a[i] = step->nose_diameter[i] * step->nose_diameter[i];
	} 
	for (int i = 1; i < NOSE_LENGTH; i++) {
		step->nose_reflection[i] = (step->nose_a[i - 1] - step->nose_a[i]) / (step->nose_a[i - 1] + step->nose_a[i]);
	}
}

inline
auto reset(detail::step* step) -> void {
	for (int i = 0; i < N; i++) {
		auto diameter = 0.0f; 
		if (i < 7.0f * N / 44.0f - 0.5f) { diameter = 0.6f; }
		else if (i < 12.0f * N / 44.0f)  { diameter = 1.1f; }
		else                             { diameter = 1.0f; } 
		step->diameter[i] = step->rest_diameter[i] = step->target_diameter[i] = step->new_diameter[i] = diameter;
	}
	for (int i = 0; i < NOSE_LENGTH; i++) {
		float diameter; 
		auto d = 2.0f * (float(i) / NOSE_LENGTH); 
		if (d < 1.0f) { diameter = 0.4f + 1.6f * d; }
		else          { diameter = 0.5f + 1.5f * (2.0f - d); } 
		diameter = std::min(diameter, 1.9f); 
		step->nose_diameter[i] = diameter;
	}
	step->new_reflection_L = step->new_reflection_R = step->new_reflection_nose = 0.0f;
	calculate_reflections(step);
	calculate_nose_reflections(step);
	step->nose_diameter[0] = step->velum_target;
}

inline
auto init(detail::step* step) -> void {
	reset(step);
}

[[nodiscard]] inline constexpr
auto calculate_slow_return(int index) -> float {
	if (index < NOSE_START) {
		return 0.6f;
	}
	if (index >= TIP_START) {
		return 1.0f;
	}
	return 0.6f + 0.4f * (float(index) - NOSE_START) / (TIP_START - NOSE_START);
}

inline
auto add_transient(detail::step* step, int position, float speed) -> void {
	if (step->transients.count == step->transients.list.size()) {
		return;
	}
	step_transient t;
	t.position = position;
	t.time_alive = 0.0f;
	t.life_time = 0.2f * (1.0f / speed);
	t.strength = 0.3f;
	t.exponent = 200.0f;
	step->transients.list[step->transients.count++] = t;
}

inline
auto reshape_tract(detail::step* step, int SR, float speed, bool fricatives) -> void {
	const auto delta_time = float(kFloatsPerDSPVector) / SR;
	auto amount = delta_time * MOVEMENT_SPEED;
	auto new_last_obstruction = -1; 
	auto move_towards = [](float current, float target, float amount_up, float amount_down) {
		if (current < target) return std::min(current + amount_up, target);
		else return std::max(current - amount_down, target);
	}; 
	for (int i = 0; i < N; i++) {
		auto diameter = step->diameter[i];
		auto target_diameter = step->target_diameter[i]; 
		if (fricatives && diameter <= 0.0f) {
			new_last_obstruction = i;
		} 
		step->diameter[i] = move_towards(diameter, target_diameter, calculate_slow_return(i) * amount, 2.0f * amount);
	} 
	if (fricatives) {
		if (step->last_obstruction > -1 && new_last_obstruction == -1 && step->nose_a[0] < 0.05f) {
			add_transient(step, step->last_obstruction, speed);
		} 
		step->last_obstruction = new_last_obstruction;
	} 
	step->nose_diameter[0] = move_towards(step->nose_diameter[0], step->velum_target, amount * 0.25f, amount * 0.1f);
	step->nose_a[0] = step->nose_diameter[0] * step->nose_diameter[0];
}

} // detail

struct input {
	ml::DSPVector glottis;
	struct {
		bool active = false;
		ml::DSPVector noise;
		ml::DSPVector intensity;
	} fricatives;
	struct {
		ml::DSPVector position;
		ml::DSPVector diameter;
	} throat;
	struct {
		ml::DSPVector position;
		ml::DSPVector diameter;
	} tongue;
};

struct model {
	detail::step step;
};

[[nodiscard]] inline
auto make() -> tract::model {
	tract::model model;
	detail::init(&model.step);
	return model;
}

[[nodiscard]] inline
auto process(tract::model* model, int SR, float speed, const tract::input& input) -> ml::DSPVector {
	static constexpr auto lambda1 = ml::DSPVector(detail::LAMBDA1_FN);
	static constexpr auto lambda2 = ml::DSPVector(detail::LAMBDA2_FN);
	ml::DSPVector out;
	ml::DSPVector lip;
	ml::DSPVector nose;
	float lip_value = 0.0f;
	float nose_value = 0.0f;
	for (int i = 0; i < kFloatsPerDSPVector; i++) {
		detail::step_input step_input; 
		step_input.fricatives.active    = input.fricatives.active;
		step_input.throat.diameter      = input.throat.diameter[i];
		step_input.fricatives.intensity = input.fricatives.intensity[i];
		step_input.fricatives.noise     = input.fricatives.noise[i];
		step_input.glottis              = input.glottis[i];
		step_input.throat.position      = input.throat.position[i];
		step_input.tongue.diameter      = input.tongue.diameter[i];
		step_input.tongue.position      = input.tongue.position[i];
		detail::process(&model->step, SR, lambda1[i], step_input, &lip_value, &nose_value);
		lip[i] += lip_value;
		nose[i] += nose_value;
		detail::process(&model->step, SR, lambda2[i], step_input, &lip_value, &nose_value);
		lip[i] += lip_value;
		nose[i] += nose_value;
	}
	out = (lip + nose) / 2.0f;
	detail::reshape_tract(&model->step, SR, speed, input.fricatives.active);
	detail::calculate_reflections(&model->step);
#if _DEBUG
	ml::validate(out);
#endif
	out = ml::clamp(out, ml::DSPVector(-1.0), ml::DSPVector(1.0));
	return out;
}

inline
auto reset(tract::model* model) -> void {
	detail::reset(&model->step);
}

} // tract
} // filter
} // audio
} // snd