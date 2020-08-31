#pragma once

#include "moron_saturator.h"

namespace snd {
namespace audio {
namespace saturator {

class MoronSaturator_Mono
{
	float drive_ = 0.0f;

	MoronSaturator saturator_;

public:

	ml::DSPVector operator()(const ml::DSPVector& in);
	void set_drive(float drive, bool recalculate = true);
	void recalculate();
};

}}}