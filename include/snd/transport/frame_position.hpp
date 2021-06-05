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

// Represents a fractional frame position with very high precision
// Consists of pos (int32) and fract (float) parts
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
	FramePosition& operator+=(std::int32_t rhs);
	FramePosition& operator-=(std::int32_t rhs);
	FramePosition& operator+=(float rhs);
	FramePosition& operator-=(float rhs);

	bool operator<(const FramePosition& rhs) const;
	bool operator<=(const FramePosition& rhs) const;
	bool operator>(const FramePosition& rhs) const;
	bool operator>=(const FramePosition& rhs) const;
	bool operator<(std::int32_t rhs) const;
	bool operator<=(std::int32_t rhs) const;
	bool operator>(std::int32_t rhs) const;
	bool operator>=(std::int32_t rhs) const;

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
extern FramePosition operator+(const FramePosition& a, std::int32_t b);
extern FramePosition operator-(const FramePosition& a, std::int32_t b);

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

	std::array<std::array<double, kFloatsPerDSPVector>, ROWS> as_doubles() const
	{
		std::array<std::array<double, kFloatsPerDSPVector>, ROWS> out;

		for (auto row = 0; row < ROWS; row++)
		{
			for (auto i = 0; i < kFloatsPerDSPVector; i++)
			{
				out[row][i] = FramePosition(pos.constRow(row)[i], fract.constRow(row)[i]);
			}
		}

		return out;
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
	out.fract = a.fract;

	return out;
}

template <size_t ROWS>
DSPVectorArrayFramePosition<ROWS> operator-(std::int32_t a, const DSPVectorArrayFramePosition<ROWS>& b)
{
	return DSPVectorArrayFramePosition<ROWS>(a) - b;
}

inline bool FramePosition::operator<(const FramePosition& rhs) const
{
	if (pos_ < rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ < rhs.fract_;

	return false;
}

inline bool FramePosition::operator<=(const FramePosition& rhs) const
{
	if (pos_ < rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ <= rhs.fract_;

	return false;
}

inline bool FramePosition::operator>(const FramePosition& rhs) const
{
	if (pos_ > rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ > rhs.fract_;

	return false;
}

inline bool FramePosition::operator>=(const FramePosition& rhs) const
{
	if (pos_ > rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ >= rhs.fract_;

	return false;
}

inline bool FramePosition::operator<(std::int32_t rhs) const
{
	return pos_ < rhs;
}

inline bool FramePosition::operator<=(std::int32_t rhs) const
{
	if (pos_ < rhs) return true;
	if (pos_ == rhs) return fract_ == 0.0f;

	return false;
}

inline bool FramePosition::operator>(std::int32_t rhs) const
{
	if (pos_ > rhs) return true;
	if (pos_ == rhs) return fract_ > 0.0f;

	return false;
}

inline bool FramePosition::operator>=(std::int32_t rhs) const
{
	if (pos_ > rhs) return true;
	if (pos_ == rhs) return fract_ >= 0.0f;

	return false;
}

inline FramePosition::FramePosition(std::int32_t pos)
	: pos_(pos)
	, fract_(0.0f)
{
}

inline FramePosition::FramePosition(std::int32_t pos, float fract)
	: pos_(pos)
	, fract_(fract)
{
}

inline FramePosition::FramePosition(float pos)
{
	float fractional_part;
	float int_part;

	fractional_part = std::modf(pos, &int_part);

	pos_ = std::int32_t(int_part);
	fract_ = fractional_part;
}

inline FramePosition& FramePosition::operator=(std::int32_t pos)
{
	pos_ = pos;
	fract_ = 0.0f;

	return *this;
}

inline FramePosition& FramePosition::operator+=(const FramePosition& rhs)
{
	pos_ += rhs.pos_;
	fract_ += rhs.fract_;

	if (fract_ >= 1.0f)
	{
		fract_ -= 1.0f;
		pos_++;
	}

	return *this;
}

inline FramePosition& FramePosition::operator-=(const FramePosition& rhs)
{
	pos_ -= rhs.pos_;
	fract_ -= rhs.fract_;

	if (fract_ < 0.0f)
	{
		fract_ += 1.0f;
		pos_--;
	}

	return *this;
}

inline FramePosition& FramePosition::operator+=(std::int32_t rhs)
{
	pos_ += rhs;

	return *this;
}

inline FramePosition& FramePosition::operator-=(std::int32_t rhs)
{
	pos_ -= rhs;

	return *this;
}

inline FramePosition& FramePosition::operator+=(float rhs)
{
	if (rhs >= 1.0f) return operator+=(FramePosition(rhs));

	fract_ += rhs;

	if (fract_ >= 1.0f)
	{
		fract_ -= 1.0f;
		pos_++;
	}

	return *this;
}

inline FramePosition& FramePosition::operator-=(float rhs)
{
	if (rhs >= 1.0f) return operator-=(FramePosition(rhs));

	fract_ -= rhs;

	if (fract_ < 0.0f)
	{
		fract_ += 1.0f;
		pos_--;
	}

	return *this;
}

inline FramePosition::operator float() const
{
	return float(pos_) + fract_;
}

inline FramePosition::operator double() const
{
	return double(pos_) + fract_;
}

inline FramePosition::operator std::int32_t() const
{
	return pos_;
}

inline FramePosition operator+(const FramePosition& a, const FramePosition& b)
{
	FramePosition out(a);

	out += b;

	return out;
}

inline FramePosition operator-(const FramePosition& a, const FramePosition& b)
{
	FramePosition out(a);

	out -= b;

	return out;
}

inline FramePosition operator+(const FramePosition& a, std::int32_t b)
{
	FramePosition out(a);

	out += b;

	return out;
}

inline FramePosition operator-(const FramePosition& a, std::int32_t b)
{
	FramePosition out(a);

	out -= b;

	return out;
}

}}
