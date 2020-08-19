project(snd)

list(APPEND PUBLIC_HEADERS
	./include/snd/convert.h
	./include/snd/ease.h
	./include/snd/misc.h
	./include/snd/audio/filter/1-pole.h
	./include/snd/audio/filter/1-pole_mono.h
	./include/snd/audio/filter/1-pole_stereo.h
	./include/snd/audio/filter/2-pole.h
	./include/snd/audio/filter/2-pole_mono.h
	./include/snd/audio/filter/2-pole_stereo.h
	./include/snd/audio/saturator/moron_saturator.h
	./include/snd/audio/saturator/moron_saturator_mono.h
	./include/snd/audio/saturator/moron_saturator_stereo.h
)

list(APPEND SRC
	./src/audio/filter/1-pole.cpp
	./src/audio/filter/1-pole_mono.cpp
	./src/audio/filter/1-pole_stereo.cpp
	./src/audio/filter/2-pole.cpp
	./src/audio/filter/2-pole_mono.cpp
	./src/audio/filter/2-pole_stereo.cpp
	./src/audio/saturator/moron_saturator.cpp
	./src/audio/saturator/moron_saturator_mono.cpp
	./src/audio/saturator/moron_saturator_stereo.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/include/snd PREFIX public_headers FILES ${PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src PREFIX src FILES ${SRC})

add_library(${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
	./include/snd
)