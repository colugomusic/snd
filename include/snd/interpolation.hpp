#pragma once

namespace snd {
namespace interpolation {

template <class T>
T interp_4pt(T p0, T p1, T p2, T p3, T t)
{
	auto a = T(0.5) * (p2 - p0);
	auto b = T(0.5) * (p3 - p1);
	auto c = p1 - p2;
	auto d = a + c;
	auto e = b + d;
	auto f = c + e;

	return p1 + (t * (a + (t * ((t * f) - (d + f)))));
}

}}