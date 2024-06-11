#pragma once

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include <cmath>
#include "third_party/const_math.hpp"

namespace snd {
namespace ease {
namespace quadratic {

template <class T> constexpr T in(T x)
{
	return x * x;
}

template <class T> constexpr T out(T x)
{
	return (x * (x - T(2))) * T(-1);
}


template <class T> constexpr T in_out(T x)
{
	x /= T(0.5);

	if (x < T(1)) return x * x * T(0.5);

	x--;

	return (x * (x - T(2)) - T(1)) * T(-0.5);
}

template <class T> constexpr T out_in(T x)
{
	if (x < T(0.5))
	{
		x /= T(0.5);

		return T(-0.5) * (x * (x - T(2)));
	}
	else
	{
		return T(2 * const_math::pow(double(x) - 0.5, 2)) + T(0.5);
	}
}

} // quadratic

namespace parametric {

template <class T> constexpr T in_out(T x)
{
	auto sqr = x * x;

	return sqr / (T(2) * (sqr - x) + T(1));
}

} // parametric

}}
