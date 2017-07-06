#include "encoder_app.h"
#include "encoding_task.h"
#include "riff_wave.h"

int main(int argc, char *argv[])
{
    GMp3Enc::EncoderApp app(argc, argv);
    return app.exec();
}
