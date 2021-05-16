#pragma once

#include <vector>

#include <snd/ease.hpp>
#include <snd/misc.hpp>
#include <snd/types.hpp>

namespace snd {
namespace curve {
namespace tilt {

enum class Mode
{
	Linear = 0,
	InOut = 1,
	Under = 2,
	Over = 3,
};

template <class T>
struct Spec
{
	T points[2];
	Mode mode;
};

template <class T>
inline T value(T x, const Spec<T>& spec)
{
	switch (spec.mode)
	{
		case Mode::InOut:
		{
			return snd::lerp(spec.points[0], spec.points[1], snd::ease::quadratic::in_out(x));
		}

		case Mode::Over:
		{
			if (spec.points[0] < spec.points[1])
			{
				return snd::lerp(spec.points[0], spec.points[1], snd::ease::quadratic::out(x));
			}
			else
			{
				return snd::lerp(spec.points[1], spec.points[0], snd::ease::quadratic::out(T(1) - x));
			}
		}

		case Mode::Under:
		{
			if (spec.points[0] < spec.points[1])
			{
				return snd::lerp(spec.points[0], spec.points[1], snd::ease::quadratic::in(x));
			}
			else
			{
				return snd::lerp(spec.points[1], spec.points[0], snd::ease::quadratic::in(T(1) - x));
			}
		}

		case Mode::Linear:
		default:
		{
			return snd::lerp(spec.points[0], spec.points[1], x);
		}
	}
}

template <class T>
inline Spec<T> make_spec(float amp, float tilt, Mode mode)
{
	Spec<float> spec;

	tilt = snd::ease::quadratic::out_in(tilt);

	spec.points[0] = std::min(1.0f, 2.0f * (1.0f - tilt)) * amp;
	spec.points[1] = std::min(1.0f, 2.0f * tilt) * amp;
	spec.mode = mode;

	return spec;
}

inline std::vector<XY<float>> generate(float amp, float tilt, Mode mode, const Range<float> range, std::size_t resolution)
{
	std::vector<XY<float>> out;

	const auto spec = make_spec<float>(amp, tilt, mode);

	if (resolution < 2)
	{
		out.push_back({ range.begin, value(range.end, spec) });

		return out;
	}

	out.reserve(resolution);

	for (float x = range.begin; x < range.end; x += 1.0f / resolution)
	{
		out.push_back({ x, value(x, spec) });
	}

	out.push_back({ range.end, value(range.end, spec) });

	return out;
}

}}}
