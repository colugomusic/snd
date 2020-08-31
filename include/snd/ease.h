#pragma once

namespace snd {
namespace ease {
namespace quadratic {

template <class T> T in(T x)
{
	return x * x;
}

template <class T> T out(T x)
{
	return (x * (x - T(2))) * T(-1);
}

template <class T> T in_out(T x)
{
	x /= T(0.5);

	if (x < T(1)) return x * x * T(0.5);

	x--;

	return (x * (x - T(2)) - T(1)) * T(-0.5);
}

} // quadratic

namespace parametric {

template <class T> T in_out(T x)
{
	auto sqr = x * x;

	return sqr / (T(2) * (sqr - x) + T(1));
}

} // parametric

}}