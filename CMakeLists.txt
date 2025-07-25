cmake_minimum_required(VERSION 3.23)
project(snd)
add_library(snd INTERFACE)
add_library(snd::snd ALIAS snd)
target_sources(snd INTERFACE
	FILE_SET HEADERS
	BASE_DIRS include
	FILES
		include/snd/const_math.hpp
		include/snd/convert.hpp
		include/snd/diff_detector.hpp
		include/snd/dup_filter.hpp
		include/snd/ease.hpp
		include/snd/flags.hpp
		include/snd/interpolation.hpp
		include/snd/misc.hpp
		include/snd/mlext.hpp
		include/snd/ramp_gen.hpp
		include/snd/resampler.hpp
		include/snd/simplex_noise.hpp
		include/snd/threading.hpp
		include/snd/types.hpp
		include/snd/audio/autocorrelation.hpp
		include/snd/audio/clipping.hpp
		include/snd/audio/dc_bias.hpp
		include/snd/audio/delay.hpp
		include/snd/audio/env_follower.hpp
		include/snd/audio/feedback_delay.hpp
		include/snd/audio/fudge.hpp
		include/snd/audio/glottis.hpp
		include/snd/audio/level_meter.hpp
		include/snd/audio/normalizer.hpp
		include/snd/audio/oscillators.hpp
		include/snd/audio/player.hpp
		include/snd/audio/scale.hpp
		include/snd/audio/wavebender.hpp
		include/snd/audio/filter/1-pole.hpp
		include/snd/audio/filter/2-pole.hpp
		include/snd/audio/filter/2-pole_allpass.hpp
		include/snd/audio/filter/2-pole_allpass_array.hpp
		include/snd/audio/filter/tract.hpp
		include/snd/audio/filter/serial/2-pole.hpp
		include/snd/audio/filter/serial/2-pole_mono.hpp
		include/snd/clock/clock_divider.hpp
		include/snd/control/smoother.hpp
		include/snd/curve/curve.hpp
		include/snd/curve/schlick.hpp
		include/snd/curve/tilt.hpp
		include/snd/transport/frame_position.hpp
)
target_compile_features(snd INTERFACE cxx_std_20)
install(TARGETS snd FILE_SET HEADERS)