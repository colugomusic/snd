#include "audio/saturator/moron_saturator.h"
#include "convert.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace saturator {

float MoronSaturator::operator()(float in) const
{
	auto s_ = (in * limit_) / c_;
	auto wet = (s_ / (std::sqrt(1.0f + (s_ * s_)))) / limit_;

	return snd::lerp(in, wet, mix_);
}

void MoronSaturator::set(float c, float limit, float mix)
{
	c_ = c;
	limit_ = limit;
	mix_ = mix;
}

void MoronSaturator::calculate(float drive, float ceiling_db, float* c, float* limit, float* mix)
{
	auto softness = 0.5f - (drive / 2.0f);
	auto ceiling_af = convert::dB2AF(ceiling_db);

	*c = std::sqrt(std::max(0.001f, std::pow(softness, 4.0f)) * 16.0f);

	auto n = 2.0f * (1.0f - std::max(0.5f, ceiling_af));

	*mix = std::min(1.0f, (2.0f * drive) + n);

	*limit = 1.0f / ceiling_af;
}

}}}