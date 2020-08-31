#include "audio/saturator/moron_saturator.h"
#include "convert.h"
#include "ease.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace saturator {

ml::DSPVector MoronSaturator::operator()(const ml::DSPVector& in) const
{
	auto s_ = (in * limit_) / c_;
	
	return ((s_ / (ml::sqrt(1.0f + (s_ * s_)))) / limit_) * drv_plus_1_;
}

void MoronSaturator::set(float c, float limit, float drv_plus_1)
{
	c_ = c;
	limit_ = limit;
	drv_plus_1_ = drv_plus_1;
}

void MoronSaturator::calculate(float drive, float* c, float* limit, float* drv_plus_1)
{
	auto softness = 0.5f - (drive / 2.0f);
	auto ceiling_af = 1.0f - (ease::quadratic::out(drive) / 2.0f);

	*c = std::sqrt(std::max(0.001f, std::pow(softness, 4.0f)) * 16.0f);

	auto n = 2.0f * (1.0f - std::max(0.5f, ceiling_af));

	*limit = 1.0f / ceiling_af;
	*drv_plus_1 = drive + 1.0f;
}

}}}