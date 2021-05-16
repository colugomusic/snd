#pragma once

#include <algorithm>
#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

#include "convert.hpp"

namespace snd {

constexpr auto PI = 3.14159265358979f;


constexpr float DENORMAL_DC = 1e-25f;

inline void flush_denormal_to_zero(float& x)
{
	x += DENORMAL_DC;
	x -= DENORMAL_DC;
}

template <size_t ROWS>
inline ml::DSPVectorArray<ROWS> flush_denormal_to_zero(const ml::DSPVectorArray<ROWS>& in)
{
	ml::DSPVectorArray<ROWS> out;

	for (int i = 0; i < kFloatsPerDSPVector; i++)
	{
		out = in;

		out[i] += DENORMAL_DC;
		out[i] -= DENORMAL_DC;
	}

	return out;
}

template <class T>
T tan_rational(T in)
{
	auto a = (in * T(-0.0896638f)) + T(0.0388452f);
	auto b = (in * T(-0.430871f)) + T(0.0404318f);
	auto c = (a * in) + T(1.00005f);
	auto d = (b * in) + T(1.f);

	return (c * in) / d;
}

template <class T>
T blt_prewarp_rat_0_08ct_sr_div_2(int SR, const T& freq)
{
	return tan_rational(ml::min(T(1.50845f), (T(PI) / float(SR)) * freq));
}

template <class T>
constexpr T lerp(T a, T b, T x)
{
	return (x * (b - a)) + a;
}

template <class T>
constexpr T inverse_lerp(T a, T b, T x)
{
	return (x - a) / (b - a);
}

template <class T> T quadratic_sequence(T step, T start, T n)
{
	auto a = T(0.5) * step;
	auto b = start - (T(3.0) * a);

	return (a * std::pow(n + 1, 2)) + (b * (n + 1)) - (start - 1) + (step - 1);
}

template <class T> T quadratic_formula(T a, T b, T c, T n)
{
	return (a * (n * n)) + (b * n) + c;
}

template <class T> T holy_fudge(T f0, T f1, T N, T n, T C)
{
	auto accel = (f1 - f0) / (T(2) * N);

	return quadratic_formula(accel, f0, C, n);
}

template <class T> T distance_traveled(T start, T speed, T acceleration, T n)
{
	return (speed * n) + (T(0.5) * acceleration) * (n * n) + start;
}

template <class T> T ratio(T min, T max, T distance)
{
    return std::pow(T(2), ((max - min) / distance) / T(12));
}

template <class T> T sum_of_geometric_series(T first, T ratio, T n)
{
    return first * ((T(1) - std::pow(ratio, n)) / (T(1) - ratio));
}

template <class T> T holy_grail(T min, T max, T distance, T n)
{
	auto r = ratio(min, max, distance);

	if (std::abs(T(1) - r) <= T(0))
	{
		return n * convert::P2FF(min);
	}

	return sum_of_geometric_series(convert::P2FF(min), r, n);
}

template <class T> T holy_grail_ff(T min, T max, T distance, T n)
{
	return convert::P2FF(min) * std::pow(ratio(min, max, distance), n);
}

template <class T>
T dn_cancel(T value)
{
	if (std::fpclassify(value) != FP_NORMAL)
	{
		return std::numeric_limits<T>().min();
	}

	return value;
}

template <class T>
T dn_cancel(T value, T min)
{
	if (value < min)
	{
		return min;
	}

	return value;
}

template <class T>
inline T wrap(T x, T y)
{
	x = std::fmod(x, y);

	if (x < T(0)) x += y;

	return x;
}

inline int wrap(int x, int y)
{
	x = x % y;

	if (x < 0) x += y;

	return x;
}

}
