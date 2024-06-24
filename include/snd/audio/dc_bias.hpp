#pragma once

#include "../ease.hpp"
#include "../misc.hpp"
#include <limits>
#include <vector>

namespace snd {
namespace audio {
namespace dc_bias {

struct frames {
	float* floats = nullptr;
	size_t size   = 0;
};

struct const_frames {
	const float* floats = nullptr;
	size_t size         = 0;
};

struct detection {
	std::vector<float> frame_values;
	std::vector<float> window_midpoints;
};

namespace detail {

struct detection_window {
	size_t beg = 0;
	size_t end = 0;
};

[[nodiscard]] inline
auto expand_window(size_t frame_count, size_t window_size, detection_window window) -> detection_window {
	window.beg = size_t(std::max(0, int(window.beg) - int(window_size / 2)));
	window.end = std::min(frame_count, window.end + (window_size / 2));
	return window;
}

[[nodiscard]] inline
auto detect_midpoint(dc_bias::const_frames frames, size_t window_size, detection_window window) -> float {
	float min = std::numeric_limits<float>::max();
	float max = std::numeric_limits<float>::lowest();
	// i think this actually makes things worse but it's an interesting idea
	// window = expand_window(frames.size, window_size, window);
	for (size_t i = window.beg; i < window.end; i++) {
		const auto value = frames.floats[i];
		if (value < min) { min = value; }
		if (value > max) { max = value; }
	}
	return (min + max) * 0.5f;
}

inline
auto detect_midpoints(dc_bias::const_frames frames, size_t window_size, std::vector<float>* out) -> void {
	out->clear();
	detection_window window;
	for (;;) {
		if (window.beg >= frames.size) {
			return;
		}
		window.end = std::min(frames.size, window.beg + window_size);
		if (window.end > window.beg) {
			const auto midpoint = detect_midpoint(frames, window_size, window);
			out->push_back(midpoint);
		}
		window.beg = window.end;
	}
}

[[nodiscard]] inline
auto get_window_center(size_t window_size, size_t window_count, size_t frame_count, size_t window_index) -> size_t {
	auto center = (window_index * window_size) + (window_size / 2);
	if (center >= frame_count) {
		center = ((window_index * window_size) + frame_count) / 2;
	}
	return center;
}

inline
auto generate_frame_values(const std::vector<float>& window_midpoints, size_t window_size, std::vector<float>* out) -> void {
	if (window_midpoints.empty()) {
		return;
	}
	if (window_midpoints.size() == 1) {
		std::fill(out->begin(), out->end(), window_midpoints[0]);
		return;
	}
	const auto window_count = window_midpoints.size();
	const auto frame_count  = out->size();
	// Beginning
	{
		const auto frame_beg = 0;
		const auto frame_end = window_size / 2;
		const auto value     = window_midpoints.front();
		std::fill(out->begin() + frame_beg, out->begin() + frame_end, value);
	}
	// Middle parts
	for (size_t index = 0; index < window_count - 1; index++) {
		const auto window_a_midpoint = window_midpoints[index + 0];
		const auto window_b_midpoint = window_midpoints[index + 1];
		const auto frame_beg = get_window_center(window_size, window_count, frame_count, index);
		const auto frame_end = get_window_center(window_size, window_count, frame_count, index + 1);
		for (size_t frame_index = frame_beg; frame_index < frame_end; frame_index++) {
			const auto t         = static_cast<float>(frame_index - frame_beg) / static_cast<float>(window_size);
			const auto value     = lerp(window_a_midpoint, window_b_midpoint, ease::quadratic::in_out(t));
			const auto abs_value = std::abs(value);
			const auto amount    = abs_value * abs_value * abs_value;
			(*out)[frame_index] = value * amount;
		}
	}
	// End
	{
		const auto frame_beg = get_window_center(window_size, window_count, frame_count, window_count - 1);
		const auto frame_end = frame_count;
		const auto value     = window_midpoints.back();
		std::fill(out->begin() + frame_beg, out->begin() + frame_end, value);
	}
}

} // detail

inline
auto detect(dc_bias::const_frames frames, size_t window_size, dc_bias::detection* out) -> void {
	out->frame_values.resize(frames.size);
	detail::detect_midpoints(frames, window_size, &out->window_midpoints);
	detail::generate_frame_values(out->window_midpoints, window_size, &out->frame_values);
}

inline
auto apply_correction(dc_bias::frames frames, const dc_bias::detection& detection) -> void {
	for (size_t i = 0; i < frames.size; i++) {
		frames.floats[i] -= detection.frame_values[i];
	}
}

} // dc_bias
} // audio
} // snd