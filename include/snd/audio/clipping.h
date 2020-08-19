#pragma once

#include <cmath>

namespace snd {
namespace audio {
namespace clipping {

template <class T>
T hard_clip(T x, T ceiling = 1)
{
	return T(0.5) * (std::abs(x + ceiling) - std::abs(x - ceiling));
}

/// @param threshold range: 0.5..1.0
template <class T>
T soft_clip(T x, T threshold = 0.75)
{
	auto magic = (T(1) - (threshold / x)) * (T(1) - threshold) + threshold;

	if (x > threshold) return magic;
	if (x < -threshold) return magic - T(2);

	return x;
}

}}}