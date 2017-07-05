#include "riff_wave.h"

#include <stdint.h>
#include <string.h>
#include <sstream>

#define RIFF_WAVE_CHUNK_ID_VALUE 0x46464952
#define RIFF_WAVE_FORMAT_VALUE 0x45564157
#define RIFF_WAVE_SUBCHUNK1ID_VALUE 0x20746d66

using namespace GMp3Enc;

#pragma pack(push, 1)
struct GMp3Enc::RiffWaveHeaderInternal
{
    uint32_t ChunkID;
    uint32_t ChunkSize;
    uint32_t Format;
    uint32_t Subchunk1ID;
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    uint32_t Subchunk2ID;
    uint32_t Subchunk2Size;
};
#pragma pack(pop)


RiffWave::RiffWave()
    : f_(NULL)
    , hi_(NULL)
{
}

RiffWave::RiffWave(const RiffWave &other)
    : f_(NULL)
    , hi_(NULL)
{
    if (other.isValid()) {
        hi_ = new RiffWaveHeaderInternal;
        memcpy(hi_, other.hi_, sizeof(RiffWaveHeaderInternal));
        riffWavePath_ = other.riffWavePath_;
    }
}

RiffWave::RiffWave(const std::string &riffWavePath)
    : f_(NULL)
    , hi_(NULL)
{
    readWave(riffWavePath);
}

RiffWave::~RiffWave()
{
    clear();
}

RiffWave &RiffWave::operator=(const RiffWave &other)
{
    clear();

    if (other.isValid()) {
        hi_ = new RiffWaveHeaderInternal;
        memcpy(hi_, other.hi_, sizeof(RiffWaveHeaderInternal));
        riffWavePath_ = other.riffWavePath_;
    }

    return *this;
}

bool RiffWave::readWave(const std::string &riffWavePath)
{
    clear();

    riffWavePath_ = riffWavePath;
    f_ = fopen(riffWavePath_.c_str(), "rb");
    if (!f_)
        return false;

    hi_ = new RiffWaveHeaderInternal;
    size_t nr = fread(
                hi_,
                sizeof(RiffWaveHeaderInternal),
                1,
                f_);
    if (nr != 1) {
        delete hi_;
        return false;
    }

    if (hi_->ChunkID     != RIFF_WAVE_CHUNK_ID_VALUE    ||
        hi_->Format      != RIFF_WAVE_FORMAT_VALUE      ||
        hi_->Subchunk1ID != RIFF_WAVE_SUBCHUNK1ID_VALUE ||
        hi_->AudioFormat != 1) {
        delete hi_;
        return false;
    }

    return true;
}

bool RiffWave::isValid() const
{
    return hi_ != NULL;
}

bool RiffWave::readSamples(uint8_t *dest, size_t size, size_t &rb)
{
    if (!isValid())
        return false;

    if (!f_) {
        if (!seekStart())
            return false;
    }

    if (feof(f_)) {
        rb = 0;
        return true;
    }

    rb = fread(dest, 1, size, f_);
    if (rb != size) {
        if (ferror(f_))
            return false;
    }

    return true;
}

bool RiffWave::seekStart()
{
    if (!isValid())
        return false;

    if (!f_) {
        f_ = fopen(riffWavePath_.c_str(), "rb");
        if (!f_)
            return false;
    }

    return fseek(f_, 0, SEEK_SET) == 0;
}

void RiffWave::clear()
{
    riffWavePath_.clear();

    if (f_) {
        fclose(f_);
        f_ = NULL;
    }

    if (hi_) {
        delete hi_;
        hi_ = NULL;
    }
}

short int RiffWave::channelsNumber() const
{
    if (!hi_)
        return 0;
    return static_cast<short int>(hi_->NumChannels);
}

int RiffWave::samplesPerSec() const
{
    if (!hi_)
        return 0;
    return static_cast<int>(hi_->SampleRate);
}

int RiffWave::avgBytesPerSec() const
{
    if (!hi_)
        return 0;
    return static_cast<int>(hi_->ByteRate);
}

size_t RiffWave::dataSize() const
{
    if (!hi_)
        return 0;
    return static_cast<size_t>(hi_->Subchunk2Size);
}
