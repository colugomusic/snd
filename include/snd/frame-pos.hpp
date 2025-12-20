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
	[[nodiscard]] friend auto operator<=>(const frame_pos& a, double b) { return a.v <=> b; }
	[[nodiscard]] friend auto operator<=>(double a, const frame_pos& b) { return a <=> b.v; }
	[[nodiscard]] friend auto operator-(frame_pos a, double b) -> frame_pos { return frame_pos{a.v - b}; }
	[[nodiscard]] friend auto operator+(frame_pos a, double b) -> frame_pos { return frame_pos{a.v + b}; }
	[[nodiscard]] friend auto operator*(frame_pos a, double b) -> frame_pos { return frame_pos{a.v * b}; }
	[[nodiscard]] friend auto operator/(frame_pos a, double b) -> frame_pos { return frame_pos{a.v / b}; }
	friend auto operator-=(frame_pos& a, double b) -> frame_pos& { a.v -= b; return a; }
	friend auto operator+=(frame_pos& a, double b) -> frame_pos& { a.v += b; return a; }
	friend auto operator*=(frame_pos& a, double b) -> frame_pos& { a.v *= b; return a; }
	friend auto operator/=(frame_pos& a, double b) -> frame_pos& { a.v /= b; return a; }
	[[nodiscard]] static auto fn_plus(double v)  { return [v](frame_pos x) { return x + v; }; }
	[[nodiscard]] static auto fn_minus(double v) { return [v](frame_pos x) { return x - v; }; }
	[[nodiscard]] static auto fn_mult(double v)  { return [v](frame_pos x) { return x * v; }; }
	[[nodiscard]] static auto fn_div(double v)   { return [v](frame_pos x) { return x / v; }; }
	[[nodiscard]] static auto fn_minus_from(double v) { return [v](frame_pos x) { return v - x; }; }
	[[nodiscard]] static auto fn_div_from(double v)   { return [v](frame_pos x) { return v / x; }; }
private:
	double v = 0.0;
};

template <size_t N>
struct frame_vec {
	[[nodiscard]] auto operator[](size_t index) -> frame_pos&             { return v.at(index); }
	[[nodiscard]] auto operator[](size_t index) const -> const frame_pos& { return v.at(index); }
	[[nodiscard]] auto at(size_t index) -> frame_pos&             { return v.at(index); }
	[[nodiscard]] auto at(size_t index) const -> const frame_pos& { return v.at(index); }
	[[nodiscard]] auto data() -> frame_pos*             { return v.data(); }
	[[nodiscard]] auto data() const -> const frame_pos* { return v.data(); }
	[[nodiscard]] auto begin()        { return v.begin(); }
	[[nodiscard]] auto end()          { return v.end(); }
	[[nodiscard]] auto begin() const  { return v.begin(); }
	[[nodiscard]] auto end() const    { return v.end(); }
	[[nodiscard]] auto cbegin() const { return v.cbegin(); }
	[[nodiscard]] auto cend() const   { return v.cend(); }
	auto set(size_t index, frame_pos v) -> void { at(index) = v; }
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
	[[nodiscard]] friend auto operator-(double v, frame_vec<N> x) -> frame_vec<N> { return update(x, frame_pos::fn_minus_from(v)); }
	[[nodiscard]] friend auto operator+(double v, frame_vec<N> x) -> frame_vec<N> { return update(x, frame_pos::fn_plus(v)); }
	[[nodiscard]] friend auto operator*(double v, frame_vec<N> x) -> frame_vec<N> { return update(x, frame_pos::fn_mult(v)); }
	[[nodiscard]] friend auto operator/(double v, frame_vec<N> x) -> frame_vec<N> { return update(x, frame_pos::fn_div_from(v)); }
	friend auto operator-=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_minus(v)); return x; }
	friend auto operator+=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	friend auto operator*=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	friend auto operator/=(frame_vec<N>& x, double v) -> frame_vec<N>& { x = update(x, frame_pos::fn_div(v)); return x; }
	friend auto operator-=(double v, frame_vec<N>& x) -> frame_vec<N>& { x = update(x, frame_pos::fn_minus_from(v)); return x; }
	friend auto operator+=(double v, frame_vec<N>& x) -> frame_vec<N>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	friend auto operator*=(double v, frame_vec<N>& x) -> frame_vec<N>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	friend auto operator/=(double v, frame_vec<N>& x) -> frame_vec<N>& { x = update(x, frame_pos::fn_div_from(v)); return x; }
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
	[[nodiscard]] friend auto operator-=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_minus_from(v)); return x; }
	[[nodiscard]] friend auto operator+=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	[[nodiscard]] friend auto operator*=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	[[nodiscard]] friend auto operator/=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_div_from(v)); return x; }
	friend auto operator-=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_minus(v)); return x; }
	friend auto operator+=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	friend auto operator*=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	friend auto operator/=(frame_vec_array<N0, N1>& x, double v) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_div(v)); return x; }
	friend auto operator-=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_minus_from(v)); return x; }
	friend auto operator+=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_plus(v)); return x; }
	friend auto operator*=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_mult(v)); return x; }
	friend auto operator/=(double v, frame_vec_array<N0, N1>& x) -> frame_vec_array<N0, N1>& { x = update(x, frame_pos::fn_div_from(v)); return x; }
private:
	std::array<frame_vec<N0>, N1> v;
};

} // snd
