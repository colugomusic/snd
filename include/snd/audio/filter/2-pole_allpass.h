#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Allpass
{
public:

	struct BQAP
	{
		double a0 = 0.0f;
		double a1 = 0.0f;
		double a2 = 0.0f;
		double b0 = 0.0f;
		double b1 = 0.0f;
		double b2 = 0.0f;
	};

private:

	BQAP bqap_;

	double zdfbk_val_0_ = 0.0f;
	double zdfbk_val_1_ = 0.0f;
	double zdfbk_val_2_ = 0.0f;
	double zdfbk_val_3_ = 0.0f;

public:

	float operator()(float in);

	void set(const BQAP& bqap);

	static void calculate(int sr, float freq, float res, BQAP* bqap);
};

}}}