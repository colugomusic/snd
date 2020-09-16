project(snd)

set(dir_madronalib ${CMAKE_CURRENT_LIST_DIR}/submodules/madronalib)

list(APPEND PUBLIC_HEADERS
	./include/snd/convert.h
	./include/snd/dup_filter.h
	./include/snd/ease.h
	./include/snd/interpolation.h
	./include/snd/misc.h
	./include/snd/mlext.h
	./include/snd/audio/clipping.h
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
	./include/snd/threads/audio_object.h
	./include/snd/threads/audio_object_manager.h
	./include/snd/threads/garbage_disposal.h
)

list(APPEND SRC
	./src/audio/filter/serial/2-pole.cpp
	./src/audio/filter/serial/2-pole_mono.cpp
	./src/clock/clock_divider.cpp
	./src/control/smoother.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/include/snd PREFIX public_headers FILES ${PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src PREFIX src FILES ${SRC})

add_library(${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
	./include/snd
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${dir_madronalib}/source
)