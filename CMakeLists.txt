cmake_minimum_required(VERSION 3.13...3.27)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(third_party/pico-sdk/pico_sdk_init.cmake)

project(kat VERSION 0.0.0 LANGUAGES C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# add project source directory
add_subdirectory(kat)
