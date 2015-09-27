cmake_minimum_required(VERSION 3.1)
project(mumsi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

INCLUDE(FindPkgConfig)
find_package(PkgConfig REQUIRED)

pkg_check_modules(OPUS "opus")
pkg_check_modules(SNDFILE "sndfile")
pkg_check_modules(PJSIP "libpjproject")

include_directories(${OPUS_INCLUDE_DIRS})
include_directories(libmumble)
include_directories(${PJSIP_INCLUDE_DIRS})
link_directories(libmumble)

set(SOURCE_FILES main.cpp PjsuaCommunicator.cpp PjsuaMediaPort.cpp)
add_executable(mumsi ${SOURCE_FILES})
target_link_libraries(mumsi ${OPUS_LIBRARIES})
target_link_libraries(mumsi ${SNDFILE_LIBRARIES})
target_link_libraries(mumsi ${PJSIP_LIBRARIES})
target_link_libraries(mumsi mumble)
