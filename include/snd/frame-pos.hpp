#pragma once

#include <algorithm>
#include <array>

namespace snd {

struct frame_pos {
	frame_pos() = default;
	frame_pos(double v) : v{v} {}
	frame_pos(const frame_pos&) = default;
	frame_pos& operator=(const frame_pos&) = default;
	frame_pos(frame_pos&&) noexcept = default;
	frame_pos& operator=(frame_pos&&) noexcept = default;
	operator double() const { return v; }
	[[nodiscard]] friend auto operator<=>(const frame_pos&, const frame_pos&) = default;
	[[nodiscard]] friend auto operator-(frame_pos a, double b) -> frame_pos { return frame_pos{a.v - b}; }
	[[nodiscard]] friend auto operator+(frame_pos a, double b) -> frame_pos { return frame_pos{a.v + b}; }
	[[nodiscard]] friend auto operator*(frame_pos a, double b) -> frame_pos { return frame_pos{a.v * b}; }
	[[nodiscard]] friend auto operator/(frame_pos a, double b) -> frame_pos { return frame_pos{a.v / b}; }
	[[nodiscard]] friend auto operator-=(frame_pos& a, double b) -> frame_pos& { a.v -= b; return a; }
	[[nodiscard]] friend auto operator+=(frame_pos& a, double b) -> frame_pos& { a.v += b; return a; }
	[[nodiscard]] friend auto operator*=(frame_pos& a, double b) -> frame_pos& { a.v *= b; return a; }
	[[nodiscard]] friend auto operator/=(frame_pos& a, double b) -> frame_pos& { a.v /= b; return a; }
	[[nodiscard]] static auto fn_plus(double v)  { return [v](frame_pos x) { return x + v; }; }
	[[nodiscard]] static auto fn_minus(double v) { return [v](frame_pos x) { return x - v; }; }
	[[nodiscard]] static auto fn_mult(double v)  { return [v](frame_pos x) { return x * v; }; }
	[[nodiscard]] static auto fn_div(double v)   { return [v](frame_pos x) { return x / v; }; }
private:
	double v = 0.0;
};

template <size_t N>
struct frame_vec {
	[[nodiscard]] auto operator[](size_t index) -> frame_pos&             { return v.at(index); }
	[[nodiscard]] auto operator[](size_t index) const -> const frame_pos& { return v.at(index); }
	[[nodiscard]] auto at(size_t index) -> frame_pos&             { return v.at(index); }
	[[nodiscard]] auto at(size_t index) const -> const frame_pos& { return v.at(index); }
	[[nodiscard]] friend
	auto update(frame_vec<N> x, auto fn) -> frame_vec<N> {
		std::ranges::for_each(x.v, [fn](frame_pos& x) { x = fn(x); });
		return x;
	}
	[[nodiscard]] friend
	auto max(frame_vec<N> x) -> frame_pos {
		std::ranges::sort(x.v);
		return x.v.back();
	}
	[[nodiscard]] friend
	auto min(frame_vec<N> x) -> frame_pos {
		std::ranges::sort(x.v);
		return x.v.front();
	}
	[[nodiscard]] friend
	auto minmax(frame_vec<N> x) -> std::pair<frame_pos, frame_pos> {
		std::ranges::sort(x.v);
		return {x.v.front(), x.v.back()};
	}
	[[nodiscard]] friend auto operator-(frame_vec<N> x, double v) -> frame_vec<N> { return update(x, frame_pos::fn_minus(v)); }
	[[nodiscard]] friend auto operator+(frame_vec<N> x, double v) -> frame_vec<N> { return update(x, frame_pos::fn_plus(v)); }
	[[nodiscard]] friend auto operator*(frame_vec<N> x, double v) -> frame_vec<N> { return update(x, frame_pos::fn_mult(v)); }
	[[nodiscard]] friend auto operator/(frame_vec<N> x, double v) -> frame_vec<N> { return update(x, frame_pos::fn_div(v)); }
	[[nodiscard]] friend auto operator-=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_minus(v)); return x; }
	[[nodiscard]] friend auto operator+=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	[[nodiscard]] friend auto operator*=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	[[nodiscard]] friend auto operator/=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_div(v)); return x; }
private:
	std::array<frame_pos, N> v;
};

template <size_t N0, size_t N1>
struct frame_vec_array {
	[[nodiscard]] auto operator[](size_t index) -> frame_vec<N0>&             { return v.at(index); }
	[[nodiscard]] auto operator[](size_t index) const -> const frame_vec<N0>& { return v.at(index); }
	[[nodiscard]] auto at(size_t index) -> frame_vec<N0>&             { return v.at(index); }
	[[nodiscard]] auto at(size_t index) const -> const frame_vec<N0>& { return v.at(index); }
	[[nodiscard]] friend
	auto update(frame_vec_array<N0, N1> x, auto fn) -> frame_vec_array<N0, N1> {
		std::ranges::for_each(x.v, [fn](frame_vec<N0>& row) {
			row = update(std::move(row), fn);
		});
		return x;
	}
	[[nodiscard]] friend auto operator-(frame_vec_array<N0, N1> x, double v) -> frame_vec_array<N0, N1> { return update(x, frame_pos::fn_minus(v)); }
	[[nodiscard]] friend auto operator+(frame_vec_array<N0, N1> x, double v) -> frame_vec_array<N0, N1> { return update(x, frame_pos::fn_plus(v)); }
	[[nodiscard]] friend auto operator*(frame_vec_array<N0, N1> x, double v) -> frame_vec_array<N0, N1> { return update(x, frame_pos::fn_mult(v)); }
	[[nodiscard]] friend auto operator/(frame_vec_array<N0, N1> x, double v) -> frame_vec_array<N0, N1> { return update(x, frame_pos::fn_div(v)); }
	[[nodiscard]] friend auto operator-=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_minus(v)); return x; }
	[[nodiscard]] friend auto operator+=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	[[nodiscard]] friend auto operator*=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	[[nodiscard]] friend auto operator/=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_div(v)); return x; }
private:
	std::array<frame_vec<N0>, N1> v;
};

} // snd
