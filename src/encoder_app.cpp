#include "encoder_app.h"

#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <string.h>

#include "logging_utils.h"


using namespace GMp3Enc;


EncoderApp::EncoderApp(int argc, char *argv[])
    : inactiveTimeoutMs_(150)
{
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

    int r = eventLoop();

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
    //GMP3ENC_LOGGER_INFO("Check queue");
    return true;
}
