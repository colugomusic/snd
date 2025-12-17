#pragma once

#include <algorithm>
#include <array>

namespace snd {

using frame_pos = double;
template <size_t N> using frame_vec = std::array<frame_pos, N>;

template <size_t N> [[nodiscard]]
auto operator-(frame_vec<N> x, frame_pos v) -> frame_vec<N> {
	std::ranges::for_each(x, [v](frame_pos& x) { x -= v; });
	return x;
}

template <size_t N> [[nodiscard]]
auto operator+(frame_vec<N> x, frame_pos v) -> frame_vec<N> {
	std::ranges::for_each(x, [v](frame_pos& x) { x += v; });
	return x;
}

template <size_t N> [[nodiscard]]
auto operator*(frame_vec<N> x, double v) -> frame_vec<N> {
	std::ranges::for_each(x, [v](frame_pos& x) { x *= v; });
	return x;
}

template <size_t N> [[nodiscard]]
auto operator/(frame_vec<N> x, double v) -> frame_vec<N> {
	std::ranges::for_each(x, [v](frame_pos& x) { x /= v; });
	return x;
}

template <size_t N>
auto operator-=(frame_vec<N>& x, frame_pos v) -> frame_vec<N>& {
	std::ranges::for_each(x, [v](frame_pos& x) { x -= v; });
	return x;
}

template <size_t N>
auto operator+=(frame_vec<N>& x, frame_pos v) -> frame_vec<N>& {
	std::ranges::for_each(x, [v](frame_pos& x) { x += v; });
	return x;
}

template <size_t N>
auto operator*=(frame_vec<N>& x, double v) -> frame_vec<N>& {
	std::ranges::for_each(x, [v](frame_pos& x) { x *= v; });
	return x;
}

template <size_t N>
auto operator/=(frame_vec<N>& x, double v) -> frame_vec<N>& {
	std::ranges::for_each(x, [v](frame_pos& x) { x /= v; });
	return x;
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
