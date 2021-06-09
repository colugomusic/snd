#pragma once

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

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
	if (x < T(0)-threshold) return magic - T(2);
	
	return x;
}

template <size_t ROWS>
ml::DSPVectorArray<ROWS> hard_clip(const ml::DSPVectorArray<ROWS>& in, float ceiling = 1.0f)
{
	return 0.5f * (ml::abs(in + ceiling) - ml::abs(in - ceiling));
}

template <size_t ROWS>
ml::DSPVectorArray<ROWS> soft_clip(const ml::DSPVectorArray<ROWS>& in, float threshold = 0.75f)
{
	auto magic = (1.0f - (threshold / in)) * (1.0f - threshold) + threshold;

	auto is_high = ml::greaterThan(in, ml::DSPVectorArray<ROWS>(threshold));
	auto is_low = ml::lessThan(in, ml::DSPVectorArray<ROWS>(-threshold));

	return ml::select(magic, ml::select(magic - 2, in, is_low), is_high);
}

}}}
