#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "snd/ease.hpp"
#include "snd/curved-line-constructor.hpp"

TEST_CASE("easing functions") {
	REQUIRE(snd::ease(0.0, snd::easing::curve::quadratic, snd::easing::mode::in_out, 0.0) == 0);
}

TEST_CASE("curved-line-constructor compile") {
	const auto fn_get_value = [](float) -> float { return 0.0f; };
	const auto resolution   = snd::XY<float>{.x = 100.0f, .y = 100.0f};
	auto tmp_list = std::list<snd::curved_line_constructor::builder_point>{};
	auto points   = std::vector<snd::XY<float>>{};
	snd::curved_line_constructor::construct(fn_get_value, 0.0f, 1.0f, resolution, tmp_list, std::back_inserter(points));
}
