#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace ml {


// usage:
// if_else { condition_vector } ( branch_1, branch_2 );

struct if_else
{
	ml::DSPVectorInt condition;

	ml::DSPVector operator()(const ml::DSPVector& a, const ml::DSPVector& b)
	{
		return ml::select(a, b, condition);
	}
};


}