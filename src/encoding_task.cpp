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
{
}

EncodingTask::~EncodingTask()
{
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
    EncodingResult r = EncodingSuccess;

    std::vector<int32_t> pcmBuffer;
    std::vector<int32_t> pcmBufferLeft;
    std::vector<int32_t> pcmBufferRight;
    std::vector<uint8_t> mp3Buffer;

    FILE *outf = fopen(mp3Destination_.c_str(), "wb");
    if (!outf) {
        errorStr_ = "Could not open destination file";
        return EncodingBadDestination;
    }

    if (!initLame()) {
        fclose(outf);
        return EncodingSystemError;
    }

    int frameSize = lame_get_framesize(lame_);
    if (frameSize <= 0) {
        fclose(outf);
        lame_close(lame_);
        lame_ = NULL;
        return EncodingSystemError;
    }

    pcmBuffer.resize(frameSize * wave_.channelsNumber());
    if (wave_.channelsNumber() == 2) {
        pcmBufferLeft.resize(frameSize);
        pcmBufferRight.resize(frameSize);
    }
    mp3Buffer.resize(MP3_SIZE);

    int wb = 0;
    int i = 0;
    while (true) {
        size_t rs = 0;
        int numSamples = 0;
        bool isok = false;

        int *bufl = NULL;
        int *bufr = NULL;

        i++;
#ifdef __linux__
//        if (i % 10 == 0) {
//            if (executor_ && executor_->checkCancelationSignal()) {
//                break;
//            }
//        }
#endif

        isok = wave_.unpackReadSamples(&pcmBuffer[0], frameSize * wave_.channelsNumber(), rs);
        if (!isok) {
            errorStr_ = "Failed to read PCM source";
            r = EncodingBadSource;
            break;
        }

        if (!rs)
            break;

        numSamples = rs / wave_.channelsNumber();
        if (wave_.channelsNumber() == 2) {
            int32_t *p = &pcmBuffer[0] + rs;
            for (int j = numSamples; --j >= 0;) {
                pcmBufferLeft[j] = *--p;
                pcmBufferRight[j] = *--p;
            }
            bufl = &pcmBufferLeft[0];
            bufr = &pcmBufferRight[0];

        } else {
            bufl = &pcmBuffer[0];
        }

        wb = lame_encode_buffer_int(
                    lame_,
                    bufl,
                    bufr,
                    numSamples,
                    &mp3Buffer[0],
                    MP3_SIZE);

        if (wb < 0) {
            errorStr_ = "lame processing error: " + lameErrorCodeToStr(wb);
            r = EncodingSystemError;
            break;
        } else if (wb > 0) {
            size_t owb = fwrite(&mp3Buffer[0], 1, wb, outf);
            if (owb != wb) {
                errorStr_ = "Failed to write into output file";
                r = EncodingBadDestination;
                break;
            }
        }
    }

    if (r == EncodingSuccess) {
        wb = lame_encode_flush(
                    lame_,
                    &mp3Buffer[0],
                    MP3_SIZE);
        if (wb < 0) {
            errorStr_ = "lame processing error: " + lameErrorCodeToStr(wb);
            r = EncodingSystemError;
        } else if (wb > 0) {
            size_t owb = fwrite(&mp3Buffer[0], 1, wb, outf);
            if (owb != wb) {
                errorStr_ = "Failed to write into output file";
                r = EncodingBadDestination;
            }
        }
    }

    fclose(outf);
    lame_close(lame_);

    return r;
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
