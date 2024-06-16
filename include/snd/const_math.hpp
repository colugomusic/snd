#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <limits>

namespace const_math {

template <typename T>
constexpr auto EPSILON = T(0.001);

template <typename T> [[nodiscard]] constexpr
auto abs(T x) -> T {
	return x < 0.0 ? -x : x;
}

template <typename T> [[nodiscard]] constexpr
auto square(T x) -> T {
	return x * x;
}

template <typename T> [[nodiscard]] constexpr
auto cube(T x) -> T {
	return x * x * x;
}

template <typename T> [[nodiscard]] constexpr
auto sqrt_helper(T x, T g) -> T {
	return abs(g - x / g) < EPSILON<T> ? g : sqrt_helper(x, (g + x / g) / 2);
}

template <typename T> [[nodiscard]] constexpr
auto sqrt(T x) -> T {
	return sqrt_helper(x, T(1));
}

template <typename T> [[nodiscard]] constexpr
auto sin_helper(T x) -> T {
	return x < EPSILON<T> ? x : 3 * sin_helper(x / 3) - 4 * cube(sin_helper(x / 3));
}

template <typename T> [[nodiscard]] constexpr
auto sin(T x) -> T {
	return sin_helper(x < 0 ? -x + M_PI : x);
}

template <typename T> [[nodiscard]] constexpr
auto sinh_helper(T x) -> T {
	return x < EPSILON<T> ? x : 3 * sinh_helper(x / 3) + 4 * cube(sinh_helper(x / 3));
}

template <typename T> [[nodiscard]] constexpr
auto sinh(T x) -> T {
	return x < 0 ? -sinh_helper(-x) : sinh_helper(x);
}

template <typename T> [[nodiscard]] constexpr
auto cos (T x) -> T {
	return sin(M_PI_2 - x);
}

template <typename T> [[nodiscard]] constexpr
auto cosh(T x) -> T {
	return sqrt(T(1) + square(sinh(x)));
}

template <typename T> [[nodiscard]] constexpr
auto pow(T base, int exponent) -> T {
	return exponent < 0
		? T(1) / pow(base, -exponent)
		: exponent == 0
			? T(1)
			: exponent == 1
				? base
				: base * pow(base, exponent-1);
}

template <typename T> [[nodiscard]] constexpr
auto atan_poly_helper(T res, T num1, T den1, T delta) -> T {
	return res < EPSILON<T> ? res : res + atan_poly_helper((num1 * delta) / (den1 + 2) - num1 / den1, num1 * delta * delta, den1 + 4, delta);
}

template <typename T> [[nodiscard]] constexpr
auto atan_poly(T x) -> T {
	return x + atan_poly_helper(pow(x, 5) / 5 - pow(x, 3) / 3, pow(x, 7), T(7), x * x);
}

template <typename T> [[nodiscard]] constexpr
auto atan_identity(T x) -> T {
	return x <= (T(2) - sqrt(T(3))) ? atan_poly(x) : (M_PI_2 / 3) + atan_poly((sqrt(3) * x - 1) / (sqrt(3) + x));
}

template <typename T> [[nodiscard]] constexpr
auto atan_cmplmntry(T x) -> T {
	return x < 1 ? atan_identity(x) : M_PI_2 - atan_identity(1 / x);
}

template <typename T> [[nodiscard]] constexpr
auto atan(T x) -> T {
	return x >= 0 ? atan_cmplmntry(x) : -atan_cmplmntry(-x);
}

template <typename T> [[nodiscard]] constexpr
auto atan2(T y, T x) -> T {
	return x > 0
		? atan(y / x)
		: y >= 0 && x < 0
			? atan(y / x) + M_PI
			: y < 0 && x < 0
				? atan(y / x) - M_PI
				: y > 0 && x == 0
					? M_PI_2
					: y < 0 && x == 0
						? -M_PI_2
						: 0;
}

template <typename T> [[nodiscard]] constexpr
auto nearest(T x) -> T {
	return (x - T(0.5)) > (int)(x) ? (int)(x + T(0.5)) : (int)(x);
}

template <typename T> [[nodiscard]] constexpr
auto fraction(T x) -> T {
	return (x - T(0.5)) > (int)(x) ? -(((T)(int)(x + T(0.5))) - x) : x - ((T)(int)(x));
}

template <typename T> [[nodiscard]] constexpr
auto exp_helper(T r) -> T {
	return 1.0 + r + pow(r, 2) / 2 + pow(r, 3) / 6 + pow(r, 4) / 24 + pow(r, 5) / 120 + pow(r, 6) / 720 + pow(r, 7) / 5040;
}

template <typename T> [[nodiscard]] constexpr
auto exp(T x) -> T {
	return pow(M_E, static_cast<int>(nearest(x))) * exp_helper(fraction(x));
}

template <typename T> [[nodiscard]] constexpr
auto mantissa(T x) -> T {
	return x >= 10 ? mantissa(x / 10) : x < 1 ? mantissa(x * 10) : x;
}

template <typename T> [[nodiscard]] constexpr
auto exponent_helper(T x, int e) -> int {
	return x >= 10 ? exponent_helper(x / 10, e + 1) : x < 1 ? exponent_helper(x * 10, e - 1) : e;
}

template <typename T> [[nodiscard]] constexpr
auto exponent(T x) -> int {
	return exponent_helper(x, 0);
}

template <typename T> [[nodiscard]] constexpr
auto log_helper2(T y) -> T {
	return T(2) * (y + pow(y, 3) / 3 + pow(y, 5) / 5 + pow(y, 7) / 7 + pow(y, 9) / 9 + pow(y, 11) / 11);
}

template <typename T> [[nodiscard]] constexpr
auto log_helper(T x) -> T {
	return log_helper2((x - 1) / (x + 1));
}

template <typename T> [[nodiscard]] constexpr
auto log(T x) -> T {
	return x == 0
		? -std::numeric_limits<T>::infinity()
		: x < 0
			? std::numeric_limits<T>::quiet_NaN()
			: T(2) * log_helper(sqrt(mantissa(x))) + T(2.3025851) * exponent(x);
}

} // const_math