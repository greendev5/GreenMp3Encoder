#ifndef GMP3ENC_ENCODER_APP_
#define GMP3ENC_ENCODER_APP_

#ifdef __linux__
#include <signal.h>
#endif
#include "thread_pool.h"

namespace GMp3Enc {

class EncoderApp
{
public:
    EncoderApp(int argc, char *argv[]);
    ~EncoderApp();

    int exec();

private:
#ifdef __linux__
    bool setSignalMask();
    bool readInterruptionSignal(int fd);
    int eventLoopEpoll();
#elif defined(_WIN32)
    int eventLoopWinApi();
#endif

    int eventLoop();

    bool processThreadPoolEvents();
    bool executeTasks();

    void showVersion();
    void showUsage();
    std::string getCmdOptName(const std::string &os);
    int parseCmdOpt(bool &needLoop);
    void listDirectory(std::string &dir, std::list<std::string> &wavFiles);
    std::string generateOutFileName(std::string &inFileName);

    std::list<std::string> cmdOpts_;
    std::string inf_;
    std::string outf_;
    bool scanDirs_;

    std::list<EncodingTask*> tasks_;
    std::list<EncodingTask*> inProgressTasks_;
    std::list<EncodingTask*> completedTasks_;

    ThreadPool *threadPool_;
    int inactiveTimeoutMs_;
#ifdef __linux__
    sigset_t sigmask_;
#endif
};

}

#endif
