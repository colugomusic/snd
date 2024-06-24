#pragma once

#include "dc_bias.hpp"
#include "../ease.hpp"
#include "../misc.hpp"
#include <numeric>
#include <vector>

namespace snd {
namespace poka {

// POKA "Pretty O.K. Autocorrelation"

template <typename Fn0, typename Fn1, typename Fn2>
struct cb {
	Fn0 get_frames;
	Fn1 report_progress;
	Fn2 should_abort;
};

template <typename Fn0, typename Fn1, typename Fn2>
[[nodiscard]] inline
auto make_cbs(Fn0 get_frames, Fn1 report_progress, Fn2 should_abort) {
	return cb<Fn0, Fn1, Fn2>{get_frames, report_progress, should_abort};
}

struct range {
	size_t beg, end;
};

struct zenith {
	size_t pos;
	float value = std::numeric_limits<float>::lowest();
};

struct nadir {
	size_t pos;
	float value = std::numeric_limits<float>::max();
};

struct cycle_info {
	poka::zenith zenith;
	poka::nadir nadir;
	size_t dive_pos;
	size_t best_match;
};

struct cycle {
	poka::range range;
	cycle_info info;
};

struct work {
	struct {
		std::vector<float> raw;
		std::vector<float> smoothed;
	} frames;
	std::vector<cycle> cycles;
};

struct result {
	struct {
		std::vector<float> estimated_size;
	} frames;
};

namespace detail {

static constexpr auto WORK_COST_READ_FRAMES     = 1;
static constexpr auto WORK_COST_SMOOTH_FRAMES   = 2;
static constexpr auto WORK_COST_FIND_CYCLES     = 2;
static constexpr auto WORK_COST_AUTOCORRELATION = 4;
static constexpr auto WORK_COST_WRITE_SIZES     = 2;
static constexpr auto WORK_COST_REMOVE_DC_BIAS  = 2;
static constexpr auto WORK_COST_TOTAL =
	WORK_COST_READ_FRAMES + WORK_COST_SMOOTH_FRAMES + WORK_COST_FIND_CYCLES +
	WORK_COST_AUTOCORRELATION + WORK_COST_WRITE_SIZES + WORK_COST_REMOVE_DC_BIAS;
static constexpr auto WORK_COST_NORM_READ_FRAMES     = float(WORK_COST_READ_FRAMES) / float(WORK_COST_TOTAL);
static constexpr auto WORK_COST_NORM_SMOOTH_FRAMES   = float(WORK_COST_SMOOTH_FRAMES) / float(WORK_COST_TOTAL);
static constexpr auto WORK_COST_NORM_FIND_CYCLES     = float(WORK_COST_FIND_CYCLES) / float(WORK_COST_TOTAL);
static constexpr auto WORK_COST_NORM_AUTOCORRELATION = float(WORK_COST_AUTOCORRELATION) / float(WORK_COST_TOTAL);
static constexpr auto WORK_COST_NORM_WRITE_SIZES     = float(WORK_COST_WRITE_SIZES) / float(WORK_COST_TOTAL);
static constexpr auto WORK_COST_NORM_REMOVE_DC_BIAS  = float(WORK_COST_REMOVE_DC_BIAS) / float(WORK_COST_TOTAL);

template <typename ReportProgressFn>
struct progress_reporter {
	ReportProgressFn report_progress;
	float total_work = 0.0f;
	float work_done  = 0.0f;
};

template <typename ReportProgressFn>
auto make_progress_reporter(ReportProgressFn report_progress) {
	return progress_reporter<ReportProgressFn>{report_progress};
}

template <typename ReportProgressFn>
auto add_work_to_do(progress_reporter<ReportProgressFn>* reporter, float work) -> void {
	reporter->total_work += work;
}

template <typename ReportProgressFn>
auto complete_work(progress_reporter<ReportProgressFn>* reporter, float work) -> void {
	const auto quantized_work_done_prv = size_t(reporter->work_done * 100);
	reporter->work_done += work;
	const auto quantized_work_done_now = size_t(reporter->work_done * 100);
	if (quantized_work_done_now > quantized_work_done_prv) {
		reporter->report_progress(reporter->work_done / reporter->total_work);
	}
}

template <size_t BufferSize, typename CB> [[nodiscard]] inline
auto read_frames(poka::work* work, CB cb, size_t n) -> void {
	work->frames.raw.resize(n);
	size_t frames_remaining = n;
	size_t frame_read_index = 0;
	for (;;) {
		size_t frames_to_read = std::min(frames_remaining, BufferSize);
		cb.get_frames(frame_read_index, frames_to_read, work->frames.raw.data() + frame_read_index);
		frames_remaining -= frames_to_read;
		frame_read_index += frames_to_read;
		if (frames_remaining == 0) {
			return;
		}
	}
}

inline
auto add_cycle(poka::work* work, size_t idx, poka::range range, size_t dive_pos) -> void {
	cycle_info info;
	info.best_match = range.end;
	info.dive_pos   = dive_pos;
	for (size_t i = range.beg; i < range.end; i++) {
		const auto value = work->frames.smoothed[i];
		if (value > info.zenith.value) {
			info.zenith.value = value;
			info.zenith.pos   = i;
		}
		if (value < info.nadir.value) {
			info.nadir.value = value;
			info.nadir.pos   = i;
		}
	}
	work->cycles.resize(idx + 1);
	work->cycles[idx].info  = info;
	work->cycles[idx].range = range;
}

template <typename ProgressReporter>
auto find_cycles(poka::work* work, ProgressReporter* progress_reporter) -> void {
	static constexpr auto MIN_CYCLE_LEN = 2;
	const auto frame_count = work->frames.smoothed.size();
	bool init = false;
	bool up_flag = true;
	size_t cycle_beg = 0;
	size_t cycle_idx = 0;
	size_t dive_pos = 0;
	for (size_t i = 0; i < frame_count; i++) {
		bool just_crossed    = false;
		const auto value     = work->frames.smoothed[i];
		const auto value_up  = value > 0;
		const auto cycle_len = i - cycle_beg;
		if (!up_flag && value_up) {
			up_flag = true;
			if (cycle_len >= MIN_CYCLE_LEN) {
				just_crossed = true;
			}
		}
		if (up_flag && !value_up) {
			up_flag  = false;
			dive_pos = i;
		}
		if (just_crossed) {
			if (init) {
				add_cycle(work, cycle_idx, {cycle_beg, i}, dive_pos);
				cycle_idx++;
			}
			cycle_beg = i;
			init = true;
		}
		complete_work(progress_reporter, (1.0f / frame_count) * WORK_COST_NORM_FIND_CYCLES);
	}
}

[[nodiscard]] inline
auto compare(const poka::work& work, poka::range context_a, poka::range context_b, size_t cycle_idx_a, size_t cycle_idx_b) -> float {
	const auto& cycle_a = work.cycles[cycle_idx_a];
	const auto& cycle_b = work.cycles[cycle_idx_b];
	const auto zenith_a = cycle_a.info.zenith;
	const auto zenith_b = cycle_b.info.zenith;
	const auto nadir_a  = cycle_a.info.nadir;
	const auto nadir_b  = cycle_b.info.nadir;
	const auto dive_a   = cycle_a.info.dive_pos;
	const auto dive_b   = cycle_b.info.dive_pos;
	const auto dive_a_dist = int(dive_a - context_a.beg);
	const auto dive_b_dist = int(dive_b - context_b.beg);
	const auto beg_a_dist  = int(cycle_a.range.beg - context_a.beg);
	const auto beg_b_dist  = int(cycle_b.range.beg - context_b.beg);
	const auto end_a_dist  = int(cycle_a.range.end - context_a.beg);
	const auto end_b_dist  = int(cycle_b.range.end - context_b.beg);
	const auto zenith_a_dist = int(zenith_a.pos - context_a.beg);
	const auto zenith_b_dist = int(zenith_b.pos - context_b.beg);
	const auto nadir_a_dist  = int(nadir_a.pos - context_a.beg);
	const auto nadir_b_dist  = int(nadir_b.pos - context_b.beg);
	const auto context_a_len = context_a.end - context_a.beg;
	const auto context_b_len = context_b.end - context_b.beg;
	const auto total_len     = context_a_len + context_b_len;
	const auto diff_dive       = float(std::abs(dive_a_dist - dive_b_dist)) / float(total_len);
	const auto diff_beg        = float(std::abs(beg_a_dist - beg_b_dist)) / float(total_len);
	const auto diff_end        = float(std::abs(end_a_dist - end_b_dist)) / float(total_len);
	const auto diff_zenith_pos = float(std::abs(zenith_a_dist - zenith_b_dist)) / float(total_len);
	const auto diff_nadir_pos  = float(std::abs(nadir_a_dist - nadir_b_dist)) / float(total_len);
	const auto diff_zenith_val = std::abs(zenith_a.value - zenith_b.value);
	const auto diff_nadir_val  = std::abs(nadir_a.value - nadir_b.value);
	return diff_beg + diff_end + diff_dive + diff_zenith_pos + diff_nadir_pos + diff_zenith_val + diff_nadir_val;
}

[[nodiscard]] inline
auto make_context(const poka::work& work, size_t beg, size_t end) -> poka::range {
	const auto cycle_beg_range       = work.cycles[beg].range;
	const auto cycle_end_sub_1_range = work.cycles[end-1].range;
	return { cycle_beg_range.beg, cycle_end_sub_1_range.end };
}

inline
auto autocorrelation(poka::work* work, size_t cycle_idx, size_t max_depth) -> void {
	static constexpr auto AUTO_LOSE = 1.0f;
	static constexpr auto AUTO_WIN  = 0.05f;
	auto& cycle = work->cycles[cycle_idx];
	float best_diff = std::numeric_limits<float>::max();
	for (size_t depth = 1; depth < max_depth; depth++) {
		if (cycle_idx + (depth * 2) >= work->cycles.size()) {
			break;
		}
		float total_diff = 0.0f;
		const auto context_a = make_context(*work, cycle_idx + 0    , cycle_idx + (depth * 1));
		const auto context_b = make_context(*work, cycle_idx + depth, cycle_idx + (depth * 2));
		for (size_t i = 0; i < depth; i++) {
			const auto cycle_idx_a = cycle_idx + i;
			const auto cycle_idx_b = cycle_idx + depth + i;
			const auto diff = compare(*work, context_a, context_b, cycle_idx_a, cycle_idx_b);
			total_diff += diff;
			if ((total_diff / depth) >= best_diff) {
				break;
			}
			if ((total_diff / depth) >= AUTO_LOSE) {
				break;
			}
		}
		if ((total_diff / depth) < best_diff) {
			best_diff = total_diff / depth;
			cycle.info.best_match = context_b.beg;
			if (best_diff < AUTO_WIN) {
				return;
			}
		}
	}
}

inline
auto remove_dc_bias(poka::work* work, size_t window_size) -> void {
	audio::dc_bias::detection detection;
	audio::dc_bias::const_frames const_frames{work->frames.raw.data(), work->frames.raw.size()};
	audio::dc_bias::frames frames{work->frames.raw.data(), work->frames.raw.size()};
	audio::dc_bias::detect(const_frames, window_size, &detection);
	audio::dc_bias::apply_correction(frames, detection);
}

template <typename ProgressReporter>
auto smooth_frames(poka::work* work, ProgressReporter* progress_reporter, size_t window_size) -> void {
	work->frames.smoothed.resize(work->frames.raw.size());
	if (work->frames.raw.size() < window_size) {
		std::copy(work->frames.raw.begin(), work->frames.raw.end(), work->frames.smoothed.begin());
		return;
	}
	poka::range window;
	window.beg = 0;
	window.end = window_size;
	for (size_t i = 0; i < work->frames.raw.size(); i++) {
		const auto window_beg = size_t(std::max(0, int(i) - int(window_size)));
		const auto window_end = size_t(std::min(work->frames.raw.size(), i + window_size));
		const auto sum = std::accumulate(work->frames.raw.begin() + window_beg, work->frames.raw.begin() + window_end, 0.0f);
		work->frames.smoothed[i] = sum / float(window_end - window_beg);
		complete_work(progress_reporter, (1.0f / work->frames.raw.size()) * WORK_COST_NORM_SMOOTH_FRAMES);
	}
}

template <typename ProgressReporter> inline
auto write_estimated_sizes(const poka::work& work, ProgressReporter* progress_reporter, poka::result* out) -> void {
	// Beginning
	{
		const auto cycle = work.cycles.front();
		const auto size  = float(cycle.info.best_match - cycle.range.beg);
		std::fill(out->frames.estimated_size.begin(), out->frames.estimated_size.begin() + cycle.range.end, size);
	}
	// Middle
	for (size_t i = 1; i < work.cycles.size() - 1; i++) {
		const auto& cycle_a = work.cycles[i-1];
		const auto& cycle_b = work.cycles[i-0];
		const auto cycle_b_len = float(cycle_b.range.end - cycle_b.range.beg);
		const auto size_a      = float(cycle_a.info.best_match - cycle_a.range.beg);
		const auto size_b      = float(cycle_b.info.best_match - cycle_b.range.beg);
		for (size_t j = cycle_b.range.beg; j < cycle_b.range.end; j++) {
			const auto t                  = float(j - cycle_b.range.beg) / cycle_b_len;
			out->frames.estimated_size[j] = lerp(size_a, size_b, ease::quadratic::in_out(t));
		}
		complete_work(progress_reporter, (1.0f / work.cycles.size()) * WORK_COST_NORM_WRITE_SIZES);
	}
	// End
	{
		const auto cycle = work.cycles.back();
		const auto size  = float(cycle.info.best_match - cycle.range.beg);
		std::fill(out->frames.estimated_size.begin() + cycle.range.beg, out->frames.estimated_size.end(), size);
	}
}

inline
auto no_cycles(poka::result* out) -> void {
	static constexpr auto DEFAULT_SIZE = 44100.0f;
	std::fill(out->frames.estimated_size.begin(), out->frames.estimated_size.end(), DEFAULT_SIZE);
}

inline
auto one_cycle(const poka::work& work, poka::result* out) -> void {
	const auto& cycle = work.cycles.front();
	const auto size   = float(cycle.range.end - cycle.range.beg);
	std::fill(out->frames.estimated_size.begin(), out->frames.estimated_size.end(), size);
}

} // detail

template <typename CB> [[nodiscard]] inline
auto autocorrelation(poka::work* work, CB cb, size_t n, size_t depth, size_t SR, result* out) -> bool {
	out->frames.estimated_size.resize(n);
	auto progress_reporter = detail::make_progress_reporter(cb.report_progress);
	detail::add_work_to_do(&progress_reporter, 1.0f);
	detail::read_frames<512>(work, cb, n);
	detail::complete_work(&progress_reporter, detail::WORK_COST_NORM_READ_FRAMES);
	if (cb.should_abort()) { return false; }
	detail::remove_dc_bias(work, SR / 40);
	detail::complete_work(&progress_reporter, detail::WORK_COST_NORM_REMOVE_DC_BIAS);
	if (cb.should_abort()) { return false; }
	detail::smooth_frames(work, &progress_reporter, 16);
	if (cb.should_abort()) { return false; }
	detail::find_cycles(work, &progress_reporter);
	if (cb.should_abort()) { return false; }
	if (work->cycles.empty()) {
		detail::no_cycles(out);
		return true;
	}
	if (work->cycles.size() < 2) {
		detail::one_cycle(*work, out);
		return true;
	}
	for (size_t i = 0; i < work->cycles.size(); i++) {
		if (cb.should_abort()) {
			return false;
		}
		detail::autocorrelation(work, i, depth);
		detail::complete_work(&progress_reporter, (1.0f / work->cycles.size()) * detail::WORK_COST_NORM_AUTOCORRELATION);
	}
	detail::write_estimated_sizes(*work, &progress_reporter, out);
	return true;
}

} // poka
} // snd