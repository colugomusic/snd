#pragma once

namespace snd {

constexpr float DENORMAL_DC = 1e-25f;

[[nodiscard]] inline
auto flush_denormal_to_zero(float x) -> float {
	x += DENORMAL_DC;
	x -= DENORMAL_DC;
	return x;
}

} // snd
