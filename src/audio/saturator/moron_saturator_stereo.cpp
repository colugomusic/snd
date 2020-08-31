#include "audio/saturator/moron_saturator_stereo.h"

namespace snd {
namespace audio {
namespace saturator {

ml::DSPVectorArray<2> MoronSaturator_Stereo::operator()(const ml::DSPVectorArray<2>& in)
{
	return ml::append(saturators_[0](in.constRow(0)), saturators_[1](in.constRow(1)));
}

void MoronSaturator_Stereo::set_drive(float drive, bool recalc)
{
	drive_ = drive;

	if (recalc)	recalculate();
}

void MoronSaturator_Stereo::recalculate()
{
	float c, limit, drv_plus_1;

	MoronSaturator::calculate(drive_, &c, &limit, &drv_plus_1);

	saturators_[0].set(c, limit, drv_plus_1);
	saturators_[1].set(c, limit, drv_plus_1);
}

}}}