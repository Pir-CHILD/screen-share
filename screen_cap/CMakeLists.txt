cmake_minimum_required(VERSION 3.10)

project(screen_cap)

set(CMAKE_CXX_STANDARD 14)

set(HPP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu")
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libavdevice
    libavfilter
    libavformat
    libavutil
    libswresample
    libswscale
)

add_executable(screen_cap main.cc)

# .hpp
target_include_directories(screen_cap PRIVATE ${HPP_PATH})

target_link_libraries(screen_cap PkgConfig::LIBAV)

