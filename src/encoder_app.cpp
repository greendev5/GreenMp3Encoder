#include "encoder_app.h"

#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>

#include <lame/lame.h>
#include <gmp3enc_version_no.h>

#include "logging_utils.h"


using namespace GMp3Enc;


EncoderApp::EncoderApp(int argc, char *argv[])
    : inactiveTimeoutMs_(150)
    , scanDirs_(false)
{
    // First element in the cmd args array is always
    // called program name.
    for (int i = 1; i < argc; i++)
        cmdOpts_.push_back(std::string(argv[i]));
    threadPool_ = new ThreadPool(4);
}

EncoderApp::~EncoderApp()
{
    delete threadPool_;
}

bool EncoderApp::setSignalMask()
{
    sigemptyset(&sigmask_);
    sigaddset(&sigmask_, SIGTERM);
    sigaddset(&sigmask_, SIGINT);

    int r = sigprocmask(SIG_BLOCK, &sigmask_, 0);
    if (r == -1) {
        GMP3ENC_LOGGER_ERROR("sigprocmask failed: %s.", strerror(errno));
        return false;
    }

    return true;
}

int EncoderApp::exec()
{
    bool needLoop = false;
    int r = parseCmdOpt(needLoop);
    if (!needLoop)
        return r;

    // We must set up a signal mask before running of
    // worker threads.
    // In this case all workers will inherit the main
    // thread` signal mask.
    if (!setSignalMask())
        return -1;

    if (!threadPool_->runThreads()) {
        GMP3ENC_LOGGER_ERROR("Thread pool error");
        return -1;
    }

    if (!executeTasks()) {
        threadPool_->stopThreads();
        GMP3ENC_LOGGER_ERROR("Nothing to run");
        return -1;
    }

    r = eventLoop();

    threadPool_->stopThreads();

    return r;
}

int EncoderApp::eventLoop()
{
    const int gmp3enc_epoll_events_size = 10;

    int appsigfd = signalfd(-1, &sigmask_, SFD_NONBLOCK);
    if (appsigfd == -1) {
        GMP3ENC_LOGGER_ERROR("signalfd failed: %s.", strerror(errno));
        return -1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        GMP3ENC_LOGGER_ERROR("epoll_create1 failed: %s.", strerror(errno));
        close (appsigfd);
        return -1;
    }

    epoll_event event;
    epoll_event events[gmp3enc_epoll_events_size];

    event.events = EPOLLIN;
    event.data.fd = appsigfd;
    int r = epoll_ctl(epollfd, EPOLL_CTL_ADD, appsigfd, &event);
    if (r == -1) {
        close(epollfd);
        close(appsigfd);
        return -1;
    }

    while (true) {
        r = epoll_wait(
                    epollfd,
                    events,
                    gmp3enc_epoll_events_size,
                    inactiveTimeoutMs_);

        if (r == -1)
            break;

        bool needExit = false;
        for (int i = 0; i < r; i++) {
            if (events[i].data.fd = appsigfd) {
                if (readInterruptionSignal(appsigfd)) {
                    GMP3ENC_LOGGER_INFO("Received interuption signal. Exiting...");
                    needExit = true;
                    break;
                }
            }
        }
        if (needExit)
            break;

        // Checking queue;
        if (!processThreadPoolEvents()) {
            GMP3ENC_LOGGER_INFO("All tasks completed. Exiting...");
            break;
        }
    }

    close(epollfd);
    close(appsigfd);
    return 0;
}

bool EncoderApp::readInterruptionSignal(int fd)
{
    const int signals_max_size = 10;
    static signalfd_siginfo siginfo[signals_max_size];

    int n = read(
                fd,
                reinterpret_cast<void*>(siginfo),
                sizeof(signalfd_siginfo) * signals_max_size);

    if (n != -1) {
        int  recvsig = n / sizeof(signals_max_size);
        for (int i =0; i < recvsig; i++) {
            if ((siginfo[i].ssi_signo == SIGINT) || (siginfo[i].ssi_signo == SIGTERM))
                return true;
        }
    }

    return false;
}

bool EncoderApp::processThreadPoolEvents()
{
    std::list<EncodingTask*> startedTasks;
    std::list<EncodingTask*> finishedTasks;

    threadPool_->readThreadMessages(startedTasks, finishedTasks);

    std::list<EncodingTask*>::iterator it;

    for (it = finishedTasks.begin(); it != finishedTasks.end(); ++it) {
        std::list<EncodingTask*>::iterator eit = std::find(
                    inProgressTasks_.begin(),
                    inProgressTasks_.end(),
                    *it);
        if (eit != inProgressTasks_.end())
            inProgressTasks_.erase(eit);
        GMP3ENC_LOGGER_INFO("Completed %s", (*it)->sourceFilePath().c_str());
        EncodingTask *t = *it;
        completedTasks_.push_back(t);
    }

    for (it = startedTasks.begin(); it != startedTasks.end(); ++it) {
        std::list<EncodingTask*>::iterator eit = std::find(
                    finishedTasks.begin(),
                    finishedTasks.end(),
                    *it);
        if (eit != finishedTasks.end())
            continue;

        GMP3ENC_LOGGER_INFO("Started %s", (*it)->sourceFilePath().c_str());
        EncodingTask *t = *it;
        inProgressTasks_.push_back(t);
    }

    return completedTasks_.size() < tasks_.size();
}

bool EncoderApp::executeTasks()
{
    if (!scanDirs_) {
        RiffWave wave(inf_);
        if (wave.isValid()) {
            EncodingTask *task = EncodingTask::create(wave, outf_, 0);
            threadPool_->executeAsyncTask(task);
            tasks_.push_back(task);
        } else {
            GMP3ENC_LOGGER_INFO("Could not open %s", inf_.c_str());
        }

    } else {
        return false;
    }

    return !tasks_.empty();
}

void EncoderApp::showVersion()
{
    printf("gmp3enc version %s (https://github.com/greendev5/GreenMp3Encoder)\n"
           "Based on libmp3lame %s\n",
           PRODUCTVERSTR_DOT,
           get_lame_version());
}

void EncoderApp::showUsage()
{
    showVersion();
    printf("gmp3enc [options] -i <input> -o <output>\n"
           "Required:\n"
           "\t-i --in: Path to input wav file or directory with wav files (for directory mode).\n"
           "\t-o --out: Path for generated mp3 file or path to directory (for directory mode).\n"
           "Optional:\n"
           "\t-d --directories: Directory mode. Process all wav files in a directory <input> and\n"
           "\t\tsave generated mp3 into files in <output> directory.\n"
           "Help:\n"
           "\t-v: show version\n"
           "\t-h --help: show this message\n");
}

std::string EncoderApp::getCmdOptName(const std::string &os)
{
    const char *p = os.c_str();
    while (*p == '-')
        p++;
    return std::string(p);
}

int EncoderApp::parseCmdOpt(bool &needLoop)
{
    needLoop = false;

    std::list<std::string>::iterator it;
    for (it = cmdOpts_.begin(); it != cmdOpts_.end(); ++it) {
        std::string arg = *it;

        if (arg[0] != '-') {
            showUsage();
            return -1;
        }

        arg = getCmdOptName(arg);
        if (arg == "h" || arg == "help") {
            showUsage();
            return 0;
        } else if (arg == "v") {
            showVersion();
            return 0;
        } else if (arg == "o" || arg == "out") {
            ++it;
            if (it == cmdOpts_.end())
                break;
            outf_ = *it;
        } else if (arg == "i" || arg == "in") {
            ++it;
            if (it == cmdOpts_.end())
                break;
            inf_ = *it;
        } else if (arg == "d" || arg == "directories") {
            scanDirs_ = true;
        }
    }

    if (inf_.empty() || outf_.empty()) {
        showUsage();
        return -1;
    }

    needLoop = true;
    return 0;
}
