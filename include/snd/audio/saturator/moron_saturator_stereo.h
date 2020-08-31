#pragma once

#include "moron_saturator.h"

namespace snd {
namespace audio {
namespace saturator {

class MoronSaturator_Stereo
{
	float drive_ = 0.0f;

	MoronSaturator saturators_[2];

public:

	ml::DSPVectorArray<2> operator()(const ml::DSPVectorArray<2>& in);

	void set_drive(float drive, bool recalculate = true);
	void recalculate();
};

}}}