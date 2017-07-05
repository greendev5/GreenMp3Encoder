cmake_minimum_required(VERSION 3.5)

project(greenMp3Enc)

set (GMP3ENC_VERSION_MAJOR 0)
set (GMP3ENC_VERSION_MINOR 1)
set (GMP3ENC_VERSION_BUILD 0)

set (GMP3ENC_SOURCE_DIR ${CMAKE_SOURCE_DIR})

set (GMP3ENC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker_thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoding_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoder_app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/riff_wave.cpp)

set (GMP3ENC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker_thread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/message_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoding_task.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/encoder_app.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logging_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/riff_wave.h)

set (GMP3ENC_GCC_COMPILE_FLAGS
    "-std=c++03 -g")

set (GMP3ENC_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}/substitutes
    ${GMP3ENC_SOURCE_DIR}/3rdparty/build/include)

if (MSVC)
    set (GMP3ENC_STATIC_DEPS_LIBS
        ${GMP3ENC_SOURCE_DIR}/3rdparty/build/lib/libmp3lame-static.lib
        ${GMP3ENC_SOURCE_DIR}/3rdparty/build/lib/pthreadVC2.lib)
    add_definitions(-DPTW32_STATIC_LIB)
else()
    set (GMP3ENC_STATIC_DEPS_LIBS
        ${GMP3ENC_SOURCE_DIR}/3rdparty/build/lib/libmp3lame.a)
    set (GMP3ENC_SYSTEM_DEPS_LIBS
        pthread)
endif()

set (CMAKE_CXX_FLAGS
    ${CMAKE_CXX_FLAGS} ${GMP3ENC_GCC_COMPILE_FLAGS})

configure_file (${GMP3ENC_SOURCE_DIR}/substitutes/version_no.h.in ${CMAKE_BINARY_DIR}/substitutes/gmp3enc_version_no.h )
include_directories (${GMP3ENC_INCLUDE_DIRECTORIES})
add_executable (gmp3enc ${GMP3ENC_SOURCES} ${GMP3ENC_HEADERS})
target_link_libraries (gmp3enc ${GMP3ENC_SYSTEM_DEPS_LIBS} ${GMP3ENC_STATIC_DEPS_LIBS})
