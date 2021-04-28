project(snd)

include(CPM.cmake)

# Header-only libraries #########################################
CPMAddPackage("gh:cameron314/readerwriterqueue@1.0.4")
CPMAddPackage("gh:mackron/dr_libs#master")
CPMAddPackage("gh:mackron/miniaudio#master")
CPMAddPackage("gh:madronalabs/madronalib#master")
CPMAddPackage("gh:sgorsten/linalg#master")
CPMAddPackage("gh:nemtrif/utfcpp#master")

list(APPEND include_dirs
    ${dr_libs_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
    ${madronalib_SOURCE_DIR}/source
    ${linalg_SOURCE_DIR}
    ${readerwriterqueue_SOURCE_DIR}
    ${utfcpp_SOURCE_DIR}/source
)
#xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

# WavPack #######################################################
set(WAVPACK_BUILD_COOLEDIT_PLUGIN OFF CACHE BOOL "Build CoolEdit plugin")
CPMAddPackage("gh:colugomusic/wavpack#master")
list(APPEND include_dirs ${wavpack_SOURCE_DIR}/include)
list(APPEND link_libs wavpack)
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
	./include/snd/storage/audio_file_format.h
	./include/snd/storage/audio_file_reader.h
	./include/snd/storage/audio_file_writer.h
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

list(APPEND SRC
	./src/audio/filter/serial/2-pole.cpp
	./src/audio/filter/serial/2-pole_mono.cpp
	./src/clock/clock_divider.cpp
	./src/control/smoother.cpp
	./src/threads/msg/audio_gui_comms.cpp
	./src/threads/msg/msg_channel.cpp
	./src/storage/audio_file_reader.cpp
	./src/storage/audio_file_writer.cpp
	./src/storage/dr_libs/dr_libs_impl.cpp
	./src/storage/dr_libs/dr_libs_utils.h
	./src/storage/dr_libs/dr_libs_utils.cpp
	./src/storage/miniaudio/snd_miniaudio.h
	./src/storage/miniaudio/snd_miniaudio.cpp
	./src/storage/wavpack/wavpack_reader.h
	./src/storage/wavpack/wavpack_reader.cpp
	./src/storage/wavpack/wavpack_file_reader.h
	./src/storage/wavpack/wavpack_file_reader.cpp
	./src/storage/wavpack/wavpack_memory_reader.h
	./src/storage/wavpack/wavpack_memory_reader.cpp
	./src/transport/frame_position.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/include/snd PREFIX public_headers FILES ${PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src PREFIX src FILES ${SRC})

add_library(${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
)

target_include_directories(${PROJECT_NAME} PRIVATE
	./include
	./include/snd
	${include_dirs}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${link_libs})
