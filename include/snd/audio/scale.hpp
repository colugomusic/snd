#pragma once

#include "../flags.hpp"
#include "../misc.hpp"
#include <bitset>
#include <cstdint>
#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {

struct scale {
	std::bitset<12> value;
};

template <size_t N> [[nodiscard]] inline
auto find_nth_set_bit(std::bitset<N> bits, int n) -> int {
	int count = 0;
	for (int i = 0; i < N; i++) {
		if (bits.test(i)) {
			if (count == n) {
				return i;
			}
			count++;
		}
	}
	return -1;
}

[[nodiscard]] inline
auto set(snd::scale scale, size_t bit) -> snd::scale {
	scale.value.set(bit);
	return scale;
}

[[nodiscard]] inline
auto unset(snd::scale scale, size_t bit) -> snd::scale {
	scale.value.reset(bit);
	return scale;
}

[[nodiscard]] inline
auto test(snd::scale scale, size_t bit) -> bool {
	return scale.value.test(bit);
}

[[nodiscard]] inline
auto snap(float pitch, snd::scale scale) -> float {
	if (scale.value == 0) {
		return pitch;
	}
	static constexpr auto LENGTH = 12;
	const auto octaves       = int(std::floor(pitch / LENGTH));
	const auto octave_offset = (octaves * LENGTH);
	const auto note          = int(wrap(pitch, float(LENGTH)));
	const auto note_bit      = size_t(note);
	if (test(scale, note_bit)) {
		return float(note + octave_offset);
	}
	int offset = 1;
	for (int i = 0; i < LENGTH / 2; i++) {
		auto check_note     = wrap(note + offset, LENGTH);
		auto check_note_bit = size_t(check_note);
		if (test(scale, check_note_bit)) {
			return float(check_note + octave_offset);
		}
		check_note     = wrap(note - offset, LENGTH);
		check_note_bit = size_t(check_note);
		if (test(scale, check_note_bit)) {
			return float(check_note + octave_offset);
		}
		offset++;
	}
	return pitch;
}

[[nodiscard]] inline
auto snap(ml::DSPVector pitch, snd::scale scale) -> ml::DSPVector {
	for (int i = 0; i < kFloatsPerDSPVector; i++) {
		pitch[i] = snap(pitch[i], scale);
	}
	return pitch;
}

} // snd
