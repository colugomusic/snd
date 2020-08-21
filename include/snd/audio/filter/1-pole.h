#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_1Pole
{
	float g_ = 0.0f;
	float zdfbk_val_ = 0.0f;
	float lp_ = 0.0f;
	float hp_ = 0.0f;

public:

	float lp() const { return lp_; }
	float hp() const { return hp_; }
	void process_frame(float in);
	void set_g(float g);

	static float calculate_g(int sr, float freq);
};

}}}