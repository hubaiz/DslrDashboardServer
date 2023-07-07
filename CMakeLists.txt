cmake_minimum_required(VERSION 3.5)

project(ddserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(LIBUSB_INCLUDE_DIR "/usr/local/include/libusb-1.0/libusb.h")
set(LIBUSB_LIBRARY "/usr/local/lib/libusb-1.0.so")

include_directories(
                ${PROJECT_SOURCE_DIR}/src
)
file(GLOB all_SRCS
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/*.h"
        )
add_executable(ddserver ${all_SRCS})

#find_path(LIBUSB_INCLUDE_DIR
#  NAMES libusb.h
#  PATH_SUFFIXES "include" "libusb" "libusb-1.0")
#find_library(LIBUSB_LIBRARY
#  NAMES libusb-1.0.so
#  PATH_SUFFIXES "lib")
find_package(Threads REQUIRED)
target_include_directories(ddserver PUBLIC ${LIBUSB_INCLUDE_DIR})
target_link_libraries(ddserver Threads::Threads)
target_link_libraries(ddserver ${LIBUSB_LIBRARY})
