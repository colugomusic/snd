project(snd)

include(FetchContent)
set(FETCHCONTENT_QUIET off)

macro(GITHUB_FETCH url)
	get_filename_component(name ${url} NAME_WE)

	set(SETME_MSG "You can optionally set this to your local ${url} root directory. otherwise we will pull it from github")

	if (NOT ${name}_ROOT)
		set(${name}_ROOT ${SETME_MSG} CACHE PATH "${url} root directory")
    elseif(NOT ${name}_ROOT STREQUAL ${SETME_MSG})
        set(${name}_DIR ${${name}_ROOT})
	endif()

	if (NOT ${name}_DIR)
		FetchContent_GetProperties(${name})

        if (NOT ${name}_POPULATED)
            FetchContent_Declare(
                ${name}
                GIT_REPOSITORY https://github.com/${url}
            )
            FetchContent_Populate(${name})
        endif()

        set(${name}_DIR ${${name}_SOURCE_DIR})
	endif()
endmacro()

GITHUB_FETCH(colugomusic/wavpack)
GITHUB_FETCH(madronalabs/madronalib)
GITHUB_FETCH(cameron314/readerwriterqueue)
GITHUB_FETCH(mackron/dr_libs)
GITHUB_FETCH(mackron/miniaudio)
GITHUB_FETCH(nemtrif/utfcpp)
GITHUB_FETCH(sgorsten/linalg)

set(WAVPACK_BUILD_COOLEDIT_PLUGIN OFF CACHE BOOL "Build CoolEdit plugin")
add_subdirectory(${wavpack_DIR} wavpack)

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
	${dr_libs_DIR}
	${linalg_DIR}
	${miniaudio_DIR}
	${wavpack_DIR}/include
	${utfcpp_DIR}/source
    
    # Temporary hack to get macOS working
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../extern/boost_1_74_0
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${madronalib_DIR}/source
	${readerwriterqueue_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    wavpack
)
