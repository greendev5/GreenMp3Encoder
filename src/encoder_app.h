#ifndef GMP3ENC_ENCODER_APP_
#define GMP3ENC_ENCODER_APP_

#include <signal.h>
#include "thread_pool.h"

namespace GMp3Enc {

class EncoderApp
{
public:
    EncoderApp(int argc, char *argv[]);
    ~EncoderApp();

    int exec();

private:
    bool setSignalMask();
    int eventLoop();
    bool readInterruptionSignal(int fd);
    bool processThreadPoolEvents();

    ThreadPool *threadPool_;
    int inactiveTimeoutMs_;
    sigset_t sigmask_;
};

}

#endif
