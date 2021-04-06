#pragma once

#include <cassert>
#include <deque>
#include <functional>
#include "misc.h"

#pragma warning(push, 0)
#include <DSP/MLDSPFilters.h>
#pragma warning(pop)

namespace snd {
namespace autocorrelation {

struct AnalysisCallbacks
{
	std::function<void(std::uint32_t index, std::uint32_t n, float* out)> get_frames;
	std::function<bool()> should_abort;
	std::function<void(float)> report_progress;
};

// specialized autocorrelation which only considers the distances between zero crossings in the analysis.
// output is an array of estimated wavecycle sizes for each frame
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

		for (int depth = 2; depth * 2 <= crossings.size(); depth += 2)
		{
			auto a = crossings.begin();
			auto b = crossings.begin() + depth;

			auto total_diff = 0;
			auto terrible = false;

			for (int i = 0; i < depth; i++, a++, b++)
			{
				assert(a->up == b->up);

				total_diff += std::abs(a->distance - b->distance);

				if (total_diff >= best_diff)
				{
					terrible = true;
					break;
				}
			}

			if (terrible) continue;

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

	ml::DSPVector chunk_frames;
	ml::OnePole filter;
	ml::DCBlocker dc_blocker;

	filter.mCoeffs = ml::OnePole::coeffs(0.01f);
	dc_blocker.mCoeffs = ml::DCBlocker::coeffs(0.045f);

	auto index = 0;
	auto frames_remaining = n;

	while (frames_remaining > 0)
	{
		if (callbacks.should_abort()) return false;

		auto num_chunk_frames = std::min(std::uint32_t(kFloatsPerDSPVector), frames_remaining);

		callbacks.get_frames(index, num_chunk_frames, chunk_frames.getBuffer());

		const auto filtered_frames = filter(dc_blocker(chunk_frames));

		for (std::uint32_t i = 0; i < num_chunk_frames; i++)
		{
			const auto value = filtered_frames[i];

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

				crossing.index = (index + i);
				crossing.distance = (index + i) - zx.latest;
				crossing.up = zx.up;

				crossings.push_front(crossing);

				if (crossings.size() > depth)
				{
					crossings.pop_back();
				}

				best_crossing = find_best_crossing(crossings);

				const auto size =
					best_crossing == (index + i)
					? prev_size
					: float((index + i) - best_crossing);

				const auto distance = (index + i) - zx.latest;

				if (distance > 1)
				{
					for (std::uint32_t j = 1; j < distance; j++)
					{
						const auto x = float(j) / (distance - 1);

						out[j + zx.latest] = lerp(prev_size, size, x);
					}
				}

				out[(index + i)] = size;

				callbacks.report_progress(float((index + i)) / (n - 1));

				prev_size = size;
				zx.latest = (index + i);
			}
		}

		index += num_chunk_frames;
		frames_remaining -= num_chunk_frames;
	}

	return true;
}

}
}