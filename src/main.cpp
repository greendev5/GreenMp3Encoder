#include "encoder_app.h"

int main(int argc, char *argv[])
{
    GMp3Enc::EncoderApp app(argc, argv);
    return app.exec();
}
