#include "audio/ringmod/ring_modulator.h"
#include "audio/clipping.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace ringmod {

ml::DSPVector RingModulator::operator()(const ml::DSPVector& dry)
{
	auto wet = ((ml::sin(phase_()) + 1.0f) * 0.5f) * dry;

	return ml::lerp(dry, wet, ml::DSPVector(amount_));
}

void RingModulator::reset(float phase)
{
	phase_.reset(phase);
}

void RingModulator::set_amount(float amount)
{
	amount_ = amount;
}

void RingModulator::set_inc(double inc)
{
	phase_.set_inc(inc);
}

double RingModulator::calculate_inc(int sr, float freq)
{
	return clipping::hard_clip((1.0 / sr) * freq, 0.5);
}

}}}