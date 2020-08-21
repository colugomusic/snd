#include "audio/ringmod/ring_modulator_mono.h"

namespace snd {
namespace audio {
namespace ringmod {

float RingModulator_Mono::operator()(float in)
{
	return ringmod_(in);
}

void RingModulator_Mono::reset(float phase)
{
	ringmod_.reset(phase);
}

void RingModulator_Mono::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void RingModulator_Mono::set_freq(float freq, bool recalc)
{
	freq_ = freq;

	if (recalc)	recalculate();
}

void RingModulator_Mono::set_amount(float amount)
{
	ringmod_.set_amount(amount);
}

void RingModulator_Mono::recalculate()
{
	ringmod_.set_inc(RingModulator::calculate_inc(sr_, freq_));
}

}}}