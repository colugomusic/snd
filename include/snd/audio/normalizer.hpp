#pragma once

#include <cmath>

namespace snd {
namespace audio {

[[nodiscard]] inline
auto find_max_energy(const float* frames, size_t n) -> float {
	float max = 0.0f;
	for (size_t i = 0; i < n; ++i) {
		const auto value = std::abs(frames[i]);
		if (value > max) {
			max = value;
		}
	}
	return max;
}

} // audio
} // snd