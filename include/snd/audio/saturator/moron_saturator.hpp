#pragma once

#include <algorithm>
#include <cmath>

#include "../../ease.hpp"
#include "../../diff_detector.hpp"

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace saturator {

template <int ROWS>
class MoronSaturator
{
	ml::DSPVectorArray<ROWS> c_;
	ml::DSPVectorArray<ROWS> limit_;
	ml::DSPVectorArray<ROWS> drv_plus_1_;

	DiffDetector<float, ROWS> diff_drive_;

	bool needs_recalc(const ml::DSPVectorArray<ROWS>& drive);

public:

	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in) const;
	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in, const ml::DSPVectorArray<ROWS>& drive);

	static void calculate(const ml::DSPVectorArray<ROWS>& drive, ml::DSPVectorArray<ROWS>* c, ml::DSPVectorArray<ROWS>* limit, ml::DSPVectorArray<ROWS>* drv_plus_1);
};

template <int ROWS>
bool MoronSaturator<ROWS>::needs_recalc(const ml::DSPVectorArray<ROWS>& drive)
{
	return diff_drive_(drive);
}

template <int ROWS>
ml::DSPVectorArray<ROWS> MoronSaturator<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in) const
{
	auto s_ = (in * limit_) / c_;

	return ((s_ / (ml::sqrt(1.0f + (s_ * s_)))) / limit_) * drv_plus_1_;
}

template <int ROWS>
ml::DSPVectorArray<ROWS> MoronSaturator<ROWS>::operator()(const ml::DSPVectorArray<ROWS>& in, const ml::DSPVectorArray<ROWS>& drive)
{
	if (needs_recalc(drive)) calculate(drive, &c_, &limit_, &drv_plus_1_);

	return this->operator()(in);
}

template <int ROWS>
void MoronSaturator<ROWS>::calculate(const ml::DSPVectorArray<ROWS>& drive, ml::DSPVectorArray<ROWS>* c, ml::DSPVectorArray<ROWS>* limit, ml::DSPVectorArray<ROWS>* drv_plus_1)
{
	auto drive_adjust = drive * 0.99f;
	auto softness = 0.5f - (drive_adjust / 2.0f);
	auto ceiling_af = 1.0f - (ease::quadratic::out(drive_adjust) / 2.0f);

	*c = ml::sqrt(ml::max(ml::DSPVectorArray<ROWS>(0.001f), ml::pow(ml::DSPVectorArray<ROWS>(softness), ml::DSPVectorArray<ROWS>(4.0f))) * 16.0f);

	auto n = 2.0f * (1.0f - ml::max(ml::DSPVectorArray<ROWS>(0.5f), ceiling_af));

	*limit = 1.0f / ceiling_af;
	*drv_plus_1 = drive_adjust + 1.0f;
}

}}}