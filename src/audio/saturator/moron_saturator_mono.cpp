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

void MoronSaturator_Mono::set_ceiling_db(float ceiling_db, bool recalc)
{
	ceiling_db_ = ceiling_db;

	if (recalc)	recalculate();
}

void MoronSaturator_Mono::recalculate()
{
	float c, limit, mix;

	MoronSaturator::calculate(drive_, ceiling_db_, &c, &limit, &mix);

	saturator_.set(c, limit, mix);
}

}}}