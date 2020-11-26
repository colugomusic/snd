#pragma once

#include <atomic>
#include <cstdint>
#include <cmath>

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace transport {

template <size_t ROWS>
struct DSPVectorArrayFramePosition;

// Represents a frame position
class FramePosition
{
public:

	template <size_t ROWS>
	using DSPVectorArray = DSPVectorArrayFramePosition<ROWS>;
	using DSPVector = DSPVectorArray<1>;

	FramePosition() = default;
	FramePosition(FramePosition&& rhs) = default;
	FramePosition(const FramePosition& rhs) = default;
	FramePosition(std::int32_t pos);
	FramePosition(std::int32_t pos, float fract);
	FramePosition(float pos);

	FramePosition& operator=(FramePosition&& rhs) = default;
	FramePosition& operator=(const FramePosition& rhs) = default;
	FramePosition& operator=(std::int32_t pos);

	FramePosition& operator+=(const FramePosition& rhs);
	FramePosition& operator-=(const FramePosition& rhs);

	bool operator<(const FramePosition& rhs) const;
	bool operator<=(const FramePosition& rhs) const;
	bool operator>(const FramePosition& rhs) const;
	bool operator>=(const FramePosition& rhs) const;

	operator float() const;
	operator double() const;
	operator std::int32_t() const;

	std::int32_t get_pos() const { return pos_; }
	float get_fract() const { return fract_; }

private:

	std::int32_t pos_ = 0;
	float fract_ = 0.0f;
};

extern FramePosition operator+(const FramePosition& a, const FramePosition& b);
extern FramePosition operator-(const FramePosition& a, const FramePosition& b);

// Vectorized frame position
template <size_t ROWS>
struct DSPVectorArrayFramePosition
{
	ml::DSPVectorArrayInt<ROWS> pos;
	ml::DSPVectorArray<ROWS> fract;

	DSPVectorArrayFramePosition() = default;
	DSPVectorArrayFramePosition(std::int32_t v) : pos(v) {}
	DSPVectorArrayFramePosition(const ml::DSPVectorArray<ROWS>& rhs)
	{
		pos = ml::truncateFloatToInt(rhs);
		fract = ml::fractionalPart(rhs);
	}

	FramePosition operator[](int index) const
	{
		return FramePosition(pos.getConstBufferInt()[index], fract.getConstBuffer()[index]);
	}

	template <size_t ROW = 0>
	void set(int index, const FramePosition& fp)
	{
		pos.row(ROW)[index] = fp.get_pos();
		fract.row(ROW)[index] = fp.get_fract();
	}

	DSPVectorArrayFramePosition<1> constRow(int r) const
	{
		DSPVectorArrayFramePosition<1> out;

		out.pos = pos.constRow(r);
		out.fract = fract.constRow(r);

		return out;
	}

	DSPVectorArrayFramePosition<ROWS> operator+(std::int32_t v) const
	{
		DSPVectorArrayFramePosition<ROWS> out;

		out.pos = pos + ml::DSPVectorInt(v);
		out.fract = fract;

		return out;
	}
};

using DSPVectorFramePosition = DSPVectorArrayFramePosition<1>;

template <size_t ROWS>
DSPVectorArrayFramePosition<ROWS> operator+(const DSPVectorArrayFramePosition<ROWS>& a, const DSPVectorArrayFramePosition<ROWS>& b)
{
	DSPVectorArrayFramePosition<ROWS> out;

	out.pos = a.pos + b.pos;
	out.fract = a.fract + b.fract;

	const auto wrap = ml::greaterThanOrEqual(out.fract, ml::DSPVectorArray<ROWS>(1.0f));

	out.pos = ml::select(out.pos + ml::DSPVectorArrayInt<ROWS>(1), out.pos, wrap);
	out.fract = ml::select(out.fract - 1.0f, out.fract, wrap);

	return out;
}

template <size_t ROWS>
DSPVectorArrayFramePosition<ROWS> operator+(const DSPVectorArrayFramePosition<ROWS>& a, const FramePosition& b)
{
	DSPVectorArrayFramePosition<ROWS> out;

	out.pos = a.pos + ml::DSPVectorArrayInt<ROWS>(b.get_pos());
	out.fract = a.fract + ml::DSPVectorArray<ROWS>(b.get_fract());

	const auto wrap = ml::greaterThanOrEqual(out.fract, ml::DSPVectorArray<ROWS>(1.0f));

	out.pos = ml::select(out.pos + ml::DSPVectorArrayInt<ROWS>(1), out.pos, wrap);
	out.fract = ml::select(out.fract - 1.0f, out.fract, wrap);

	return out;
}

template <size_t ROWS>
DSPVectorArrayFramePosition<ROWS> operator-(const DSPVectorArrayFramePosition<ROWS>& a, const DSPVectorArrayFramePosition<ROWS>& b)
{
	DSPVectorArrayFramePosition<ROWS> out;

	out.pos = a.pos - b.pos;
	out.fract = a.fract - b.fract;

	const auto wrap = ml::lessThan(out.fract, ml::DSPVectorArray<ROWS>(0.0f));

	out.pos = ml::select(out.pos - ml::DSPVectorArrayInt<ROWS>(1), out.pos, wrap);
	out.fract = ml::select(out.fract + 1.0f, out.fract, wrap);

	return out;
}

template <size_t ROWS>
DSPVectorArrayFramePosition<ROWS> operator-(const DSPVectorArrayFramePosition<ROWS>& a, std::int32_t b)
{
	DSPVectorArrayFramePosition<ROWS> out;

	out.pos = a.pos - ml::DSPVectorArrayInt<ROWS>(b);

	return out;
}

}}