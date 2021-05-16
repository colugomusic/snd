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
	./include/snd/convert.h
	./include/snd/dup_filter.h
	./include/snd/ease.h
	./include/snd/interpolation.h
	./include/snd/misc.h
	./include/snd/mlext.h
	./include/snd/types.h
	./include/snd/audio/clipping.h
	./include/snd/audio/level_meter.h
	./include/snd/audio/filter/1-pole.h
	./include/snd/audio/filter/2-pole.h
	./include/snd/audio/filter/2-pole_allpass.h
	./include/snd/audio/filter/2-pole_allpass_array.h
	./include/snd/audio/filter/serial/2-pole.h
	./include/snd/audio/filter/serial/2-pole_mono.h
	./include/snd/audio/ringmod/ring_modulator.h
	./include/snd/audio/saturator/moron_saturator.h
	./include/snd/clock/clock_divider.h
	./include/snd/control/smoother.h
	./include/snd/curve/curve.hpp
	./include/snd/curve/schlick.hpp
	./include/snd/storage/circular_buffer.h
	./include/snd/storage/frame_data.h
	./include/snd/storage/interleaving.h
	./include/snd/threads/audio_object.h
	./include/snd/threads/audio_object_manager.h
	./include/snd/threads/garbage_disposal.h
	./include/snd/threads/msg/audio_gui_comms.h
	./include/snd/threads/msg/msg_channel.h
	./include/snd/threads/msg/msg_comms.h
	./include/snd/transport/frame_position.h
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
