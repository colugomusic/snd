#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "snd/ease.hpp"

TEST_CASE("easing functions") {
	REQUIRE(snd::ease(0.0, snd::easing::curve::quadratic, snd::easing::mode::in_out, 0.0) == 0);
}
