#ifndef GMP3ENC_ENCODING_TASK_
#define GMP3ENC_ENCODING_TASK_

#include <string>

#include <stdio.h>

namespace GMp3Enc {

class WorkerThread;

class EncodingTask
{
public:
    enum EncodingResult
    {
        EncodingSuccess,
        EncodingBadSource,
        EncodingSystemError
    };

    ~EncodingTask();

    static EncodingTask* create(
            const std::string &wavSource,
            const std::string &mp3Destination,
            size_t taskId);

    EncodingResult encode();

    void setExecutor(WorkerThread *executor);

    inline size_t taskId() const { return taskId_; }

private:
    EncodingTask();
    EncodingTask(const EncodingTask&) {}
    EncodingTask& operator=(const EncodingTask&) {}

    size_t taskId_;
    WorkerThread *executor_;
};

struct EncodingNotification
{
    enum NotificationType
    {
        EncodingStarted,
        EncodingFinished
    };

    EncodingTask *task;
    NotificationType type;
    EncodingTask::EncodingResult result;

};

}

#endif
