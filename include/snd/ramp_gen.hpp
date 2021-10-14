#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace ml {

class RampGen
{
public:
	RampGen(int p = kFloatsPerDSPVector) : mCounter(p), mPeriod(p) {}
	~RampGen() {}

	inline void setPeriod(int p) { mPeriod = p; }

	inline DSPVectorInt operator()()
	{
		DSPVectorInt vy;
		for (int i = 0; i < kFloatsPerDSPVector; ++i)
		{
			float fy = 0;
			if (++mCounter >= mPeriod)
			{
				mCounter = 0;
				fy = 1;
			}
			vy[i] = mCounter;
		}
		return vy;
	}
	int mCounter;
	int mPeriod;
};

}