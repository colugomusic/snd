#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPFilters.h>
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Allpass
{
public:

	struct BQAP
	{
		float c0 = 0.0f;
		float c1 = 0.0f;
		float c2 = 0.0f;
	};

private:

	BQAP bqap_;
	const BQAP* data_;

	float fbk_val_0_ = 0.0f;
	float fbk_val_1_ = 0.0f;
	float fbk_val_2_ = 0.0f;
	float fbk_val_3_ = 0.0f;

public:

	Filter_2Pole_Allpass(const BQAP* data = nullptr);

	ml::DSPVector operator()(const ml::DSPVector& in);

	void copy(const Filter_2Pole_Allpass& rhs);

	void set(const BQAP& bqap);
	void set_external_data(const BQAP* data);

	static void calculate(int sr, float freq, float res, BQAP* bqap);
};

}}}