cmake_minimum_required(VERSION 3.10)

project(server)

# add_executable(hello_world hello_world.cc)
add_executable(server main.cc)

# Abseil requires C++14
set(CMAKE_CXX_STANDARD 14)

# Set third party path
set(THIRD_PARTY_PATH ../third_party)

# Process Abseil's CMake build system
target_include_directories(server PRIVATE ${THIRD_PARTY_PATH}/abseil-cpp)
add_subdirectory(${THIRD_PARTY_PATH}/abseil-cpp ${CMAKE_BINARY_DIR}/abseil-cpp/build)

# Declare dependency on the absl::strings library
# target_link_libraries(hello_world PRIVATE absl::strings)
target_link_libraries(server PRIVATE absl::time)