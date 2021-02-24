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

inline std::vector<XY<float>> generate(const Spec<float>& spec, const Rect<int>& rect)
{
	std::vector<XY<float>> out;

	if (rect.size.x - rect.position.y - rect.position.x < 2)
	{
		out.push_back({ 0.0f, 0.0f });

		return out;
	}

	out.reserve(std::size_t(rect.size.x));

	for (int i = rect.position.x; i < rect.size.x - rect.position.y; i++)
	{
		const auto x = float(i) / float((rect.size.x - 1));

		out.push_back({ float(i) - rect.position.x, (1.0f - value(x, spec)) * rect.size.y });
	}

	return out;
}

}}}