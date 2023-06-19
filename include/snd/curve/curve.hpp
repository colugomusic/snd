#pragma once

#include <list>
#include <vector>
#include "snd/convert.hpp"
#include "snd/misc.hpp"
#include "snd/types.hpp"
#include <linalg.h>

namespace snd {
namespace curve {

template <typename Vec3, typename ValueFn>
auto add_detail(std::list<Vec3> points, ValueFn&& value_fn, typename std::list<Vec3>::iterator beg, typename std::list<Vec3>::iterator end, float value_resolution, int max_depth, int depth = 0) -> std::list<Vec3> {
	if (depth >= max_depth) {
		return points;
	}
	static constexpr auto TOLERANCE{1.0f};
	static constexpr auto ONE_THIRD{1.0f / 3.0f};
	static constexpr auto TWO_THIRDS{2.0f / 3.0f};
	const auto get_diff = [value_resolution](float a, float b, float c, float r) {
		return std::abs((snd::lerp(a, c, r) * value_resolution) - (b * value_resolution));
	};
	Vec3 forward;
	forward.x = snd::lerp(beg->x, end->x, TWO_THIRDS);
	forward.y = value_fn(forward.x);
	forward.z = snd::lerp(beg->z, end->z, TWO_THIRDS);
	Vec3 back;
	back.x = snd::lerp(beg->x, end->x, ONE_THIRD);
	back.y = value_fn(back.x);
	back.z = snd::lerp(beg->z, end->z, ONE_THIRD);
	const auto back_diff{get_diff(beg->y, back.y, end->y, ONE_THIRD)};
	const auto forward_diff{get_diff(beg->y, forward.y, end->y, TWO_THIRDS)};
	if (back_diff >= TOLERANCE || forward_diff >= TOLERANCE) {
		auto p0{points.insert(end, back)};
		auto p1{points.insert(end, forward)};
		points = add_detail(std::move(points), value_fn, beg, p0, value_resolution, max_depth, depth + 1);
		points = add_detail(std::move(points), value_fn, p0, p1, value_resolution, max_depth, depth + 1);
		points = add_detail(std::move(points), value_fn, p1, end, value_resolution, max_depth, depth + 1);
	}
	return points;
}

// Constructs a curved line using a minimal number of straight lines.
// [beg/end]: specify the segment of the line to construct (beg=0,end=1 would construct the entire line)
// [value_fn]: should return the value y of the line at any given point x from 0..1
// [resolution]: basically specifies how detailed the line is. If drawing the line visually, [resolution]
// should be the rendering area of the line segment in pixels.
template <typename Vec3, typename ValueFn>
auto construct(float beg, float end, ValueFn&& value_fn, XY<float> resolution) -> std::list<Vec3> {
	std::list<Vec3> points;
	// x-coordinate is the line position (0..1)
	// y-coordinate is the value (0..1)
	// z-coordinate is the render position (0..1)
	Vec3 zero, one;
	zero.x = beg;
	zero.y = value_fn(beg);
	zero.z = 0.0f;
	one.x = end;
	one.y = value_fn(end);
	one.z = 1.0f;
	auto p0 = points.insert(points.end(), zero);
	auto p1 = points.insert(points.end(), one);
	return snd::curve::add_detail(std::move(points), std::forward<ValueFn>(value_fn), p0, p1, resolution.y, int(std::pow(resolution.x, 1.0f / 3.0f)));
}

template <class T>
std::vector<XY<T>> simplify(const std::vector<XY<T>>& in, float ratio, T tolerance = T(1)) {
	if (in.size() < 3) return in; 
	std::vector<XY<T>> out; 
	std::size_t i = 0; 
	for (; i < in.size() - 2; i += 2) {
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
		if (dp < linalg::cos(convert::deg2rad(tolerance))) {
			out.push_back(p1);
		}
	} 
	for (; i < in.size(); i++) {
		out.push_back(in[i]);
	} 
	if (in.size() == out.size()) return out; 
	return simplify(out, ratio, tolerance);
}

} // curve
} // snd
