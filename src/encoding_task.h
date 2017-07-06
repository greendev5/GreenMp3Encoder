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
    static const int LAME_DEFAULR_FRAME_SIZE = 1152;
    static const int LAME_MAX_FRAME_SIZE = 1152 * 2;
    static const int LAME_MAXALBUMART = 128 * 1024;

    static const int PCM_SIZE = sizeof(int32_t) * LAME_MAX_FRAME_SIZE * 2;
    static const int PCM_CHANNEL_SIZE = sizeof(int32_t) * LAME_MAX_FRAME_SIZE;
    static const int MP3_SIZE = 16384 + LAME_MAXALBUMART;

    static const size_t ENCODING_BUFFER_SIZE = MP3_SIZE + PCM_SIZE + PCM_CHANNEL_SIZE * 2;

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
    inline EncodingResult result() const { return r_; }

    std::string sourceFilePath() const;

private:
    EncodingTask(
            const RiffWave &wave,
            const std::string &mp3Destination,
            size_t taskId);
    EncodingTask(const EncodingTask&) {}
    EncodingTask& operator=(const EncodingTask&) {}
    bool initLame();
    std::string lameErrorCodeToStr(int r);

    bool allocateBuffers(uint8_t **mp3Buffer, int32_t **pcmBuffer,
            int32_t **pcmBufferLeft, int32_t **pcmBufferRight,
            int frameSize);

    RiffWave wave_;
    std::string sourceFilePath_;
    std::string mp3Destination_;
    size_t taskId_;
    lame_t lame_;
    std::string errorStr_;
    WorkerThread *executor_;
    uint8_t *taskBuffer_;
    EncodingResult r_;
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
