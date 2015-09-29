cmake_minimum_required(VERSION 3.0.0)
project(mumsi)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

INCLUDE(FindPkgConfig)
find_package(PkgConfig REQUIRED)

pkg_check_modules(OPUS "opus")
pkg_check_modules(PJSIP "libpjproject")
pkg_check_modules(LOG4CPP "log4cpp")

include_directories(${OPUS_INCLUDE_DIRS})
include_directories(libmumble)
include_directories(${PJSIP_INCLUDE_DIRS})
include_directories(${LOG4CPP_INCLUDE_DIRS})

link_directories(libmumble)

set(SOURCE_FILES main.cpp PjsuaCommunicator.cpp PjsuaMediaPort.cpp MumbleCommunicator.cpp  SoundSampleQueue.cpp)

add_executable(mumsi ${SOURCE_FILES} AbstractCommunicator.hpp)
target_link_libraries(mumsi ${OPUS_LIBRARIES})
target_link_libraries(mumsi ${PJSIP_LIBRARIES})
target_link_libraries(mumsi ${LOG4CPP_LIBRARIES})
target_link_libraries(mumsi mumble)
