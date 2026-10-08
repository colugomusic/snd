#pragma once

#include "snd/types.hpp"
#include <concepts>
#include <cmath>
#include <list>
#include <ranges>

namespace snd::curved_line_constructor {

static constexpr auto TOLERANCE  = 1.0f;
static constexpr auto ONE_THIRD  = 1.0f / 3.0f;
static constexpr auto TWO_THIRDS = 2.0f / 3.0f;

struct builder_point {
	float x_01;
	float y_01;
	float x_pixel;
};

template <typename Fn>
concept get_value_fn = requires(Fn fn, float x) {
	{ fn(x) } -> std::convertible_to<float>;
};

template <typename T> struct is_std_list_of_builder_point : std::false_type {};
template <typename Allocator> struct is_std_list_of_builder_point<std::list<builder_point, Allocator>> : std::true_type {};

template <typename T>
concept std_list_of_builder_point = is_std_list_of_builder_point<std::remove_reference_t<T>>::value;

auto fn_compare(float resolution) {
	return [resolution](float y_01_beg, float y_01_at_x, float y_01_end, float x_01) {
		auto linear = std::lerp(y_01_beg, y_01_end, x_01) * resolution;
		auto curved = y_01_at_x * resolution;
		return std::abs(linear - curved);
	};
}

auto fn_builder_point_to_xy(float resolution) {
	return [resolution](builder_point p) -> XY<float> {
		return {
			.x = p.x_pixel,
			.y = p.y_01 * resolution
		};
	};
}

[[nodiscard]]
auto make_builder_point(get_value_fn auto fn_get_value, builder_point beg, builder_point end, float ratio) -> builder_point {
	const auto x_01    = std::lerp(beg.x_01, end.x_01, ratio);
	const auto y_01    = fn_get_value(x_01);
	const auto x_pixel = std::lerp(beg.x_pixel, end.x_pixel, ratio);
	return builder_point{
		.x_01    = x_01,
		.y_01    = y_01,
		.x_pixel = x_pixel
	};
}

[[nodiscard]]
auto make_backward_point(get_value_fn auto fn_get_value, builder_point beg, builder_point end) -> builder_point {
	return make_builder_point(fn_get_value, beg, end, ONE_THIRD);
}

[[nodiscard]]
auto make_forward_point(get_value_fn auto fn_get_value, builder_point beg, builder_point end) -> builder_point {
	return make_builder_point(fn_get_value, beg, end, TWO_THIRDS);
}

[[nodiscard]]
auto make_beg_point(get_value_fn auto fn_get_value, float x_01) -> builder_point {
	return builder_point{
		.x_01    = x_01,
		.y_01    = fn_get_value(x_01),
		.x_pixel = 0.0f
	};
}

[[nodiscard]]
auto make_end_point(get_value_fn auto fn_get_value, float x_01, float resolution) -> builder_point {
	return builder_point{
		.x_01    = x_01,
		.y_01    = fn_get_value(x_01),
		.x_pixel = resolution
	};
}

template <
	get_value_fn GetValueFn,
	typename TemporaryAllocator,
	typename Iterator = typename std::list<builder_point, TemporaryAllocator>::iterator
>
[[nodiscard]]
auto add_detail(std::list<builder_point, TemporaryAllocator> points, GetValueFn fn_get_value, Iterator beg, Iterator end, float resolution, int max_depth, int depth = 0) -> decltype(auto) {
	if (depth >= max_depth) {
		return points;
	}
	const auto fn_compare = curved_line_constructor::fn_compare(resolution);
	const auto backward   = make_backward_point(fn_get_value, *beg, *end);
	const auto forward    = make_forward_point(fn_get_value, *beg, *end);
	const auto diff_backward = fn_compare(beg->y_01, backward.y_01, end->y_01, ONE_THIRD);
	const auto diff_forward  = fn_compare(beg->y_01, forward.y_01, end->y_01, TWO_THIRDS);
	if (diff_backward >= TOLERANCE || diff_forward >= TOLERANCE) {
		auto p0 = points.insert(end, backward);
		auto p1 = points.insert(end, forward);
		points = add_detail(std::move(points), fn_get_value, beg, p0, resolution, max_depth, depth + 1);
		points = add_detail(std::move(points), fn_get_value, p0, p1, resolution, max_depth, depth + 1);
		points = add_detail(std::move(points), fn_get_value, p1, end, resolution, max_depth, depth + 1);
	}
	return points;
}

// Constructs a curved line using the minimal number of straight lines.
// [fn_get_value]: Should return the value y (0..1) of the line at any given point x (0..1)
// [from/to]: Specify the segment of the line to construct (from=0,to=1 would construct the entire line.)
// [resolution]: Basically specifies how detailed the line is. If drawing the line visually, [resolution] should be the rendering area of the line segment in pixels.
// [tmp_list]: You need to pass in your own empty std::list<snd::curved_line_constructor::builder_point, Alloc> to be used internally for the construction process. This is so that you can use your own special allocator for the temporary memory allocations.
// [out]: Output iterator for the final line points scaled to the resolution.
template <typename TmpList>
requires std_list_of_builder_point<TmpList>
auto construct(get_value_fn auto fn_get_value, float from, float to, XY<float> resolution, TmpList&& tmp_list, std::output_iterator<XY<float>> auto out) -> void {
	const auto beg       = tmp_list.insert(tmp_list.end(), make_beg_point(fn_get_value, from));
	const auto end       = tmp_list.insert(tmp_list.end(), make_end_point(fn_get_value, to, resolution.x));
	const auto max_depth = static_cast<int>(std::pow(resolution.x, ONE_THIRD));
	const auto fn_xform  = fn_builder_point_to_xy(resolution.y);
	tmp_list             = add_detail(std::move(tmp_list), fn_get_value, beg, end, resolution.y, max_depth);
	std::ranges::copy(tmp_list | std::views::transform(fn_xform), out);
}

} // snd::curved_line_constructor
