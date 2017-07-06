#include "encoding_task.h"

#include <string.h>
#include <vector>
#include <lame/lame.h>

#include "worker_thread.h"

using namespace GMp3Enc;

EncodingTask::EncodingTask(
        const RiffWave &wave,
        const std::string &mp3Destination,
        size_t taskId)
    : wave_(wave)
    , sourceFilePath_(wave.riffWavePath())
    , mp3Destination_(mp3Destination)
    , taskId_(taskId)
    , lame_(NULL)
    , executor_(NULL)
    , taskBuffer_(NULL)
    , r_(EncodingSuccess)
{
}

EncodingTask::~EncodingTask()
{
    if (taskBuffer_)
        delete[] taskBuffer_;
}

EncodingTask* EncodingTask::create(
        const RiffWave &wave,
        const std::string &mp3Destination,
        size_t taskId)
{
    if (!wave.isValid())
        return NULL;
    return new EncodingTask(wave, mp3Destination, taskId);
}


EncodingTask::EncodingResult EncodingTask::encode()
{
    r_ = EncodingSuccess;

    uint8_t* mp3Buffer = NULL;
    int32_t* pcmBuffer = NULL;
    int32_t* pcmBufferLeft = NULL;
    int32_t* pcmBufferRight = NULL;

    FILE *outf = fopen(mp3Destination_.c_str(), "wb");
    if (!outf) {
        errorStr_ = "Could not open destination file";
        r_ = EncodingBadDestination;
        return r_;
    }

    if (!initLame()) {
        fclose(outf);
        r_ = EncodingSystemError;
        return r_;
    }

    int frameSize = lame_get_framesize(lame_);
    if (frameSize <= 0) {
        errorStr_ = "Bad lame frame size";
        fclose(outf);
        lame_close(lame_);
        lame_ = NULL;
        r_ = EncodingSystemError;
        return r_;

    } else if (frameSize > LAME_MAX_FRAME_SIZE) {
        frameSize = LAME_MAX_FRAME_SIZE;
    }

    if (!allocateBuffers(&mp3Buffer, &pcmBuffer, &pcmBufferLeft, &pcmBufferRight, frameSize)) {
        errorStr_ = "Failed to allocate buffers";
        fclose(outf);
        lame_close(lame_);
        lame_ = NULL;
        r_ = EncodingSystemError;
        return r_;
    }

    int wb = 0;
    int i = 0;
    while (true) {
        size_t readSamples = 0;
        int numSamples = 0;
        bool isok = false;

        int *bufl = NULL;
        int *bufr = NULL;

        i++;
#ifdef __linux__
        if (i % 10 == 0) {
            if (executor_ && executor_->checkCancelationSignal()) {
                break;
            }
        }
#endif

        isok = wave_.unpackReadSamples(pcmBuffer, frameSize * wave_.channelsNumber(), readSamples);
        if (!isok) {
            errorStr_ = "Failed to read PCM source";
            r_ = EncodingBadSource;
            break;
        }

        if (!readSamples)
            break;

        numSamples = readSamples / wave_.channelsNumber();
        if (wave_.channelsNumber() == 2) {
            int32_t *p = pcmBuffer + readSamples;
            for (int j = numSamples; --j >= 0;) {
                pcmBufferLeft[j] = *--p;
                pcmBufferRight[j] = *--p;
            }
            bufl = pcmBufferLeft;
            bufr = pcmBufferRight;

        } else {
            bufl = pcmBuffer;
        }

        wb = lame_encode_buffer_int(
                    lame_,
                    bufl,
                    bufr,
                    numSamples,
                    mp3Buffer,
                    MP3_SIZE);

        if (wb < 0) {
            errorStr_ = "lame processing error: " + lameErrorCodeToStr(wb);
            r_ = EncodingSystemError;
            break;
        } else if (wb > 0) {
            size_t owb = fwrite(mp3Buffer, 1, wb, outf);
            if (owb != wb) {
                errorStr_ = "Failed to write into output file";
                r_ = EncodingBadDestination;
                break;
            }
        }
    }

    if (r_ == EncodingSuccess) {
        wb = lame_encode_flush(
                    lame_,
                    mp3Buffer,
                    MP3_SIZE);
        if (wb < 0) {
            errorStr_ = "lame processing error: " + lameErrorCodeToStr(wb);
            r_ = EncodingSystemError;
        } else if (wb > 0) {
            size_t owb = fwrite(mp3Buffer, 1, wb, outf);
            if (owb != wb) {
                errorStr_ = "Failed to write into output file";
                r_ = EncodingBadDestination;
            }
        }
    }

    fclose(outf);
    lame_close(lame_);
    lame_ = NULL;

    return r_;
}

void EncodingTask::setExecutor(WorkerThread *executor)
{
    executor_ = executor;
}

std::string EncodingTask::sourceFilePath() const
{
    return sourceFilePath_;
}

bool EncodingTask::initLame()
{
    lame_ = lame_init();
    if (!lame_) {
        errorStr_ = "Failed to init lame encoder.";
        return false;
    }

    lame_set_findReplayGain(lame_, 1);
    lame_set_num_samples(lame_, wave_.numSamples());
    lame_set_in_samplerate(lame_, wave_.samplesPerSec());
    lame_set_brate(lame_, wave_.avgBytesPerSec());
    if (wave_.channelsNumber() == 1) {
        lame_set_num_channels(lame_, 1);
        lame_set_mode(lame_, MONO);
    } else {
        lame_set_num_channels(lame_, wave_.channelsNumber());
    }
    //lame_set_VBR(lame_, vbr_mtrh);
    lame_set_quality(lame_, 2); // high quality

    if (lame_init_params(lame_) < 0) {
        errorStr_ = "Fatal error during lame initialization.";
        return false;
    }

    return true;
}

std::string EncodingTask::lameErrorCodeToStr(int r)
{
    if (r >= 0)
        return std::string("no error");

    switch (r) {
    case -1:
        return std::string("mp3buf was too small");
    case -2:
        return std::string("malloc() problem");
    case -3:
        return std::string("lame_init_params() not called");
    case -4:
        return std::string("psycho acoustic problems");
    default:
        break;
    }

    return std::string("unknown error");
}

bool EncodingTask::allocateBuffers(uint8_t **mp3Buffer, int32_t **pcmBuffer,
        int32_t **pcmBufferLeft, int32_t **pcmBufferRight,
        int frameSize)
{
    uint8_t *buf;
    if (executor_) {

        buf = executor_->internalBuffer();
        if (!buf) {
            return false;
        }

    } else {

        try {
            taskBuffer_ = new uint8_t[ENCODING_BUFFER_SIZE];
        } catch(std::bad_alloc) {
            taskBuffer_ = NULL;
            return false;
        }
        buf = taskBuffer_;

    }

    *mp3Buffer = buf;

    *pcmBuffer = reinterpret_cast<int32_t*>(
                buf + MP3_SIZE);

    *pcmBufferLeft = reinterpret_cast<int32_t*>(
                buf + MP3_SIZE + (frameSize * wave_.channelsNumber()) * sizeof(int32_t));

    *pcmBufferRight = reinterpret_cast<int32_t*>(
                buf + MP3_SIZE + (frameSize * wave_.channelsNumber()) * sizeof(int32_t) +
                frameSize * sizeof(int32_t));

    return true;
}
