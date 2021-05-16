#pragma once

#include <vector>
#include "snd/convert.hpp"
#include "snd/types.hpp"
#include <linalg.h>

namespace snd {
namespace curve {

template <class T>
std::vector<XY<T>> simplify(const std::vector<XY<T>>& in, float ratio, T tolerance = T(1))
{
	if (in.size() < 3) return in;

	std::vector<XY<T>> out;

	std::size_t i = 0;

	for (; i < in.size() - 2; i += 2)
	{
		auto p0 = in[i];
		auto p1 = in[i + 1];
		auto p2 = in[i + 2];

		out.push_back(p0);
		
		auto beg = linalg::vec<float, 2>(p0.x * ratio, p0.y);
		auto mid = linalg::vec<float, 2>(p1.x * ratio, p1.y);
		auto end = linalg::vec<float, 2>(p2.x * ratio, p2.y);

		auto na = linalg::normalize(mid - beg);
		auto nb = linalg::normalize(end - mid);
		auto dp = linalg::dot(na, nb);

		if (dp < linalg::cos(convert::deg2rad(tolerance)))
		{
			out.push_back(p1);
		}
	}

	for (; i < in.size(); i++)
	{
		out.push_back(in[i]);
	}

	if (in.size() == out.size()) return out;

	return simplify(out, ratio, tolerance);
}

}}
