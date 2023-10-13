cmake_minimum_required(VERSION 3.10)

project(screen_cap)

set(CMAKE_CXX_STANDARD 14)

set(THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

add_executable(screen_cap main.cc)

# .h
target_include_directories(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg)

# .a
target_link_directories(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg)

target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libavcodec/libavcodec.a)
target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libavdevice/libavdevice.a)
target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libavfilter/libavfilter.a)
target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libavformat/libavformat.a)
target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libavutil/libavutil.a)
# target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libpostproc/libpostproc.a)
target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libswresample/libswresample.a)
target_link_libraries(screen_cap PRIVATE ${THIRD_PARTY_PATH}/ffmpeg/libswscale/libswscale.a)
