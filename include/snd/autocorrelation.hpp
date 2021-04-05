#pragma once

#include <cassert>
#include <deque>
#include <functional>
#include "misc.h"

namespace snd {
namespace autocorrelation {

struct AnalysisCallbacks
{
	std::function<float(std::uint32_t index)> get_frame;
	std::function<bool()> should_abort;
	std::function<void(float)> report_progress;
};

// specialized autocorrelation which only considers the distances between zero crossings in the analysis.
// output is an array of estimated wavecycle sizes for each frame
// if no wavecycle could be found then zero is written
// returns false if aborted before the analysis could complete
// allocates memory
inline bool analyze(AnalysisCallbacks callbacks, std::uint32_t n, std::uint32_t depth, float* out)
{
	if (depth < 4) depth = 4;

	struct ZX
	{
		bool init = false;
		bool up = false;
		std::uint32_t latest = 0;
	} zx;

	struct Crossing
	{
		std::uint32_t index;
		std::int32_t distance;
		bool up;
	};

	std::deque<Crossing> crossings;

	const auto find_best_crossing = [](const std::deque<Crossing>& crossings)
	{
		// TODO: tune these
		constexpr auto AUTO_WIN = 1;
		constexpr auto MAX_DIFF = 100;

		auto best = crossings.front().index;
		auto best_diff = MAX_DIFF;
		int depth = 2;

		for (int depth = 2; depth * 2 <= crossings.size(); depth++)
		{
			auto a = crossings.begin();
			auto b = crossings.begin() + depth;

			auto total_diff = 0;

			for (int i = 0; i < depth; i++, a++, b++)
			{
				assert(a->up == b->up);

				total_diff += std::abs(a->distance - b->distance);
			}

			if (total_diff <= AUTO_WIN)
			{
				return (crossings.begin() + depth)->index;
			}

			if (total_diff < best_diff)
			{
				best_diff = total_diff;
				best = (crossings.begin() + depth)->index;
			}
		}

		return best;
	};

	auto best_crossing = 0;
	auto prev_size = 0.0f;

	for (std::uint32_t i = 0; i < n; i++)
	{
		if (callbacks.should_abort()) return false;

		const auto value = callbacks.get_frame(i);

		bool crossed = false;

		if (value > 0.0f && (!zx.init || !zx.up))
		{
			zx.init = true;
			zx.up = true;
			crossed = true;
		}
		else if (value < 0.0f && (!zx.init || zx.up))
		{
			zx.init = true;
			zx.up = false;
			crossed = true;
		}

		if (crossed)
		{
			Crossing crossing;

			crossing.index = i;
			crossing.distance = i - zx.latest;
			crossing.up = zx.up;

			crossings.push_front(crossing);

			if (crossings.size() > depth)
			{
				crossings.pop_back();
			}

			best_crossing = find_best_crossing(crossings);

			const auto size = float(i - best_crossing);
			const auto distance = i - zx.latest;

			if (distance > 1)
			{
				for (std::uint32_t j = 1; j < distance; j++)
				{
					const auto x = float(j) / (distance - 1);

					out[j + zx.latest] = lerp(prev_size, size, x);
				}
			}

			out[i] = size;

			callbacks.report_progress(float(i) / (n - 1));

			prev_size = size;
			zx.latest = i;
		}
	}

	return true;
}

}
}