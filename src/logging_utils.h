#ifndef GMP3ENC_LOGGING_UTILS_
#define GMP3ENC_LOGGING_UTILS_

#include <stdio.h>

#define GMP3ENC_LOGGER_INFO(format, ...) {                               \
    fprintf(stderr, "GMP3ENC [INFO]: " format "\n", ##__VA_ARGS__);     \
}

#define GMP3ENC_LOGGER_ERROR(format, ...) {                              \
    fprintf(stderr, "GMP3ENC [ERROR]: " format "\n", ##__VA_ARGS__);    \
}

#define GMP3ENC_LOGGER_DEBUG(format, ...) {                              \
    fprintf(stderr, "GMP3ENC [DEBUG]: " format "\n", ##__VA_ARGS__);    \
}

#endif
