#include "audio/ringmod/ring_modulator.h"
#include "audio/clipping.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace ringmod {

float RingModulator::operator()(float dry)
{
	auto wet = ((std::sin(phase_) + 1.0) * 0.5) * dry;

	phase_ += inc_;

	return lerp(dry, float(wet), amount_);
}

void RingModulator::reset(double phase)
{
	phase_ = phase;
}

void RingModulator::set_amount(float amount)
{
	amount_ = amount;
}

void RingModulator::set_inc(double inc)
{
	inc_ = inc;
}

double RingModulator::calculate_inc(float sr, float freq)
{
	return clipping::hard_clip((1.0 / sr) * freq, 0.5);
}

}}}