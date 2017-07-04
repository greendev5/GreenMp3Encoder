#include "encoding_task.h"

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
    int k = (wave_.channelsNumber() == 1) ? 1 : 2;
    size_t size = PCM_SIZE * k * sizeof(short int);
    std::vector<uint8_t> pcmBuffer;
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

    pcmBuffer.reserve(size * 2);
    mp3Buffer.reserve(MP3_SIZE * 2);

    int i = 0;
    while (true) {
        size_t rb = 0;
        int numSamples = 0;
        int wb = 0;
        bool isok = false;

        i++;
        if (i % 10 == 0) {
            if (executor_ && executor_->checkCancelationSignal()) {
                break;
            }
        }

        isok = wave_.readSamples(
                    reinterpret_cast<uint8_t*>(&pcmBuffer[0]),
                    size, rb);
        if (!isok) {
            errorStr_ = "Failed to read PCM source";
            r = EncodingBadSource;
            break;
        }

        if (rb > 0) {
            numSamples = rb / 2;

            if (wave_.channelsNumber() == 1) {
                wb = lame_encode_buffer(
                            lame_,
                            reinterpret_cast<const short int*>(&pcmBuffer[0]),
                            NULL,
                            numSamples,
                            reinterpret_cast<uint8_t*>(&mp3Buffer[0]),
                            MP3_SIZE);
            } else {
                wb = lame_encode_buffer_interleaved(
                            lame_,
                            reinterpret_cast<short int*>(&pcmBuffer[0]),
                            numSamples,
                            reinterpret_cast<uint8_t*>(&mp3Buffer[0]),
                            MP3_SIZE);
            }

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

        if (rb < size) {
            wb = lame_encode_flush(
                        lame_,
                        reinterpret_cast<uint8_t*>(&mp3Buffer[0]),
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
            break;
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

    lame_set_in_samplerate(lame_, wave_.samplesPerSec());
    lame_set_brate(lame_, wave_.avgBytesPerSec());
    if (wave_.channelsNumber() == 1) {
        lame_set_num_channels(lame_, 1);
        lame_set_mode(lame_, MONO);
    } else {
        lame_set_num_channels(lame_, wave_.channelsNumber());
    }
    lame_set_VBR(lame_, vbr_default);
    lame_set_quality(lame_, 2); // hight quality

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
