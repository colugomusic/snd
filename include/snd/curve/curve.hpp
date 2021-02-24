#pragma once

#include <vector>
#include "snd/types.h"

namespace snd {
namespace curve {

// TODO: Change to use an angle threshold instead of just testing vertical
//       difference (will simplify more effectively). Requires an interface
//       change.
template <class T>
std::vector<XY<T>> simplify(const std::vector<XY<T>>& in, T threshold = T(0.5))
{
	if (in.size() < 3) return in;

	std::vector<XY<T>> out;

	std::size_t i = 0;

	for (; i < in.size() - 2; i += 2)
	{
		auto p0 = in[i];
		auto p1 = in[i + 1];
		auto p2 = in[i + 2];

		auto midp = lerp(p0.y, p2.y, inverse_lerp(p0.x, p2.x, p1.x));

		out.push_back(p0);

		if (std::abs(midp - p1.y) > threshold)
		{
			out.push_back(p1);
		}
	}

	for (; i < in.size(); i++)
	{
		out.push_back(in[i]);
	}

	if (in.size() == out.size()) return out;

	return simplify(out, threshold);
}

}}