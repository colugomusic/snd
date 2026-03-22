#pragma once

#include <stdexcept>
#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include <cmath>
#include "const_math.hpp"

namespace snd::easing {

enum class curve {
	back,
	quadratic,
	quartic,
};

enum class mode {
	in,
	out,
	in_out,
	out_in
};

} // snd::easing

namespace snd::easing::back {

template <class T> [[nodiscard]] constexpr
auto in(T t, T strength) -> T {
	const auto s = strength;
	const auto c = s + 1;
	return c * t * t * t - s * t * t;
}

template <class T> [[nodiscard]] constexpr
auto out(T t, T strength) -> T {
	const auto s = strength;
	const auto c = s + 1;
	return T(1) + c * const_math::pow(t - 1, T(3)) + s * const_math::pow(t - 1, T(2));
}

template <class T> [[nodiscard]] constexpr
auto in_out(T t, T strength) -> T {
	auto s = strength;
	t *= T(2);
	if (t < 1) {
		s *= T(1.525);
		return T(0.5) * (t * t * ((s+1) * t - s));
	}
	t -= 2;
	s *= T(1.525);
	return T(0.5) * (t*t * ((s+1) * t + s) + 2);
}

template <class T> [[nodiscard]] constexpr
auto out_in(T t, T strength) -> T {
	if (t < T(0.5)) {
		t *= T(2);
		const auto s = strength * T(1.525);
		return T(-0.5) * (t * t * ((s+1) * t - s));
	}
	else {
		t = t * T(2) - 2;
		const auto s = strength * T(1.525);
		return T(0.5) * (t*t * ((s+1) * t + s) + 2);
	}
}

template <class T> [[nodiscard]] constexpr
auto ease(T x, easing::mode mode, T strength) -> T {
	switch (mode) {
		case easing::mode::in:     { return in(x, strength); }
		case easing::mode::out:    { return out(x, strength); }
		case easing::mode::in_out: { return in_out(x, strength); }
		case easing::mode::out_in: { return out_in(x, strength); }
		default:                   { throw std::invalid_argument("Invalid easing mode"); }
	}
}

} // snd::easing::back

namespace snd::easing::quadratic {

template <class T> [[nodiscard]] constexpr
auto in(T x) -> T {
	return x * x;
}

template <class T> [[nodiscard]] constexpr
auto out(T x) -> T {
	return (x * (x - T(2))) * T(-1);
}

template <class T> [[nodiscard]] constexpr
auto in_out(T x) -> T {
	x /= T(0.5);
	if (x < T(1)) return x * x * T(0.5);
	x--;
	return (x * (x - T(2)) - T(1)) * T(-0.5);
}

template <class T> [[nodiscard]] constexpr
auto out_in(T x) -> T {
	if (x < T(0.5)) {
		x /= T(0.5);
		return T(-0.5) * (x * (x - T(2)));
	}
	else {
		return T(2 * const_math::pow(double(x) - 0.5, 2)) + T(0.5);
	}
}

template <class T> [[nodiscard]] constexpr
auto ease(T x, easing::mode mode) -> T {
	switch (mode) {
		case easing::mode::in:     { return in(x); }
		case easing::mode::out:    { return out(x); }
		case easing::mode::in_out: { return in_out(x); }
		case easing::mode::out_in: { return out_in(x); }
		default:                   { throw std::invalid_argument("Invalid easing mode"); }
	}
}

} // snd::easing::quadratic

namespace snd::easing::quartic {

template <class T> [[nodiscard]] constexpr
auto in(T x) -> T {
	return x * x * x * x;
}

template <class T> [[nodiscard]] constexpr
auto out(T x) -> T {
	return 1 - const_math::pow(T(1) - x, 4);
}

template <class T> [[nodiscard]] constexpr
auto in_out(T x) -> T {
	return x < T(0.5) ? 8 * const_math::pow(x, 4) : 1 - 8 * const_math::pow(x - T(1), 4);
}

template <class T> [[nodiscard]] constexpr
auto out_in(T x) -> T {
	return x < T(0.5) ? 1 - 8 * const_math::pow(T(1) - 2 * x, 4) : 8 * const_math::pow(2 * x - 1, 4) + T(0.5);
}

template <class T> [[nodiscard]] constexpr
auto ease(T x, easing::mode mode) -> T {
	switch (mode) {
		case easing::mode::in:     { return in(x); }
		case easing::mode::out:    { return out(x); }
		case easing::mode::in_out: { return in_out(x); }
		case easing::mode::out_in: { return out_in(x); }
		default:                   { throw std::invalid_argument("Invalid easing mode"); }
	}
}

} // snd::easing::quartic

namespace snd::easing::parametric {

template <class T> constexpr T in_out(T x) {
	auto sqr = x * x;
	return sqr / (T(2) * (sqr - x) + T(1));
}

} // snd::easing::parametric

namespace snd {

template <class T> [[nodiscard]] constexpr
auto ease(T value, easing::curve curve, easing::mode mode, T back_strength) -> T {
	switch (curve) {
		case easing::curve::back:      { return easing::back::ease(value, mode, back_strength); }
		case easing::curve::quadratic: { return easing::quadratic::ease(value, mode); }
		case easing::curve::quartic:   { return easing::quartic::ease(value, mode); }
		default:                       { throw std::invalid_argument{"Invalid ease curve"}; }
	}
}

} // snd
