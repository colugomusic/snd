#pragma once

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif
#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace convert {

template <class T>
T dB2AF(T db)
{
	return std::pow(T(1.122018456459045), db);
}

template <size_t ROWS>
ml::DSPVectorArray<ROWS> dB2AF(const ml::DSPVectorArray<ROWS>& db)
{
	return ml::pow(ml::DSPVectorArray<ROWS>(1.122018456459045f), db);
}

template <class T>
T AF2dB(T af)
{
	return std::log(af) / std::log(1.12202);
}

inline float pitch_to_frequency(float pitch)
{
	return 8.1758f * std::pow(2.0f, pitch / 12.0f);
}

template <class T>
T P2FF(T p)
{
	return std::pow(T(2), p / T(12));
}

template <class T>
T FF2P(T ff)
{
	return (std::log(ff) / std::log(T(2))) * T(12);
}

template <class T>
constexpr float deg2rad(T y)
{
	return y * T(M_PI) / T(180.0);
}

}}
