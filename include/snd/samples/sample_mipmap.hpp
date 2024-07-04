#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

namespace snd {
namespace mipmap {

// Mipmap representation of audio data, intended for waveform rendering,
// but also useful for audio analysis.
//
// All memory is allocated in mipmap::make().
//
// You can call write() in the audio thread if you're feeling adventurous
//
// There is no locking. It is the caller's responsibility not to
// simultaneously read and write to the same region
//
// REP is the underlying unsigned integer type which will
// be used to encode the frame data. The default of uint8_t
// is probably fine for most use cases.

template <typename REP = uint8_t> [[nodiscard]] constexpr auto VALUE_MAX() -> REP    { return std::numeric_limits<REP>::max() - 1; }
template <typename REP = uint8_t> [[nodiscard]] constexpr auto VALUE_MIN() -> REP    { return std::numeric_limits<REP>::min(); }
template <typename REP = uint8_t> [[nodiscard]] constexpr auto VALUE_SILENT() -> REP { return REP(std::floor(double(VALUE_MAX<REP>()) / 2)); }

template <typename REP = uint8_t> struct max   { REP value = VALUE_SILENT<REP>(); };
template <typename REP = uint8_t> struct min   { REP value = VALUE_SILENT<REP>(); };
template <typename REP = uint8_t> struct frame { mipmap::min<REP> min; mipmap::max<REP> max; };

struct channel_count   { uint16_t value = 0; };
struct channel_index   { uint16_t value = 0; };
struct detail          { uint8_t value = 0; };
struct frame_count     { size_t value = 0; };
struct lod_index       { size_t value = 0; };
struct bin_size        { int value = 0; };
struct max_source_clip { float value = 0.0f; };
struct region          { size_t beg = 0; size_t end = 0; };

template <typename REP>
struct lod {
	using channel_t = std::vector<frame<REP>>;
	using data_t    = std::vector<channel_t>;
	mipmap::lod_index index;
	mipmap::bin_size bin_size;
	data_t data;
	region valid_region;
};

template <typename REP>
struct lod0 {
	using channel_t = std::vector<REP>;
	using data_t    = std::vector<channel_t>;
	data_t data;
	region valid_region;
};

template <typename REP = uint8_t>
struct body {
	mipmap::channel_count channel_count;
	mipmap::frame_count frame_count;
	mipmap::detail detail;
	mipmap::max_source_clip max_source_clip;
	mipmap::lod0<REP> lod0;
	std::vector<mipmap::lod<REP>> lods;
};

template <typename REP> [[nodiscard]]
auto read(const mipmap::body<REP>& body, mipmap::lod_index lod_index, mipmap::channel_index channel, size_t lod_frame) -> mipmap::frame<REP>;

namespace detail_ {

template <typename T>
struct lerp_helper {
	struct { T a; T b; } index;
	float t;
};

[[nodiscard]] inline
auto is_empty(const region& r) -> bool {
	return r.beg >= r.end;
}

template <typename REP>
auto generate(const mipmap::body<>& body, mipmap::lod<REP>* lod, mipmap::detail detail, mipmap::channel_index channel, size_t frame) -> void {
	auto min = VALUE_MAX<REP>();
	auto max = VALUE_MIN<REP>();
	auto beg = frame * detail.value;
	auto end = beg + detail.value;
	for (size_t i = beg; i < end; i++) {
		const auto frame = mipmap::read(body, mipmap::lod_index{lod->index.value - 1}, channel, i);
		if (frame.min.value < min) min = frame.min.value;
		if (frame.max.value > max) max = frame.max.value;
	}
	lod->data[channel.value][frame] = { min, max };
}

template <typename REP>
auto generate(const mipmap::body<>& body, mipmap::lod<REP>* lod, mipmap::detail detail, mipmap::channel_index channel, mipmap::region region) -> void {
	for (size_t frame = region.beg; frame < region.end; frame++) {
		generate(body, lod, detail, channel, frame);
	}
}

template <typename REP>
auto generate(const mipmap::body<>& body, mipmap::lod<REP>* lod, mipmap::detail detail, mipmap::channel_count channel_count, mipmap::region region) -> void {
	if (region.beg < lod->valid_region.beg) lod->valid_region.beg = region.beg;
	if (region.end > lod->valid_region.end) lod->valid_region.end = region.end;
	for (uint16_t channel = 0; channel < channel_count.value; channel++) {
		generate(body, lod, detail, mipmap::channel_index{channel}, region);
	}
}

template <typename Result, typename Value> [[nodiscard]]
auto lerp(Value a, Value b, float t) -> Result {
	const auto af = static_cast<float>(a);
	const auto bf = static_cast<float>(b);
	return static_cast<Result>((t * (bf - af)) + af);
}

template <typename Result, typename Value, typename T> [[nodiscard]]
auto lerp(const lerp_helper<T>& lh, Value a, Value b) -> Result {
	return lerp<Result>(a, b, lh.t);
}

template <typename T> [[nodiscard]]
auto make_lerp_helper(float frame) -> lerp_helper<T> {
	lerp_helper<T> lh;
	lh.index.a = static_cast<T>(std::floor(frame));
	lh.index.b = static_cast<T>(std::ceil(frame));
	lh.t = frame - lh.index.a;
	return lh;
}

template <typename REP> [[nodiscard]]
auto make_lod(mipmap::lod_index index, mipmap::channel_count channel_count, mipmap::frame_count frame_count, mipmap::detail detail) -> mipmap::lod<REP> {
	mipmap::lod<REP> lod;
	lod.index    = index;
	lod.bin_size = {int(std::pow(detail.value, index.value))};
	lod.data     = mipmap::lod<REP>::data_t{static_cast<size_t>(channel_count.value), mipmap::lod<REP>::channel_t(frame_count.value)};
	return lod;
}

template <typename REP> [[nodiscard]]
auto read(const mipmap::lod<REP>& lod, mipmap::channel_index channel, size_t lod_frame) -> mipmap::frame<REP> {
	if (is_empty(lod.valid_region)) {
		return {};
	}
	lod_frame = std::min(lod.data[0].size() - 1, lod_frame);
	if (lod_frame < lod.valid_region.beg || lod_frame >= lod.valid_region.end) {
		return {};
	}
	return lod.data[channel.value][lod_frame];
}

template <typename REP> [[nodiscard]]
auto read(const mipmap::body<REP>& body, mipmap::channel_index channel, size_t frame) -> REP {
	if (is_empty(body.lod0.valid_region)) {
		return VALUE_SILENT<REP>();
	}
	frame = std::min(body.lod0.data[0].size() - 1, frame);
	if (frame < body.lod0.valid_region.beg || frame >= body.lod0.valid_region.end) {
		return VALUE_SILENT<REP>();
	}
	return body.lod0.data[channel.value][frame];
}

template <typename REP> [[nodiscard]]
auto read(const mipmap::body<REP>& body, mipmap::channel_index channel, float frame) -> REP {
	const auto lerp_frame = make_lerp_helper<size_t>(frame);
	const auto a_value    = read(body, channel, lerp_frame.index.a);
	const auto b_value    = read(body, channel, lerp_frame.index.b);
	return lerp<REP>(lerp_frame, a_value, b_value);
}

} // detail_

template <typename REP> [[nodiscard]]
auto as_float(REP value, mipmap::max_source_clip max_clip) -> float {
	const auto limit = 1.0f + max_clip.value;
	auto fvalue = float(value);
	fvalue /= VALUE_SILENT<REP>();
	fvalue -= 1.0f;
	fvalue *= limit;
	return fvalue;
}

// detail:
//	lower is better quality, but uses more memory
//	detail=0 is a regular mipmap, i.e. each level is half the size of the previous one
//	detail=1 each level is 1/3 the size of the previous one
//	detail=2 1/4, detail=3 1/5, etc. 
//	level zero is always the original sample size
//
// max_source_clip:
//	If the source data exceeds the range -1.0f..1.0f, set this to the max amount by
//	which it exceeds. For example if a frame of data hits -1.543f, this should be set
//	to 0.543f. This will prevent the mipmap values from being clipped to the REP range.
//	If you're dynamically writing to the mipmap from the audio thread or something then
//	you probably won't know ahead of time what that value will be, so you can just leave
//	this at 0.0f and live with the clipping.
template <typename REP = uint8_t> [[nodiscard]]
auto make(mipmap::channel_count channel_count, mipmap::frame_count frame_count, mipmap::detail detail, mipmap::max_source_clip max_source_clip) -> mipmap::body<REP> {
	mipmap::body<REP> body;
	body.channel_count   = channel_count;
	body.frame_count     = frame_count;
	body.detail          = {REP(detail.value + 2)};
	body.max_source_clip = max_source_clip;
	body.lod0.data.resize(channel_count.value);
	for (auto& c : body.lod0.data) {
		c.resize(frame_count.value, VALUE_SILENT<REP>());
	}
	auto size  = frame_count.value / body.detail.value;
	auto index = lod_index{1};
	while (size > 0) {
		body.lods.push_back(detail_::make_lod<REP>(index, channel_count, {size}, body.detail));
		index.value++;
		size /= body.detail.value;
	}
	return body;
}

template <typename REP> [[nodiscard]]
auto bin_size_to_lod(const mipmap::body<REP>& body, float bin_size) -> float {
	if (bin_size <= 1) {
		return 0.0;
	}
	return float(std::log(bin_size) / std::log(body.detail.value));
}

template <typename REP>
auto clear(mipmap::body<REP>* body) -> void {
	body->lod0.valid_region = {};
	for (auto& lod : body->lods) {
		lod.valid_region = {};
	}
}

template <typename REP = uint8_t> [[nodiscard]]
auto encode(mipmap::max_source_clip max_source_clip, float value) -> REP {
	const auto limit = 1.0f + max_source_clip.value;
	value  = std::clamp(value, -limit, limit);
	value /= limit;
	value += 1;
	value *= VALUE_SILENT<REP>();
	return static_cast<REP>(value);
}

template <typename REP = uint8_t> [[nodiscard]]
auto encode(float value) -> REP {
	return encode({}, value);
}

template <typename REP> [[nodiscard]]
auto encode(const mipmap::body<REP>& body, float value) -> REP {
	return encode(body.max_source_clip, value);
}

template <typename REP> [[nodiscard]]
auto lerp(mipmap::frame<REP> a, mipmap::frame<REP> b, float t) -> mipmap::frame<REP> {
	const auto min = detail_::lerp<REP>(a.min.value, b.min.value, t);
	const auto max = detail_::lerp<REP>(a.max.value, b.max.value, t);
	return { min, max };
}

template <typename REP> [[nodiscard]]
auto lod_count(const mipmap::body<REP>& body) -> size_t {
	return body.lods.size() + 1;
}

// Interpolate between two frames of the same LOD
template <typename REP> [[nodiscard]]
auto read(const mipmap::body<REP>& body, mipmap::lod_index lod_index, mipmap::channel_index channel, float frame) -> mipmap::frame<REP> {
	assert(channel.value < body.channel_count.value);
	if (lod_index.value == 0) {
		const auto value = detail_::read(body, channel, frame);
		return { value, value };
	}
	lod_index.value = std::min(lod_index.value, body.lods.size());
	const auto& lod = body.lods[lod_index.value - 1];
	frame /= lod.bin_size.value;
	const auto lerp_frame = detail_::make_lerp_helper<size_t>(frame);
	const auto a_value    = detail_::read(lod, channel, lerp_frame.index.a);
	const auto b_value    = detail_::read(lod, channel, lerp_frame.index.b);
	const auto min        = mipmap::min<REP>{detail_::lerp<REP>(lerp_frame, a_value.min.value, b_value.min.value)};
	const auto max        = mipmap::max<REP>{detail_::lerp<REP>(lerp_frame, a_value.max.value, b_value.max.value)};
	return { min, max };
}

// Interpolate between two LODs and two frames
template <typename REP> [[nodiscard]]
auto read(const mipmap::body<REP>& body, float lod, mipmap::channel_index channel, float frame) -> mipmap::frame<REP> {
	assert(channel.value < body.channel_count.value);
	assert(lod >= 0);
	const auto lerp_lod = detail_::make_lerp_helper<uint16_t>(lod);
	const auto a_value  = read(body, mipmap::lod_index{lerp_lod.index.a}, channel, frame);
	const auto b_value  = read(body, mipmap::lod_index{lerp_lod.index.b}, channel, frame);
	const auto min      = mipmap::min<REP>{detail_::lerp<REP>(lerp_lod, a_value.min.value, b_value.min.value)};
	const auto max      = mipmap::max<REP>{detail_::lerp<REP>(lerp_lod, a_value.max.value, b_value.max.value)};
	return { min, max };
}

// No interpolation.
// lod_frame is the LOD-local frame, i.e. in a 100-frame sample
// with detail=0 so each LOD is half the size of the previous,
// the lod_frame parameter would range from
// 0-99 for LOD 0,
// 0-49 for LOD 1,
// 0-24 for LOD 2, etc.
template <typename REP> [[nodiscard]]
auto read(const mipmap::body<REP>& body, mipmap::lod_index lod_index, mipmap::channel_index channel, size_t lod_frame) -> mipmap::frame<REP> {
	assert(channel.value < body.channel_count.value);
	if (lod_index.value == 0) {
		const auto value = detail_::read(body, channel, lod_frame);
		return { value, value };
	}
	assert(lod_index.value <= body.lods.size());
	const auto& lod = body.lods[lod_index.value - 1];
	return detail_::read(lod, channel, lod_frame);
}

// Writes level zero data. Mipmap data for the other levels won't be generated until update() is called
template <typename REP>
auto write(mipmap::body<REP>* body, mipmap::channel_index channel, size_t frame, float value) -> void {
	body->lod0.data[channel.value][frame] = encode(body->max_source_clip, value);
}

// Write level zero frame data beginning at frame_begin, using a custom writer function
// The writer needs to encode the frames to the range VALUE_MIN<REP>..VALUE_MAX<REP> itself
template <typename REP, typename WriterFn>
auto write(mipmap::body<REP>* body, mipmap::channel_index channel, size_t frame_begin, WriterFn writer) -> void {
	writer(&body->lod0.data[channel.value][frame_begin]);
}

// Generates mipmap data for the specified (top-level) region
// This does both reading and writing of frames within the
// region at all mipmap levels
template <typename REP>
auto update(mipmap::body<REP>* body, mipmap::region region) -> void {
	assert(region.end > region.beg);
	assert(region.end <= body->lod0.data[0].size());
	if (region.beg < body->lod0.valid_region.beg) body->lod0.valid_region.beg = region.beg;
	if (region.end > body->lod0.valid_region.end) body->lod0.valid_region.end = region.end;
	for (auto& lod : body->lods) {
		region.beg /= body->detail.value;
		region.end /= body->detail.value;
		detail_::generate(*body, &lod, body->detail, body->channel_count, region);
	}
}

} // mipmap
} // snd
