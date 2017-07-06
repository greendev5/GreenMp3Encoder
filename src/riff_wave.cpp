#include "riff_wave.h"

#include <stdint.h>
#include <string.h>
#include <sstream>

// Many thanks to lame frontend developers! :)
static int const WAV_ID_RIFF = 0x52494646; // "RIFF"
static int const WAV_ID_WAVE = 0x57415645; // "WAVE"
static int const WAV_ID_FMT  = 0x666d7420; // "fmt "
static int const WAV_ID_DATA = 0x64617461; // "data"

static short const WAVE_FORMAT_PCM        = 0x0001;
static short const WAVE_FORMAT_IEEE_FLOAT = 0x0003;
static short const WAVE_FORMAT_EXTENSIBLE = 0xFFFE;

static long make_even_number_of_bytes_in_length(long x)
{
    if ((x & 0x01) != 0) {
        return x + 1;
    }
    return x;
}

static int read_16_bits_low_high(FILE * fp)
{
    unsigned char bytes[2] = { 0, 0 };
    fread(bytes, 1, 2, fp);
    {
        int32_t const low = bytes[0];
        int32_t const high = (signed char) (bytes[1]);
        return (high << 8) | low;
    }
}

static int read_32_bits_low_high(FILE * fp)
{
    unsigned char bytes[4] = { 0, 0, 0, 0 };
    fread(bytes, 1, 4, fp);
    {
        int32_t const low = bytes[0];
        int32_t const medl = bytes[1];
        int32_t const medh = bytes[2];
        int32_t const high = (signed char) (bytes[3]);
        return (high << 24) | (medh << 16) | (medl << 8) | low;
    }
}

static int read_16_bits_high_low(FILE * fp)
{
    unsigned char bytes[2] = { 0, 0 };
    fread(bytes, 1, 2, fp);
    {
        int32_t const low = bytes[1];
        int32_t const high = (signed char) (bytes[0]);
        return (high << 8) | low;
    }
}

static int read_32_bits_high_low(FILE * fp)
{
    unsigned char bytes[4] = { 0, 0, 0, 0 };
    fread(bytes, 1, 4, fp);
    {
        int32_t const low = bytes[3];
        int32_t const medl = bytes[2];
        int32_t const medh = bytes[1];
        int32_t const high = (signed char) (bytes[0]);
        return (high << 24) | (medh << 16) | (medl << 8) | low;
    }
}

static void write_16_bits_low_high(FILE * fp, int val)
{
    unsigned char bytes[2];
    bytes[0] = (val & 0xff);
    bytes[1] = ((val >> 8) & 0xff);
    fwrite(bytes, 1, 2, fp);
}

static void write_32_bits_low_high(FILE * fp, int val)
{
    unsigned char bytes[4];
    bytes[0] = (val & 0xff);
    bytes[1] = ((val >> 8) & 0xff);
    bytes[2] = ((val >> 16) & 0xff);
    bytes[3] = ((val >> 24) & 0xff);
    fwrite(bytes, 1, 4, fp);
}

using namespace GMp3Enc;

struct GMp3Enc::RiffWaveHeaderInternal
{
    int formatTag;
    int channels;
    int blockAlign;
    int bitsPerSample;
    int samplesPerSec;
    int avgBytesPerSec;
    long dataSize;
    long dataOffset;
    unsigned long numSamples;
};

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

    int type = read_32_bits_high_low(f_);
    if (type != WAV_ID_RIFF) {
        clear();
        return false;
    }

    long fileLength = read_32_bits_high_low(f_);
    if (read_32_bits_high_low(f_) != WAV_ID_WAVE) {
        clear();
        return false;
    }

    int formatTag = 0;
    int channels = 0;
    int blockAlign = 0;
    int bitsPerSample = 0;
    int samplesPerSec = 0;
    int avgBytesPerSec = 0;
    long dataSize = 0;
    long subSize = 0;

    bool is_wav = false;
    for (int i = 0; i < 20; i++) {
        type = read_32_bits_high_low(f_);

        if (type == WAV_ID_FMT) {
            subSize = read_32_bits_low_high(f_);
            subSize = make_even_number_of_bytes_in_length(subSize);
            if (subSize < 16) {
                clear();
                return false;
            }

            formatTag = read_16_bits_low_high(f_);
            subSize -= 2;
            channels = read_16_bits_low_high(f_);
            subSize -= 2;
            samplesPerSec = read_32_bits_low_high(f_);
            subSize -= 4;
            avgBytesPerSec = read_32_bits_low_high(f_);
            subSize -= 4;
            blockAlign = read_16_bits_low_high(f_);
            subSize -= 2;
            bitsPerSample = read_16_bits_low_high(f_);
            subSize -= 2;

            if ((subSize > 9) && (formatTag == WAVE_FORMAT_EXTENSIBLE)) {
                read_16_bits_low_high(f_);
                read_16_bits_low_high(f_);
                read_32_bits_low_high(f_);
                formatTag = read_16_bits_low_high(f_);
                subSize -= 10;
            }

            if (subSize > 0) {
                if (fseek(f_, (long) subSize, SEEK_CUR) != 0) {
                    clear();
                    return false;
                }
            }

        } else if (type == WAV_ID_DATA) {
            is_wav = true;
            subSize = read_32_bits_low_high(f_);
            dataSize = subSize;
            break;

        } else {
            subSize = read_32_bits_low_high(f_);
            subSize = make_even_number_of_bytes_in_length(subSize);
            if (fseek(f_, (long) subSize, SEEK_CUR) != 0) {
                clear();
                return false;
            }
        }
    }

    if (!is_wav) {
        clear();
        return false;
    }

    if (channels != 1 && channels != 2) {
        clear();
        return false;
    }

    if (bitsPerSample != 8  && bitsPerSample != 16 &&
        bitsPerSample != 24 && bitsPerSample != 32) {
        clear();
        return false;
    }

    if (formatTag != WAVE_FORMAT_PCM) {
        clear();
        return false;
    }

    hi_ = new RiffWaveHeaderInternal;
    hi_->formatTag = formatTag;
    hi_->channels = channels;
    hi_->blockAlign = blockAlign;
    hi_->bitsPerSample = bitsPerSample;
    hi_->samplesPerSec = samplesPerSec;
    hi_->avgBytesPerSec = avgBytesPerSec;
    hi_->dataSize = dataSize;
    hi_->dataOffset = ftell(f_);
    hi_->numSamples = dataSize / (channels * ((bitsPerSample + 7) / 8));

    return true;
}

bool RiffWave::isValid() const
{
    return hi_ != NULL;
}

bool RiffWave::unpackReadSamples(int *buffer, size_t count, size_t &rs)
{
    const int b = sizeof(int) * 8;
    int bytesPerSample = hi_->bitsPerSample / 8;
    bool swapOrder = bytesPerSample == 1;

    if (!isValid())
        return false;

    if (!f_) {
        if (!seekStart())
            return false;
    }

    if (feof(f_)) {
        rs = 0;
        return true;
    }

    rs = fread(buffer, bytesPerSample, count, f_);
    if (rs != count) {
        if (ferror(f_))
            return false;
    }

    unsigned char *ip = reinterpret_cast<unsigned char *>(buffer);
    int *op = buffer + rs;

    // Lame frontend algo:
    if (!swapOrder) {

        if (bytesPerSample == 1) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 8);
        } else if (bytesPerSample == 2) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 16) | ip[i + 1] << (b - 8);
        } else if (bytesPerSample == 3) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 24) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 8);
        } else if (bytesPerSample == 4) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 32) | ip[i + 1] << (b - 24) | ip[i + 2] << (b - 16) | ip[i + 3] << (b - 8);
        }

    } else {

        if (bytesPerSample == 1) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = (ip[i] ^ 0x80) << (b - 8) | 0x7f << (b - 16); /* convert from unsigned */
        } else if (bytesPerSample == 2) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 8) | ip[i + 1] << (b - 16);
        } else if (bytesPerSample == 3) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 8) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 24);
        } else if (bytesPerSample == 4) {
            for(int i = rs * bytesPerSample; (i -= bytesPerSample) >=0;)
                * --op = ip[i] << (b - 8) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 24) | ip[i + 3] << (b - 32);
        }

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

    return fseek(f_, hi_->dataOffset, SEEK_SET) == 0;
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
    return hi_->channels;
}

int RiffWave::samplesPerSec() const
{
    if (!hi_)
        return 0;
    return hi_->samplesPerSec;
}

int RiffWave::avgBytesPerSec() const
{
    if (!hi_)
        return 0;
    return hi_->avgBytesPerSec;
}

int RiffWave::numSamples() const
{
    if (!hi_)
        return 0;
    return hi_->numSamples;
}

size_t RiffWave::dataSize() const
{
    if (!hi_)
        return 0;
    return static_cast<size_t>(hi_->dataSize);
}
