#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_1Pole
{
	float g_;
	float zdfbk_val_;
	float lp_;
	float hp_;

public:

	Filter_1Pole();

	float lp() const { return lp_; }
	float hp() const { return hp_; }
	void process_frame(float in);
	void set_g(float g);

	static float calculate_g(float sr, float freq);
};

}}}