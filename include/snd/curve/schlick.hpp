#pragma once

#include <cmath>
#include <vector>
#include "snd/misc.h"
#include "snd/types.h"
#include "snd/ease.h"

namespace snd {
namespace curve {
namespace schlick {

template <class T>
static constexpr T EPSILON = T(0.00001);

template <class T>
struct Spec
{
	T slope;
	T threshold;
};

template <class T>
inline T value(T x, const Spec<T>& spec)
{
	if (x < spec.threshold)
	{
		return (spec.threshold * x) / (x + spec.slope * (spec.threshold - x) + EPSILON<T>);
	}
	else
	{
		return (((T(1) - spec.threshold) * (x - T(1))) / (T(1) - x - spec.slope * (spec.threshold - x) + EPSILON<T>)) + T(1);
	}
}

template <class T>
inline T derivative(T x, const Spec<T>& spec)
{
	if (x < spec.threshold)
	{
		return (spec.slope * std::pow(spec.threshold, T(2))) / std::pow((spec.slope - T(1)) * x - (spec.slope * spec.threshold), T(2));
	}
	else
	{
		return (spec.slope * std::pow(spec.threshold - T(1), T(2))) / std::pow((spec.slope - T(1)) * x - (spec.slope * spec.threshold) + T(1), T(2));
	}
}

template <class T>
inline T calculate_slope(T x)
{
	x = ease::quadratic::out_in(x);

	return std::pow(T(3), (T(10) * (x - T(0.5))));
}

template <class T>
inline T calculate_threshold(T y)
{
	return T(0.5) + (T(0.5) * (T(1.0) - y));
}

inline std::vector<XY<float>> generate(const Spec<float>& spec, const Range<float> range, std::size_t resolution)
{
	std::vector<XY<float>> out;

	if (resolution < 2)
	{
		out.push_back({ range.begin, value(range.end, spec) });

		return out;
	}

	out.reserve(resolution);

	for (float x = range.begin; x < range.end; x += 1.0f / resolution)
	{
		out.push_back({ x, value(x, spec)});
	}

	out.push_back({ range.end, value(range.end, spec) });

	return out;
}

}}}