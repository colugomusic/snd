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

void MoronSaturator_Stereo::set_ceiling_db(float ceiling_db, bool recalc)
{
	ceiling_db_ = ceiling_db;

	if (recalc)	recalculate();
}

void MoronSaturator_Stereo::recalculate()
{
	float c, limit, mix;

	MoronSaturator::calculate(drive_, ceiling_db_, &c, &limit, &mix);

	saturators_[0].set(c, limit, mix);
	saturators_[1].set(c, limit, mix);
}

}}}