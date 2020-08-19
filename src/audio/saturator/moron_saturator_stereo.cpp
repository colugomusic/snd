#include "audio/saturator/moron_saturator_stereo.h"

namespace snd {
namespace audio {
namespace saturator {

float MoronSaturator_Stereo::process_left(float in)
{
	return saturators_[0](in);
}

float MoronSaturator_Stereo::process_right(float in)
{
	return saturators_[1](in);
}

void MoronSaturator_Stereo::set_drive(float drive, bool recalc)
{
	drive_ = drive;

	if (recalc)	recalculate();
}

void MoronSaturator_Stereo::recalculate()
{
	float c, limit, mix, drv_plus_1;

	MoronSaturator::calculate(drive_, &c, &limit, &mix, &drv_plus_1);

	saturators_[0].set(c, limit, mix, drv_plus_1);
	saturators_[1].set(c, limit, mix, drv_plus_1);
}

}}}