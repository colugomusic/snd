cmake_minimum_required(VERSION 3.23)
project(snd)
add_library(snd INTERFACE)
add_library(snd::snd ALIAS ads)
target_sources(snd INTERFACE
	FILE_SET snd
	TYPE HEADERS
	BASE_DIRS include
)
target_compile_features(snd INTERFACE cxx_std_20)