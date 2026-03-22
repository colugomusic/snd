cmake_minimum_required(VERSION 3.30)
project(snd-test)
add_executable(snd-test)
target_sources(snd-test PRIVATE
	src/doctest.h
	src/main.cpp
)
target_link_libraries(snd-test snd::snd)
target_compile_definitions(snd-test PRIVATE
	ASSETS_DIR="${CMAKE_CURRENT_LIST_DIR}/assets"
)
add_test(NAME snd-test COMMAND snd-test)
