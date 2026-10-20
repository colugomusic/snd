#pragma once

#include "flush-dc.hpp"
#include <algorithm>
#include <cassert>
#include <complex>
#include <span>
#include <numbers>

namespace snd::filters {

constexpr auto SETTLE_FRAMES = 2;

struct gain_db          { float v = 0.0f; };
struct inverted_gain_db { float v = 0.0f; };

[[nodiscard]] inline
auto bi_clip(float v, float max) -> std::pair<float,float> {
	if (v > max) { return {max, v}; }
	else         { return {v, max}; }
}

[[nodiscard]] inline auto reciprocal(float v) -> float          { return 1.0f / v; }
[[nodiscard]] inline auto linear_to_db(float linear) -> float   { return std::log(linear) * 8.6858896380650365530225783783321f; }
[[nodiscard]] inline auto db_to_linear(float db) -> float       { return std::exp(db * 0.11512925464970228420089957273422f); }
[[nodiscard]] inline auto pitch_to_frequency(float v) -> float  { return 8.1758f * std::pow(2.0f, v / 12.0f); }
[[nodiscard]] inline auto linear_to_filter_hz(float v) -> float { return pitch_to_frequency(std::lerp(-8.513f, 135.076f, v)); }


} // snd::filters

// NOTE: Remember to call settle()!

// At extreme parameter values these filters may need to take a couple frames to "settle"
// first (i.e. once you are ready to begin processing the filter with your initial
// parameter values, call settle() to feed the filters with a couple frames of silence
// before suddenly throwing audio data at them, otherwise they may glitch.)

namespace snd::filters::shelving {

struct svf_result {
	std::complex<float> LP;
	std::complex<float> BPn;
	std::complex<float> HP;
};

struct integrator_state {
	float fbk = 0.0f;
};

[[nodiscard]] inline
auto g2R(float g, float M) -> float {
	const auto a = ((2.0f - g) / g) * M;
	return sqrt(a + 2.0f) / 2.0f;
}

[[nodiscard]] inline
auto R2p1(float _2R, float M) -> float {
	const auto a = std::max(1.0f, _2R);
	const auto b = std::sqrt(std::pow(a, 2.0f) - 1.0f);
	const auto c = a + b;
	if (M >= 1.0f) { return c; }
	else           { return reciprocal(c); }
}

[[nodiscard]] inline
auto a22R(float v) -> float {
	return cos(v) * 2.0f;
}

[[nodiscard]] inline
auto approx_tan_rational(float v) -> float {
	auto a = (v * -0.0896638f) + 0.0388452f;
	auto b = (v * -0.430871f) + 0.0404318f;
	auto c = (a * v) + 1.00005f;
	auto d = (b * v) + 1.0f;
	return (c * v) / d;
}

[[nodiscard]] inline
auto compute_wdiv2(float f, float SR) -> float {
	f = std::max(f, 0.01f);
	f *= std::numbers::pi_v<float> / SR;
	f = std::min(f, 1.50845f);
	return approx_tan_rational(f);
}

[[nodiscard]] inline
auto integrator(integrator_state state, float x, std::complex<float> zdf, float wdiv2) -> std::tuple<integrator_state, float, std::complex<float>> {
	const auto a = x * wdiv2;
	const auto out = a + state.fbk;
	state.fbk = a + out;
	state.fbk = flush_denormal_to_zero(state.fbk);
	zdf = {
		wdiv2 * zdf.real(),
		state.fbk + (zdf.imag() * wdiv2)
	};
	return std::make_tuple(state, out, zdf);
}

[[nodiscard]] inline
auto compute_1divG(float g) -> float {
	return 1.0f / (1.0f - g);
}

[[nodiscard]] inline
auto solve_y(float x, float g, float s) -> float {
	const auto a      = x * g;
	const auto b      = a + s;
	const auto _1divG = compute_1divG(g);
	return b * _1divG;
}

[[nodiscard]] inline
auto gain(float v, std::complex<float> zdf, float amount) -> std::tuple<float, std::complex<float>> {
	v   *= amount;
	zdf *= amount;
	return std::make_tuple(v, zdf);
}

} // snd::filters::shelving

namespace snd::filters::shelving::tf {

[[nodiscard]] inline
auto svf(std::complex<float> s, float _2R) -> svf_result {
	const auto spow2 = s * s;
	const auto a     = spow2 + 1.0f;
	const auto b     = s * _2R;
	const auto c     = a + b;
	const auto LP    = 1.0f / c;
	const auto BPn   = (LP * s) * _2R;
	const auto HP    = (LP * s) * s;
	return {
		.LP  = LP,
		.BPn = BPn,
		.HP  = HP
	};
}

} // snd::filters::shelving::tf

namespace snd::filters::shelving::band_shelf {

struct lp_to_bp_state {
	integrator_state integrator1;
	integrator_state integrator2;
	float fbk_g     = 0.0f;
	float fbk_s     = 0.0f;
	float fbk_1divG = 0.0f;
};

struct svf_state {
	lp_to_bp_state lp_to_bp_1;
	lp_to_bp_state lp_to_bp_2;
	float fbk_g = 0.0f;
	float fbk_s = 0.0f;
};

struct tilt_state {
	svf_state svf;
};

struct state {
	state() = default;
	tilt_state tilt1;
	tilt_state tilt2;
};

struct tilt_coeffs {
	float damp;
	float M;
	float recip_M;
	float BW;
	float wMid;
};

struct coeffs {
	float F;
	float Mpow2;
	float recip_Mpow2;
	float wdiv2;
	tilt_coeffs tilt1;
	tilt_coeffs tilt2;
};

[[nodiscard]] inline
auto make_coeffs(float f, filters::gain_db gain_db, float slope, float bw, float SR) -> coeffs {
	const auto slope_mod            = std::pow(2.0f, slope);
	const auto M                    = db_to_linear(gain_db.v);
	const auto Mpow2                = std::pow(M, 2.0f);
	const auto recip_Mpow2          = reciprocal(Mpow2);
	const auto [slope_lo, slope_hi] = bi_clip(slope_mod, 2.0f);
	const auto slope_hi_half        = slope_hi / 2.0f;
	const auto g2R_M                = Mpow2 + recip_Mpow2;
	const auto g2R1                 = g2R(slope_lo, g2R_M);
	const auto g2R2                 = g2R(slope_hi_half, g2R_M);
	const auto g2R1_clamp           = std::min(1.0f, g2R1);
	const auto g2R2_clamp           = std::min(1.0f, g2R2);
	const auto R1x                  = g2R1 / g2R1_clamp;
	const auto a1                   = std::acos(g2R1_clamp);
	const auto a2                   = std::acos(g2R2_clamp);
	const auto g2R2_R2p1            = R2p1(g2R2, M);
	const auto tilt1_M              = M * g2R2_R2p1;
	const auto tilt2_M              = M / g2R2_R2p1;
	const auto a2div2               = a2 / 2.0f;
	const auto tilt1_damp           = a22R(a1 + a2div2) * R1x;
	const auto tilt2_damp           = a22R(a1 - a2div2);
	const auto tilt1_wMid           = std::sqrt(tilt1_M);
	const auto tilt2_wMid           = std::sqrt(tilt2_M);
	const auto BWpow2               = std::pow(2.0f, bw);
	const auto tilt1_recip_M        = reciprocal(tilt1_M);
	const auto tilt2_recip_M        = reciprocal(tilt2_M);
	const auto tilt1_BW             = (BWpow2 - reciprocal(BWpow2)) * tilt1_wMid;
	const auto tilt2_BW             = (BWpow2 - reciprocal(BWpow2)) * tilt2_wMid;
	const auto wdiv2                = compute_wdiv2(f, SR);
	return {
		.F             = f,
		.Mpow2         = Mpow2,
		.recip_Mpow2   = recip_Mpow2,
		.wdiv2         = wdiv2,
		.tilt1 = {
			tilt1_damp,
			tilt1_M,
			tilt1_recip_M,
			tilt1_BW,
			tilt1_wMid,
		},
		.tilt2 = {
			tilt2_damp,
			tilt2_M,
			tilt2_recip_M,
			tilt2_BW,
			tilt2_wMid,
		},
	};
}

[[nodiscard]] inline
auto zdf_output(std::complex<float> zdf, std::complex<float> ext) -> std::complex<float> {
	return std::complex<float>{
		ext.real() * zdf.real(),
		zdf.imag() + (zdf.real() * ext.imag())
	};
}

[[nodiscard]] inline
auto compute_ext(std::complex<float> zdf, float s, float fbk_1divG) -> std::complex<float> {
	return {
		fbk_1divG * zdf.real(),
		(zdf.imag() + s) * fbk_1divG
	};
}

[[nodiscard]] inline
auto lp_to_bp(lp_to_bp_state state, float x, std::complex<float> zdf, float wdiv2, float BW) -> std::tuple<lp_to_bp_state, float, std::complex<float>> {
	std::tie(x, zdf)                    = gain(x, zdf, BW);
	auto gained_zdf                     = zdf;
	const auto y                        = solve_y(x, state.fbk_g, state.fbk_s);
	x                                   = x + y;
	zdf                                 = {1.0f, 0.0f};
	std::tie(state.integrator1, x, zdf) = integrator(state.integrator1, x, zdf, wdiv2);
	auto out_x                          = x;
	auto post_integ1_zdf                = zdf;
	std::tie(state.integrator2, x, zdf) = integrator(state.integrator2, x, zdf, wdiv2);
	state.fbk_g                         = zdf.real() * -1.0f;
	state.fbk_s                         = zdf.imag() * -1.0f;
	auto ext                            = compute_ext(gained_zdf, state.fbk_s, state.fbk_1divG);
	state.fbk_1divG                     = compute_1divG(state.fbk_g);
	const auto out_zdf                  = zdf_output(post_integ1_zdf, ext);
	return std::make_tuple(state, out_x, out_zdf);
}

[[nodiscard]] inline
auto svf(svf_state state, float x, float wdiv2, float damp, float BW) -> std::tuple<svf_state, float, float, float> {
	auto zdf                           = std::complex<float>{1.0f, 0.0f};
	auto BPn_zdf                       = std::complex<float>{};
	auto BPn                           = 0.0f;
	const auto y                       = solve_y(x, state.fbk_g, state.fbk_s);
	x                                  = x + y;
	const auto HP                      = x;
	std::tie(state.lp_to_bp_1, x, zdf) = lp_to_bp(state.lp_to_bp_1, x, zdf, wdiv2, BW);
	std::tie(BPn, BPn_zdf)             = gain(x, zdf, damp);
	std::tie(state.lp_to_bp_2, x, zdf) = lp_to_bp(state.lp_to_bp_2, x, zdf, wdiv2, BW);
	const auto LP                      = x;
	state.fbk_g                        = (BPn_zdf.real() + zdf.real()) * -1.0f;
	state.fbk_s                        = (BPn_zdf.imag() + zdf.imag()) * -1.0f;
	return std::make_tuple(state, LP, HP, BPn);
}

[[nodiscard]] inline
auto tilt(tilt_state state, float in, float wdiv2, tilt_coeffs coeffs) -> std::tuple<tilt_state, float> {
	float LP, HP, BPn;
	std::tie(state.svf, LP, HP, BPn) = svf(state.svf, in, wdiv2, coeffs.damp, coeffs.BW);
	return std::make_tuple(state, (LP / coeffs.M) + (HP * coeffs.M) + BPn);
}

[[nodiscard]] inline
auto process_impl(band_shelf::state state, float in, const band_shelf::coeffs& coeffs, float out_mult) -> std::tuple<band_shelf::state, float> {
	auto signal = in;
	std::tie(state.tilt1, signal) = tilt(state.tilt1, signal, coeffs.wdiv2, coeffs.tilt1);
	std::tie(state.tilt2, signal) = tilt(state.tilt2, signal, coeffs.wdiv2, coeffs.tilt2);
	signal *= out_mult;
	return std::make_tuple(state, signal);
}

[[nodiscard]] inline
auto settle_impl(band_shelf::state state, const band_shelf::coeffs& coeffs, float out_mult) -> band_shelf::state {
	for (size_t i = 0; i < SETTLE_FRAMES; i++) {
		std::tie(state, std::ignore) = process_impl(state, 0.0f, coeffs, coeffs.recip_Mpow2);
	}
	return state;
}

[[nodiscard]] inline
auto process(band_shelf::state state, float in, const band_shelf::coeffs& coeffs) -> std::tuple<band_shelf::state, float> {
	return process_impl(state, in, coeffs, coeffs.recip_Mpow2);
}

[[nodiscard]] inline
auto settle(band_shelf::state state, const band_shelf::coeffs& coeffs) -> band_shelf::state {
	return settle_impl(state, coeffs, coeffs.recip_Mpow2);
}

} // snd::filters::shelving::band_shelf

namespace snd::filters::shelving::band_shelf::tf {

[[nodiscard]] inline
auto convert(std::complex<float> v, float _2R) -> std::complex<float> {
	static constexpr auto EPSILON = 0.00000001f;
	const auto a = 1.0f / (v + EPSILON);
	const auto b = v + a;
	return b * (1.0f / _2R);
}

[[nodiscard]] inline
auto svf(float f, float F, float _2R, float BW) -> svf_result {
	return shelving::tf::svf(convert({0.0f, f / F}, BW), _2R);
}

[[nodiscard]] inline
auto tilt(std::complex<float> t, float f, float F, tilt_coeffs coeffs) -> std::complex<float> {
	const auto [LP, BPn, HP] = svf(f, F, coeffs.damp, coeffs.BW);
	const auto a = LP * coeffs.recip_M;
	const auto b = HP * coeffs.M;
	const auto c = a + b;
	const auto d = c + BPn;
	return t * d;
}

[[nodiscard]] inline
auto compute_impl(const band_shelf::coeffs& coeffs, float f, float out_mult) -> float {
	auto s = std::complex<float>{1.0f, 0.0f};
	s = tilt(s, f, coeffs.F, coeffs.tilt1);
	s = tilt(s, f, coeffs.F, coeffs.tilt2);
	s *= out_mult;
	return abs(s);
}

[[nodiscard]] inline
auto compute(const band_shelf::coeffs& coeffs, float f) -> float {
	return compute_impl(coeffs, f, coeffs.recip_Mpow2);
}

inline
auto compute(const band_shelf::coeffs& coeffs, std::span<const float> in_omega, std::span<float> out_magnitude) -> void {
	assert (in_omega.size() == out_magnitude.size());
	auto fn_compute = [&coeffs](float omega) -> float { return compute(coeffs, omega); };
	std::transform(in_omega.begin(), in_omega.end(), out_magnitude.begin(), fn_compute);
}

} // snd::filters::shelving::band_shelf::tf

namespace snd::filters::shelving::hilo {

struct tilt_coeffs {
	float damp;
	float M;
	float recip_M;
	float wMid;
};

struct coeffs {
	float F;
	float Mpow2;
	float recip_Mpow2;
    float wdiv2;
	tilt_coeffs tilt1;
	tilt_coeffs tilt2;
	tilt_coeffs tilt3;
	tilt_coeffs tilt4;
};

struct svf_state {
	integrator_state integrator1;
	integrator_state integrator2;
	float fbk_g = 0.0f;
	float fbk_s = 0.0f;
};

struct tilt_state {
	svf_state svf;
};

struct state {
	state() = default;
	tilt_state tilt1;
	tilt_state tilt2;
	tilt_state tilt3;
	tilt_state tilt4;
};

[[nodiscard]] inline
auto make_coeffs(float f, filters::gain_db gain_db, float slope, float SR) -> hilo::coeffs {
	const auto slope_mod              = std::pow(2.0f, slope);
	const auto M                      = db_to_linear(gain_db.v);
	const auto Mpow2                  = std::pow(M, 2.0f);
	const auto recip_Mpow2            = reciprocal(Mpow2);
	const auto [slope1_lo, slope1_hi] = bi_clip(slope_mod, 2.0f);
	const auto slope1_hi_half         = slope1_hi / 2.0f;
	const auto [slope2_lo, slope2_hi] = bi_clip(slope1_hi_half, 2.0f);
	const auto slope2_hi_half         = slope2_hi / 2.0f;
	const auto g2R_M                  = Mpow2 + recip_Mpow2;
	const auto g2R1                   = g2R(slope1_lo, g2R_M);
	const auto g2R2                   = g2R(slope2_lo, g2R_M);
	const auto g2R3                   = g2R(slope2_hi_half, g2R_M);
	const auto g2R1_clamp             = std::min(1.0f, g2R1);
	const auto g2R2_clamp             = std::min(1.0f, g2R2);
	const auto g2R3_clamp             = std::min(1.0f, g2R3);
	const auto R1x                    = g2R1 / g2R1_clamp;
	const auto a1                     = std::acos(g2R1_clamp);
	const auto a2                     = std::acos(g2R2_clamp);
	const auto a3                     = std::acos(g2R3_clamp);
	const auto g2R2_R2p1              = R2p1(g2R2, M);
	const auto g2R3_R2p1              = R2p1(g2R3, M);
	const auto M1                     = std::sqrt(M * g2R3_R2p1);
	const auto tilt34_M               = std::sqrt(M / g2R3_R2p1);
	const auto tilt1_M                = M1 * g2R2_R2p1;
	const auto tilt2_M                = M1 / g2R2_R2p1;
	const auto a2div2                 = a2 / 2.0f;
	const auto a3div4                 = a3 / 4.0f;
	const auto tilt1_damp             = a22R(a1 + a2div2) * R1x;
	const auto tilt2_damp             = a22R(a1 - a2div2);
	const auto tilt3_damp             = a22R((a1 + a2div2) - a3div4);
	const auto tilt4_damp             = a22R((a1 - a2div2) - a3div4);
	const auto tilt1_recip_M          = reciprocal(tilt1_M);
	const auto tilt2_recip_M          = reciprocal(tilt2_M);
	const auto tilt34_recip_M         = reciprocal(tilt34_M);
	const auto tilt1_wMid             = std::sqrt(tilt1_M);
	const auto tilt2_wMid             = std::sqrt(tilt2_M);
	const auto tilt34_wMid            = std::sqrt(tilt34_M);
	const auto wdiv2                  = compute_wdiv2(f, SR);
	return {
		.F           = f,
		.Mpow2       = Mpow2,
		.recip_Mpow2 = recip_Mpow2,
		.wdiv2       = wdiv2,
		.tilt1 = {
			tilt1_damp,
			tilt1_M,
			tilt1_recip_M,
			tilt1_wMid,
		},
		.tilt2 = {
			tilt2_damp,
			tilt2_M,
			tilt2_recip_M,
			tilt2_wMid,
		},
		.tilt3 = {
			tilt3_damp,
			tilt34_M,
			tilt34_recip_M,
			tilt34_wMid,
		},
		.tilt4 = {
			tilt4_damp,
			tilt34_M,
			tilt34_recip_M,
			tilt34_wMid,
		},
	};
}

[[nodiscard]] inline
auto svf(svf_state state, float x, float wdiv2, float damp) -> std::tuple<svf_state, float, float, float> {
	auto zdf                            = std::complex<float>{1.0f, 0.0f};
	auto BPn_zdf                        = std::complex<float>{};
	auto BPn                            = 0.0f;
	const auto y                        = solve_y(x, state.fbk_g, state.fbk_s);
	x                                   = x + y;
	const auto HP                       = x;
	std::tie(state.integrator1, x, zdf) = integrator(state.integrator1, x, zdf, wdiv2);
	std::tie(BPn, BPn_zdf)              = gain(x, zdf, damp);
	std::tie(state.integrator2, x, zdf) = integrator(state.integrator2, x, zdf, wdiv2);
	const auto LP                       = x;
	state.fbk_g                         = (zdf.real() + BPn_zdf.real()) * -1.0f;
	state.fbk_s                         = (zdf.imag() + BPn_zdf.imag()) * -1.0f;
	return std::make_tuple(state, LP, HP, BPn);
}

[[nodiscard]] inline
auto tilt(tilt_state state, float in, float wdiv2, tilt_coeffs coeffs) -> std::tuple<tilt_state, float> {
	wdiv2 *= std::sqrt(coeffs.M);
	float LP, HP, BPn;
	std::tie(state.svf, LP, HP, BPn) = svf(state.svf, in, wdiv2, coeffs.damp);
	return std::make_tuple(state, (LP / coeffs.M) + (HP * coeffs.M) + BPn);
}

[[nodiscard]] inline
auto process(hilo::state state, float in, const hilo::coeffs& coeffs) -> std::tuple<hilo::state, float> {
	auto signal = in;
	std::tie(state.tilt1, signal) = tilt(state.tilt1, signal, coeffs.wdiv2, coeffs.tilt1);
	std::tie(state.tilt2, signal) = tilt(state.tilt2, signal, coeffs.wdiv2, coeffs.tilt2);
	std::tie(state.tilt3, signal) = tilt(state.tilt3, signal, coeffs.wdiv2, coeffs.tilt3);
	std::tie(state.tilt4, signal) = tilt(state.tilt4, signal, coeffs.wdiv2, coeffs.tilt4);
	return std::make_tuple(state, signal);
}

[[nodiscard]] inline
auto settle(hilo::state state, const hilo::coeffs& coeffs) -> hilo::state {
	for (size_t i = 0; i < SETTLE_FRAMES; i++) {
		std::tie(state, std::ignore) = process(state, 0.0f, coeffs);
	}
	return state;
}

} // snd::filters::shelving::hilo

namespace snd::filters::shelving::hilo::tf {

[[nodiscard]] inline
auto svf(float f, float F, float _2R) -> svf_result {
	return shelving::tf::svf({0.0f, f / F}, _2R);
}

[[nodiscard]] inline
auto tilt(std::complex<float> t, float f, float F, tilt_coeffs coeffs) -> std::complex<float> {
	const auto [LP, BPn, HP] = svf(f, F * coeffs.wMid, coeffs.damp);
	const auto a = LP * coeffs.recip_M;
	const auto b = HP * coeffs.M;
	const auto c = a + b;
	const auto d = c + BPn;
	return t * d;
}

[[nodiscard]] inline
auto compute(const hilo::coeffs& coeffs, float f) -> std::complex<float> {
	auto s = std::complex<float>{1.0f, 0.0f};
	s = hilo::tf::tilt(s, f, coeffs.F, coeffs.tilt1);
	s = hilo::tf::tilt(s, f, coeffs.F, coeffs.tilt2);
	s = hilo::tf::tilt(s, f, coeffs.F, coeffs.tilt3);
	s = hilo::tf::tilt(s, f, coeffs.F, coeffs.tilt4);
	return s;
}

} // snd::filters::shelving::hilo::tf

namespace snd::filters::shelving::band_pass {

struct coeffs { band_shelf::coeffs band_shelf; };
struct state { band_shelf::state band_shelf; };

[[nodiscard]] inline
auto make_coeffs(float f, filters::gain_db gain_db, float slope, float bw, float SR) -> coeffs {
	return {.band_shelf = band_shelf::make_coeffs(f, gain_db, slope, bw, SR)};
}

[[nodiscard]] inline
auto process(band_pass::state state, float in, const band_pass::coeffs& coeffs) -> std::tuple<band_pass::state, float> {
	auto signal = in;
	std::tie(state.band_shelf, signal) = band_shelf::process_impl(state.band_shelf, signal, coeffs.band_shelf, coeffs.band_shelf.Mpow2);
	return std::make_tuple(state, signal);
}

[[nodiscard]] inline
auto settle(band_pass::state state, const band_pass::coeffs& coeffs) -> band_pass::state {
	state.band_shelf = band_shelf::settle_impl(state.band_shelf, coeffs.band_shelf, coeffs.band_shelf.Mpow2);
	return state;
}

} // snd::filters::shelving::band_pass

namespace snd::filters::shelving::band_pass::tf {

[[nodiscard]] inline
auto compute(const band_pass::coeffs& coeffs, float f) -> float {
	return abs(band_shelf::tf::compute_impl(coeffs.band_shelf, f, coeffs.band_shelf.Mpow2));
}

inline
auto compute(const band_pass::coeffs& coeffs, std::span<const float> in_omega, std::span<float> out_magnitude) -> void {
	assert (in_omega.size() == out_magnitude.size());
	auto fn_compute = [&coeffs](float omega) -> float { return compute(coeffs, omega); };
	std::transform(in_omega.begin(), in_omega.end(), out_magnitude.begin(), fn_compute);
}

} // snd::filters::shelving::band_pass::tf

namespace snd::filters::shelving::hi_shelf {

struct coeffs { hilo::coeffs hilo; };
struct state { hilo::state hilo; };

[[nodiscard]] inline
auto make_coeffs(float f, inverted_gain_db gain_db, float slope, float SR) -> coeffs {
	return {.hilo = hilo::make_coeffs(f, filters::gain_db{gain_db.v}, slope, SR)};
}

[[nodiscard]] inline
auto process(hi_shelf::state state, float in, const hi_shelf::coeffs& coeffs) -> std::tuple<hi_shelf::state, float> {
	auto signal = in;
	std::tie(state.hilo, signal) = hilo::process(state.hilo, signal, coeffs.hilo);
	signal *= coeffs.hilo.Mpow2;
	return std::make_tuple(state, signal);
}

[[nodiscard]] inline
auto settle(hi_shelf::state state, const hi_shelf::coeffs& coeffs) -> hi_shelf::state {
	state.hilo = hilo::settle(state.hilo, coeffs.hilo);
	return state;
}

} // snd::filters::shelving::hi_shelf

namespace snd::filters::shelving::hi_shelf::tf {

[[nodiscard]] inline
auto compute(const hi_shelf::coeffs& coeffs, float f) -> float {
	return abs(hilo::tf::compute(coeffs.hilo, f)) * coeffs.hilo.Mpow2;
}

inline
auto compute(const hi_shelf::coeffs& coeffs, std::span<const float> in_omega, std::span<float> out_magnitude) -> void {
	assert (in_omega.size() == out_magnitude.size());
	auto fn_compute = [&coeffs](float omega) -> float { return compute(coeffs, omega); };
	std::transform(in_omega.begin(), in_omega.end(), out_magnitude.begin(), fn_compute);
}

} // snd::filters::shelving::hi_shelf::tf

namespace snd::filters::shelving::lo_shelf {

struct coeffs { hilo::coeffs hilo; };
struct state { hilo::state hilo; };

[[nodiscard]] inline
auto make_coeffs(float f, filters::gain_db gain_db, float slope, float SR) -> coeffs {
	return {.hilo = hilo::make_coeffs(f, gain_db, slope, SR)};
}

[[nodiscard]] inline
auto process(lo_shelf::state state, float in, const lo_shelf::coeffs& coeffs) -> std::tuple<lo_shelf::state, float> {
	auto signal = in;
	std::tie(state.hilo, signal) = hilo::process(state.hilo, signal, coeffs.hilo);
	signal *= coeffs.hilo.recip_Mpow2;
	return std::make_tuple(state, signal);
}

[[nodiscard]] inline
auto settle(lo_shelf::state state, const lo_shelf::coeffs& coeffs) -> lo_shelf::state {
	state.hilo = hilo::settle(state.hilo, coeffs.hilo);
	return state;
}

} // snd::filters::shelving::lo_shelf

namespace snd::filters::shelving::lo_shelf::tf {

[[nodiscard]] inline
auto compute(const lo_shelf::coeffs& coeffs, float f) -> float {
	return abs(hilo::tf::compute(coeffs.hilo, f)) * coeffs.hilo.recip_Mpow2;
}

inline
auto compute(const lo_shelf::coeffs& coeffs, std::span<const float> in_omega, std::span<float> out_magnitude) -> void {
	assert (in_omega.size() == out_magnitude.size());
	auto fn_compute = [&coeffs](float omega) -> float { return compute(coeffs, omega); };
	std::transform(in_omega.begin(), in_omega.end(), out_magnitude.begin(), fn_compute);
}

} // snd::filters::shelving::lo_shelf::tf
