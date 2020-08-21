#include "audio/ringmod/ring_modulator_stereo.h"

namespace snd {
namespace audio {
namespace ringmod {

float RingModulator_Stereo::process_left(float in)
{
	return ringmod_[0](in);
}

float RingModulator_Stereo::process_right(float in)
{
	return ringmod_[1](in);
}

void RingModulator_Stereo::reset(float phase)
{
	ringmod_[0].reset(phase);
	ringmod_[1].reset(phase);
}

void RingModulator_Stereo::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

void RingModulator_Stereo::set_freq(float freq, bool recalc)
{
	freq_ = freq;

	if (recalc)	recalculate();
}

void RingModulator_Stereo::set_amount(float amount)
{
	ringmod_[0].set_amount(amount);
	ringmod_[1].set_amount(amount);
}

void RingModulator_Stereo::recalculate()
{
	auto inc = RingModulator::calculate_inc(sr_, freq_);

	ringmod_[0].set_inc(inc);
	ringmod_[1].set_inc(inc);
}

}}}