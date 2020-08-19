#include "audio/saturator/moron_saturator_mono.h"

namespace snd {
namespace audio {
namespace saturator {

float MoronSaturator_Mono::operator()(float in)
{
	return saturator_(in);
}

void MoronSaturator_Mono::set_drive(float drive, bool recalc)
{
	drive_ = drive;

	if (recalc)	recalculate();
}

void MoronSaturator_Mono::recalculate()
{
	float c, limit, drv_plus_1;

	MoronSaturator::calculate(drive_, &c, &limit, &drv_plus_1);

	saturator_.set(c, limit, drv_plus_1);
}

}}}