#pragma once

#include "dc_bias.hpp"
#include "../ease.hpp"
#include "../misc.hpp"
#include <chrono>
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
};

struct work {
	struct {
		std::vector<float> raw;
		std::vector<float> smoothed;
		std::vector<float> estimated_size;
	} frames;
	std::vector<poka::cycle_info> cycle_info;
	std::vector<poka::range> cycle_range;
	std::vector<size_t> cycle_best_match;
};

struct output {
	struct {
		std::vector<float> estimated_size;
	} frames;
};

enum class result { ok, aborted };
enum class mode { milestones, classic };

namespace detail {

static constexpr auto WORK_COST_READ_FRAMES     = 1;
static constexpr auto WORK_COST_REMOVE_DC_BIAS  = 8;
static constexpr auto WORK_COST_PRE_SMOOTH      = 200;
static constexpr auto WORK_COST_FIND_CYCLES     = 10;
static constexpr auto WORK_COST_AUTOCORRELATION = 4000;
static constexpr auto WORK_COST_WRITE_SIZES     = 1;
static constexpr auto WORK_COST_POST_SMOOTH     = 1000;
static constexpr auto WORK_COST_TOTAL =
	WORK_COST_READ_FRAMES + WORK_COST_PRE_SMOOTH + WORK_COST_FIND_CYCLES +
	WORK_COST_AUTOCORRELATION + WORK_COST_WRITE_SIZES + WORK_COST_REMOVE_DC_BIAS +
	WORK_COST_POST_SMOOTH;

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

template <size_t BufferSize, typename CB> inline
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
	info.dive_pos = dive_pos;
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
	work->cycle_info.resize(idx + 1);
	work->cycle_range.resize(idx + 1);
	work->cycle_best_match.resize(idx + 1);
	work->cycle_info[idx]  = info;
	work->cycle_range[idx] = range;
	work->cycle_best_match[idx] = range.end;
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
		complete_work(progress_reporter, (1.0f / frame_count) * WORK_COST_FIND_CYCLES);
	}
}

[[nodiscard]] inline
auto milestones_compare(const poka::work& work, poka::range context_a, poka::range context_b, size_t cycle_idx_a, size_t cycle_idx_b) -> float {
	const auto& cycle_info_a = work.cycle_info[cycle_idx_a];
	const auto& cycle_info_b = work.cycle_info[cycle_idx_b];
	const auto& cycle_range_a = work.cycle_range[cycle_idx_a];
	const auto& cycle_range_b = work.cycle_range[cycle_idx_b];
	const auto zenith_a = cycle_info_a.zenith;
	const auto zenith_b = cycle_info_b.zenith;
	const auto nadir_a  = cycle_info_a.nadir;
	const auto nadir_b  = cycle_info_b.nadir;
	const auto dive_a   = cycle_info_a.dive_pos;
	const auto dive_b   = cycle_info_b.dive_pos;
	const auto dive_a_dist = int(dive_a - context_a.beg);
	const auto dive_b_dist = int(dive_b - context_b.beg);
	const auto beg_a_dist  = int(cycle_range_a.beg - context_a.beg);
	const auto beg_b_dist  = int(cycle_range_b.beg - context_b.beg);
	const auto end_a_dist  = int(cycle_range_a.end - context_a.beg);
	const auto end_b_dist  = int(cycle_range_b.end - context_b.beg);
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
	const auto cycle_beg_range       = work.cycle_range[beg];
	const auto cycle_end_sub_1_range = work.cycle_range[end-1];
	return { cycle_beg_range.beg, cycle_end_sub_1_range.end };
}

inline
auto remove_dc_bias(poka::work* work, size_t window_size) -> void {
	audio::dc_bias::detection detection;
	audio::dc_bias::const_frames const_frames{work->frames.raw.data(), work->frames.raw.size()};
	audio::dc_bias::frames frames{work->frames.raw.data(), work->frames.raw.size()};
	audio::dc_bias::detect(const_frames, window_size, &detection);
	audio::dc_bias::apply_correction(frames, detection);
}

[[nodiscard]] inline
auto smooth_value(const std::vector<float>& in, size_t index, size_t window_size) -> float {
	const auto window_beg = size_t(std::max(0, int(index) - int(window_size)));
	const auto window_end = size_t(std::min(in.size(), index + window_size));
	const auto sum = std::accumulate(in.begin() + window_beg, in.begin() + window_end, 0.0f);
	return sum / float(window_end - window_beg);
}

template <typename ProgressReporter>
auto pre_smooth(poka::work* work, ProgressReporter* progress_reporter, size_t window_size) -> void {
	work->frames.smoothed.resize(work->frames.raw.size());
	if (work->frames.raw.size() < window_size) {
		std::copy(work->frames.raw.begin(), work->frames.raw.end(), work->frames.smoothed.begin());
		return;
	}
	for (size_t i = 0; i < work->frames.raw.size(); i++) {
		work->frames.smoothed[i] = smooth_value(work->frames.raw, i, window_size);
		complete_work(progress_reporter, (1.0f / work->frames.raw.size()) * WORK_COST_PRE_SMOOTH);
	}
}

template <typename ProgressReporter>
auto post_smooth(const poka::work& work, ProgressReporter* progress_reporter, size_t window_size, poka::output* out) -> void {
	out->frames.estimated_size.resize(work.frames.raw.size());
	if (work.frames.raw.size() < window_size) {
		std::copy(work.frames.estimated_size.begin(), work.frames.estimated_size.end(), out->frames.estimated_size.begin());
		return;
	}
	for (size_t i = 0; i < work.frames.raw.size(); i++) {
		out->frames.estimated_size[i] = smooth_value(work.frames.estimated_size, i, window_size);
		complete_work(progress_reporter, (1.0f / work.frames.raw.size()) * WORK_COST_POST_SMOOTH);
	}
}

template <typename ProgressReporter> inline
auto write_estimated_sizes(poka::work* work, ProgressReporter* progress_reporter) -> void {
	work->frames.estimated_size.resize(work->frames.raw.size());
	// Beginning
	{
		const auto cycle_range      = work->cycle_range.front();
		const auto cycle_best_match = work->cycle_best_match.front();
		const auto size             = float(cycle_best_match - cycle_range.beg);
		std::fill(work->frames.estimated_size.begin(), work->frames.estimated_size.begin() + cycle_range.end, size);
	}
	// Middle
	for (size_t i = 1; i < work->cycle_info.size() - 1; i++) {
		const auto& cycle_best_match_a = work->cycle_best_match[i-1];
		const auto& cycle_best_match_b = work->cycle_best_match[i-0];
		const auto& cycle_range_a      = work->cycle_range[i-1];
		const auto& cycle_range_b      = work->cycle_range[i-0];
		const auto cycle_range_b_len   = float(cycle_range_b.end - cycle_range_b.beg);
		const auto size_a              = float(cycle_best_match_a - cycle_range_a.beg);
		const auto size_b              = float(cycle_best_match_b - cycle_range_b.beg);
		for (size_t j = cycle_range_b.beg; j < cycle_range_b.end; j++) {
			const auto t                   = float(j - cycle_range_b.beg) / cycle_range_b_len;
			work->frames.estimated_size[j] = lerp(size_a, size_b, t);
		}
		complete_work(progress_reporter, (1.0f / work->cycle_info.size()) * WORK_COST_WRITE_SIZES);
	}
	// End
	{
		const auto cycle_range      = work->cycle_range.back();
		const auto cycle_best_match = work->cycle_best_match.back();
		const auto size             = float(cycle_best_match - cycle_range.beg);
		std::fill(work->frames.estimated_size.begin() + cycle_range.beg, work->frames.estimated_size.end(), size);
	}
}

inline
auto no_cycles(poka::output* out) -> void {
	static constexpr auto DEFAULT_SIZE = 44100.0f;
	std::fill(out->frames.estimated_size.begin(), out->frames.estimated_size.end(), DEFAULT_SIZE);
}

inline
auto one_cycle(const poka::work& work, poka::output* out) -> void {
	const auto cycle_range = work.cycle_range.front();
	const auto size   = float(cycle_range.end - cycle_range.beg);
	std::fill(out->frames.estimated_size.begin(), out->frames.estimated_size.end(), size);
}

inline
auto classic_cycle_autocorrelation(poka::work* work, size_t cycle_idx, size_t max_depth) -> void {
	static constexpr auto AUTO_LOSE = 1.0f;
	static constexpr auto AUTO_WIN  = 0.05f;
	float best_diff = std::numeric_limits<float>::max();
	for (size_t depth = 1; depth < max_depth; depth++) {
		const auto cycle_idx_a = cycle_idx;
		const auto cycle_idx_b = cycle_idx + depth;
		if (cycle_idx_b >= work->cycle_info.size()) {
			break;
		}
		const auto cycle_beg_a = work->cycle_range[cycle_idx_a].beg;
		const auto cycle_beg_b = work->cycle_range[cycle_idx_b].beg;
		const auto length  = cycle_beg_b - cycle_beg_a;
		const auto range_a = poka::range{cycle_beg_a, cycle_beg_b};
		const auto range_b = poka::range{cycle_beg_b, cycle_beg_b + length};
		float total_diff = 0.0f;
		for (size_t i = 0; i < depth * 4; i++) {
			const auto t       = float(i) / float(depth * 4);
			const auto frame_a = range_a.beg + size_t(length * t);
			const auto frame_b = range_b.beg + size_t(length * t);
			const auto value_a = work->frames.smoothed[frame_a];
			const auto value_b = frame_b >= work->frames.smoothed.size() ? 0.0f : work->frames.smoothed[frame_b];
			total_diff += std::abs(value_a - value_b);
			if ((total_diff / depth) >= best_diff) {
				break;
			}
			if ((total_diff / depth) >= AUTO_LOSE) {
				break;
			}
		}
		if ((total_diff / depth) < best_diff) {
			best_diff = total_diff / depth;
			work->cycle_best_match[cycle_idx] = range_b.beg;
			if (best_diff < AUTO_WIN) {
				return;
			}
		}
	}
}

inline
auto milestones_cycle_autocorrelation(poka::work* work, size_t cycle_idx, size_t max_depth) -> void {
	static constexpr auto AUTO_LOSE = 1.0f;
	static constexpr auto AUTO_WIN  = 0.05f;
	float best_diff = std::numeric_limits<float>::max();
	for (size_t depth = 1; depth < max_depth; depth++) {
		if (cycle_idx + (depth * 2) >= work->cycle_info.size()) {
			break;
		}
		float total_diff = 0.0f;
		const auto context_a = make_context(*work, cycle_idx + 0    , cycle_idx + (depth * 1));
		const auto context_b = make_context(*work, cycle_idx + depth, cycle_idx + (depth * 2));
		for (size_t i = 0; i < depth; i++) {
			const auto cycle_idx_a = cycle_idx + i;
			const auto cycle_idx_b = cycle_idx + depth + i;
			const auto diff = milestones_compare(*work, context_a, context_b, cycle_idx_a, cycle_idx_b);
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
			work->cycle_best_match[cycle_idx] = context_b.beg;
			if (best_diff < AUTO_WIN) {
				return;
			}
		}
	}
}

template <mode Mode, typename ShouldAbortFn, typename ProgressReporter> [[nodiscard]] inline
auto autocorrelation(poka::work* work, ShouldAbortFn should_abort, ProgressReporter* progress_reporter, size_t depth) -> bool {
	for (size_t i = 0; i < work->cycle_info.size(); i++) {
		if (should_abort()) {
			return false;
		}
		if constexpr (Mode == mode::milestones) {
			milestones_cycle_autocorrelation(work, i, depth);
		}
		else {
			classic_cycle_autocorrelation(work, i, depth);
		}
		complete_work(progress_reporter, (1.0f / work->cycle_info.size()) * detail::WORK_COST_AUTOCORRELATION);
	}
	return true;
}

template <mode Mode, typename ShouldAbortFn, typename ProgressReporter> [[nodiscard]] inline
auto autocorrelation(poka::work* work, ShouldAbortFn should_abort, ProgressReporter* progress_reporter, size_t depth, size_t SR, poka::output* out) -> poka::result {
	if (work->cycle_info.empty()) {
		no_cycles(out);
		complete_work(progress_reporter, WORK_COST_AUTOCORRELATION);
		return poka::result::ok;
	}
	if (work->cycle_info.size() < 2) {
		one_cycle(*work, out);
		complete_work(progress_reporter, WORK_COST_AUTOCORRELATION);
		return poka::result::ok;
	}
	if (!autocorrelation<Mode>(work, should_abort, progress_reporter, depth)) {
		return poka::result::aborted;
	}
	write_estimated_sizes(work, progress_reporter);
	if (should_abort()) { return poka::result::aborted; }
	detail::post_smooth(*work, progress_reporter, SR / 100, out);
	return poka::result::ok;
}

} // detail

template <mode Mode, typename CB> [[nodiscard]] inline
auto autocorrelation(poka::work* work, CB cb, size_t n, size_t depth, size_t SR, poka::output* out) -> poka::result {
	auto progress_reporter = detail::make_progress_reporter(cb.report_progress);
	detail::add_work_to_do(&progress_reporter, float(detail::WORK_COST_TOTAL));
	detail::read_frames<512>(work, cb, n);
	detail::complete_work(&progress_reporter, detail::WORK_COST_READ_FRAMES);
	if (cb.should_abort()) { return poka::result::aborted; }
	detail::remove_dc_bias(work, SR / 20);
	detail::complete_work(&progress_reporter, detail::WORK_COST_REMOVE_DC_BIAS);
	if (cb.should_abort()) { return poka::result::aborted; }
	detail::pre_smooth(work, &progress_reporter, 3);
	if (cb.should_abort()) { return poka::result::aborted; }
	detail::find_cycles(work, &progress_reporter);
	if (cb.should_abort()) { return poka::result::aborted; }
	return detail::autocorrelation<Mode>(work, cb.should_abort, &progress_reporter, depth, SR, out);
}

} // poka
} // snd