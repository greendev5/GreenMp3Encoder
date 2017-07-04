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
    bool executeTasks();

    void showVersion();
    void showUsage();
    std::string getCmdOptName(const std::string &os);
    int parseCmdOpt(bool &needLoop);

    std::list<std::string> cmdOpts_;
    std::string inf_;
    std::string outf_;
    bool scanDirs_;

    std::list<EncodingTask*> tasks_;
    std::list<EncodingTask*> inProgressTasks_;
    std::list<EncodingTask*> completedTasks_;

    ThreadPool *threadPool_;
    int inactiveTimeoutMs_;
    sigset_t sigmask_;
};

}

#endif
