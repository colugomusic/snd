project(snd)

include(CPM.cmake)

# Header-only libraries #########################################
CPMAddPackage("gh:cameron314/readerwriterqueue@1.0.4")
CPMAddPackage("gh:madronalabs/madronalib#master")
CPMAddPackage("gh:sgorsten/linalg#master")

list(APPEND include_dirs
    ${madronalib_SOURCE_DIR}/source
    ${linalg_SOURCE_DIR}
    ${readerwriterqueue_SOURCE_DIR}
)
#xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

list(APPEND PUBLIC_HEADERS
	./include/snd/convert.hpp
	./include/snd/dup_filter.hpp
	./include/snd/ease.hpp
	./include/snd/interpolation.hpp
	./include/snd/misc.hpp
	./include/snd/mlext.hpp
	./include/snd/types.hpp
	./include/snd/audio/clipping.hpp
	./include/snd/audio/level_meter.hpp
	./include/snd/audio/filter/1-pole.hpp
	./include/snd/audio/filter/2-pole.hpp
	./include/snd/audio/filter/2-pole_allpass.hpp
	./include/snd/audio/filter/2-pole_allpass_array.hpp
	./include/snd/audio/filter/serial/2-pole.hpp
	./include/snd/audio/filter/serial/2-pole_mono.hpp
	./include/snd/audio/ringmod/ring_modulator.hpp
	./include/snd/audio/saturator/moron_saturator.hpp
	./include/snd/clock/clock_divider.hpp
	./include/snd/control/smoother.hpp
	./include/snd/curve/curve.hpp
	./include/snd/curve/schlick.hpp
	./include/snd/storage/circular_buffer.hpp
	./include/snd/storage/frame_data.hpp
	./include/snd/storage/interleaving.hpp
	./include/snd/threads/audio_object.hpp
	./include/snd/threads/audio_object_manager.hpp
	./include/snd/threads/garbage_disposal.hpp
	./include/snd/threads/msg/audio_gui_comms.hpp
	./include/snd/threads/msg/msg_channel.hpp
	./include/snd/threads/msg/msg_comms.hpp
	./include/snd/transport/frame_position.hpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/include/snd PREFIX public_headers FILES ${PUBLIC_HEADERS})

add_library(${PROJECT_NAME} INTERFACE ${PUBLIC_HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
)

target_include_directories(${PROJECT_NAME} PRIVATE
	./include
	./include/snd
	${include_dirs}
)
