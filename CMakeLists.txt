cmake_minimum_required(VERSION 3.5)

project(greenMp3Enc)

set (GMP3ENC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker_thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoding_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoder_app.cpp)

set (GMP3ENC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker_thread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/message_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoding_task.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoder_app.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logging_utils.h)

set(GMP3ENC_GCC_COMPILE_FLAGS
    "-std=c++03 -g")

set(CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS} ${GMP3ENC_GCC_COMPILE_FLAGS})

add_executable(gmp3enc ${GMP3ENC_SOURCES} ${GMP3ENC_HEADERS})
target_link_libraries(gmp3enc pthread)
