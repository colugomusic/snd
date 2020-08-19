#pragma once

#include "moron_saturator.h"

namespace snd {
namespace audio {
namespace saturator {

class MoronSaturator_Stereo
{
	float drive_ = 0.0f;
	float ceiling_db_ = 0.0f;

	MoronSaturator saturators_[2];

public:

	float process_left(float in);
	float process_right(float in);
	void set_drive(float drive, bool recalculate = true);
	void set_ceiling_db(float ceiling_db, bool recalculate = true);
	void recalculate();
};

}}}