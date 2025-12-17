#pragma once

#include <algorithm>
#include <array>

namespace snd {

using frame_pos = double;
template <size_t N>             using frame_vec       = std::array<frame_pos, N>;
template <size_t N0, size_t N1> using frame_vec_array = std::array<frame_vec<N0>, N1>;

template <size_t N> [[nodiscard]]
auto update(frame_vec<N> x, auto fn) -> frame_vec<N> {
	std::ranges::for_each(x, [fn](frame_pos& x) { x = fn(x); });
	return x;
}

template <size_t N0, size_t N1> [[nodiscard]]
auto update(frame_vec_array<N0, N1> x, auto fn) -> frame_vec_array<N0, N1> {
	std::ranges::for_each(x, [fn](frame_vec<N0>& row) {
		row = update(std::move(row), fn);
	});
	return x;
}

template <size_t N> [[nodiscard]]
auto operator-(frame_vec<N> x, frame_pos v) -> frame_vec<N> {
	return update(x, [v](frame_pos x) { return x - v; });
}

template <size_t N> [[nodiscard]]
auto operator+(frame_vec<N> x, frame_pos v) -> frame_vec<N> {
	return update(x, [v](frame_pos x) { return x + v; });
}

template <size_t N> [[nodiscard]]
auto operator*(frame_vec<N> x, double v) -> frame_vec<N> {
	return update(x, [v](frame_pos x) { return x * v; });
}

template <size_t N> [[nodiscard]]
auto operator/(frame_vec<N> x, double v) -> frame_vec<N> {
	return update(x, [v](frame_pos x) { return x / v; });
}

template <size_t N>
auto operator-=(frame_vec<N>& x, frame_pos v) -> frame_vec<N>& {
	return update(x, [v](frame_pos x) { return x - v; });
}

template <size_t N>
auto operator+=(frame_vec<N>& x, frame_pos v) -> frame_vec<N>& {
	return update(x, [v](frame_pos x) { return x + v; });
}

template <size_t N>
auto operator*=(frame_vec<N>& x, double v) -> frame_vec<N>& {
	return update(x, [v](frame_pos x) { return x * v; });
}

template <size_t N>
auto operator/=(frame_vec<N>& x, double v) -> frame_vec<N>& {
	return update(x, [v](frame_pos x) { return x / v; });
}

template <size_t N> [[nodiscard]]
auto max(frame_vec<N> v) -> frame_pos {
	std::ranges::sort(v);
	return v.back();
}

template <size_t N> [[nodiscard]]
auto min(frame_vec<N> v) -> frame_pos {
	std::ranges::sort(v);
	return v.front();
}

template <size_t N> [[nodiscard]]
auto minmax(frame_vec<N> v) -> std::pair<frame_pos, frame_pos> {
	std::ranges::sort(v);
	return {v.front(), v.back()};
}

} // snd
