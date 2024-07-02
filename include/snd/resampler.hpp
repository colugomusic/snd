#pragma once

#include <array>
#include <cassert>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {

template <size_t ROWS>
struct resampler {
	ml::DSPVectorArray<ROWS> prev_source_block;
	ml::DSPVectorArray<ROWS> curr_source_block;
	int64_t curr_block_index = -1;
	double frame_pos         = 0.0;
};

namespace detail {

template <typename SourceFn, size_t ROWS>
auto read_next_source_block(resampler<ROWS>* rs, SourceFn source_fn) -> void {
	rs->prev_source_block = rs->curr_source_block;
	rs->curr_source_block = source_fn();
	rs->curr_block_index++;
}

template <size_t ROWS> [[nodiscard]]
auto read_source_block_frame(const ml::DSPVectorArray<ROWS>& block, int index) -> std::array<float, ROWS> {
	std::array<float, ROWS> out; 
	for (int r = 0; r < ROWS; r++) {
		out[r] = block.constRow(r)[index];
	} 
	return out;
}

template <typename SourceFn, size_t ROWS> [[nodiscard]]
auto read_source_block_frame(resampler<ROWS>* rs, SourceFn source_fn, uint32_t index) -> std::array<float, ROWS> {
	const auto block_index = index / int(kFloatsPerDSPVector);
	const auto local_index = index % int(kFloatsPerDSPVector); 
	assert(block_index >= rs->curr_block_index - 1);
	assert(block_index <= rs->curr_block_index + 1); 
	if (block_index < rs->curr_block_index) {
		return read_source_block_frame(rs->prev_source_block, local_index);
	} 
	if (block_index > rs->curr_block_index) {
		read_next_source_block(rs, source_fn);
	} 
	return read_source_block_frame(rs->curr_source_block, local_index);
}

template <typename SourceFn, size_t ROWS> [[nodiscard]]
auto read_source_block_frame(resampler<ROWS>* rs, SourceFn source_fn, double pos) -> std::array<float, ROWS> {
	const auto idx_prev = static_cast<uint32_t>(std::floor(pos));
	const auto idx_next = static_cast<uint32_t>(std::ceil(pos));
	const auto x = pos - idx_prev;
	const auto prev = read_source_block_frame(rs, source_fn, idx_prev);
	const auto next = read_source_block_frame(rs, source_fn, idx_next);
	std::array<float, ROWS> out;
	for (int r = 0; r < ROWS; r++) {
		out[r] = ml::lerp(prev[r], next[r], float(x));
	} 
	return out;
}

} // detail

template <typename SourceFn, size_t ROWS> [[nodiscard]]
auto process(resampler<ROWS>* rs, SourceFn source_fn, float factor) -> ml::DSPVectorArray<ROWS> {
	ml::DSPVectorArray<ROWS> out;
	for (int i = 0; i < kFloatsPerDSPVector; i++) {
		const auto frame = detail::read_source_block_frame(rs, source_fn, rs->frame_pos); 
		rs->frame_pos += factor; 
		for (int r = 0; r < ROWS; r++) {
			out.row(r)[i] = frame[r];
		}
	} 
	return out;
}

} // snd