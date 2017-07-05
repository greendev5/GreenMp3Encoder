#ifndef GMP3ENC_RIFF_WAVE_
#define GMP3ENC_RIFF_WAVE_

#include <stdint.h>
#include <stdio.h>
#include <string>

#include "logging_utils.h"

namespace GMp3Enc {

struct RiffWaveHeaderInternal;

class RiffWave
{
public:
    RiffWave();
    RiffWave(const RiffWave &other);
    RiffWave(const std::string &riffWavePath);
    ~RiffWave();

    RiffWave &operator=(const RiffWave &other);

    bool readWave(const std::string &riffWavePath);
    bool isValid() const;

    bool readSamples(uint8_t *dest, size_t size, size_t &rb);
    bool unpackReadSamples(int *buffer, size_t count, size_t &rs);
    bool seekStart();
    void clear();

    short int channelsNumber() const;
    int samplesPerSec() const;
    int avgBytesPerSec() const;
    int numSamples() const;
    size_t dataSize() const;

    inline std::string riffWavePath() const { return riffWavePath_; }

private:
    std::string riffWavePath_;
    FILE *f_;
    RiffWaveHeaderInternal *hi_;

};

}

#endif
