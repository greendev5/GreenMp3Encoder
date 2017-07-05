#ifndef GMP3ENC_ENCODING_TASK_
#define GMP3ENC_ENCODING_TASK_

#include <string>
#include <stdio.h>

#include "riff_wave.h"

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
typedef lame_global_flags *lame_t;

namespace GMp3Enc {

class WorkerThread;

class EncodingTask
{
public:
    enum EncodingResult
    {
        EncodingSuccess,
        EncodingBadSource,
        EncodingBadDestination,
        EncodingSystemError
    };

    ~EncodingTask();

    static EncodingTask* create(
            const RiffWave &wave,
            const std::string &mp3Destination,
            size_t taskId);

    EncodingResult encode();

    void setExecutor(WorkerThread *executor);

    inline size_t taskId() const { return taskId_; }
    inline std::string errorStr() const { return errorStr_; }

    std::string sourceFilePath() const;

private:
    static const int LAME_MAXALBUMART = 128 * 1024;

    static const int PCM_SIZE = 8192;
    static const int MP3_SIZE = 16384 + LAME_MAXALBUMART;

    EncodingTask(
            const RiffWave &wave,
            const std::string &mp3Destination,
            size_t taskId);
    EncodingTask(const EncodingTask&) {}
    EncodingTask& operator=(const EncodingTask&) {}
    bool initLame();
    std::string lameErrorCodeToStr(int r);

    RiffWave wave_;
    std::string sourceFilePath_;
    std::string mp3Destination_;
    size_t taskId_;
    lame_t lame_;
    std::string errorStr_;
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
