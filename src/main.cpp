#include "encoder_app.h"
#include "encoding_task.h"
#include "riff_wave.h"

int main(int argc, char *argv[])
{
//    GMp3Enc::RiffWave w;
//    w.readWave("./temple_of_love-sisters_of_mercy.wav");

//    if (!w.isValid()) {
//        return -1;
//    }

//    GMp3Enc::EncodingTask *t = GMp3Enc::EncodingTask::create(
//                w, "./1.mp3", 0);

//    t->encode();

    GMp3Enc::EncoderApp app(argc, argv);
    return app.exec();
}
