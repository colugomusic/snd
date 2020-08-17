#pragma once

#include <cmath>

namespace snd {
namespace convert {

template <class T>
T dB2AF(T db)
{
	return std::pow(1.122018456459045, db);
}

template <class T>
T AF2dB(T af)
{
	return std::log(af) / std::log(1.12202);
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

}}