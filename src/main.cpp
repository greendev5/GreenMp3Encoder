#include <iostream>
#include <unistd.h>

#include "encoder_app.h"

int main(int argc, char *argv[])
{
    std::cout << "Hello" << std::endl;

    GMp3Enc::EncoderApp app(argc, argv);
    return app.exec();
}
