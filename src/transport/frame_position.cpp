#include "transport/frame_position.h"

namespace snd {
namespace transport {

bool FramePosition::operator<(const FramePosition& rhs) const
{
	if (pos_ < rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ < rhs.fract_;

	return false;
}

bool FramePosition::operator<=(const FramePosition& rhs) const
{
	if (pos_ < rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ <= rhs.fract_;

	return false;
}

bool FramePosition::operator>(const FramePosition& rhs) const
{
	if (pos_ > rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ > rhs.fract_;

	return false;
}

bool FramePosition::operator>=(const FramePosition& rhs) const
{
	if (pos_ > rhs.pos_) return true;
	if (pos_ == rhs.pos_) return fract_ >= rhs.fract_;

	return false;
}

bool FramePosition::operator<(std::int32_t rhs) const
{
	return pos_ < rhs;
}

bool FramePosition::operator<=(std::int32_t rhs) const
{
	if (pos_ < rhs) return true;
	if (pos_ == rhs) return fract_ == 0.0f;

	return false;
}

bool FramePosition::operator>(std::int32_t rhs) const
{
	if (pos_ > rhs) return true;
	if (pos_ == rhs) return fract_ > 0.0f;

	return false;
}
bool FramePosition::operator>=(std::int32_t rhs) const
{
	if (pos_ > rhs) return true;
	if (pos_ == rhs) return fract_ >= 0.0f;

	return false;
}

FramePosition::FramePosition(std::int32_t pos)
	: pos_(pos)
	, fract_(0.0f)
{
}

FramePosition::FramePosition(std::int32_t pos, float fract)
	: pos_(pos)
	, fract_(fract)
{
}

FramePosition::FramePosition(float pos)
{
	float fractional_part;
	float int_part;

	fractional_part = std::modf(pos, &int_part);

	pos_ = std::int32_t(int_part);
	fract_ = fractional_part;
}

FramePosition& FramePosition::operator=(std::int32_t pos)
{
	pos_ = pos;
	fract_ = 0.0f;

	return *this;
}

FramePosition& FramePosition::operator+=(const FramePosition& rhs)
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

FramePosition& FramePosition::operator-=(const FramePosition& rhs)
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

FramePosition& FramePosition::operator+=(std::int32_t rhs)
{
	pos_ += rhs;

	return *this;
}

FramePosition& FramePosition::operator-=(std::int32_t rhs)
{
	pos_ -= rhs;

	return *this;
}

FramePosition& FramePosition::operator+=(float rhs)
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

FramePosition& FramePosition::operator-=(float rhs)
{
	if (rhs >= 1.0f) return operator-=(FramePosition(rhs));

	fract_ -= fract_;

	if (fract_ < 0.0f)
	{
		fract_ += 1.0f;
		pos_--;
	}

	return *this;
}

FramePosition::operator float() const
{
	return float(pos_) + fract_;
}

FramePosition::operator double() const
{
	return double(pos_) + fract_;
}

FramePosition::operator std::int32_t() const
{
	return pos_;
}

FramePosition operator+(const FramePosition& a, const FramePosition& b)
{
	FramePosition out(a);

	out += b;

	return out;
}

FramePosition operator-(const FramePosition& a, const FramePosition& b)
{
	FramePosition out(a);

	out -= b;

	return out;
}

FramePosition operator+(const FramePosition& a, std::int32_t b)
{
	FramePosition out(a);

	out += b;

	return out;
}

FramePosition operator-(const FramePosition& a, std::int32_t b)
{
	FramePosition out(a);

	out -= b;

	return out;
}

}}