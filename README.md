## Synopsis

**GreenMp3Encoder** simple mp3 encoder for RIFF Wave files. Based on **lame** 3.99.5

## Motivation

Just a test/research project written with plain C++03. This project is an attempt to show
my ability to work in embedded Linux and Windows enviroments. Lots of compilers for
embedded devices have limitations and don't support modern C++11/14 standards. It is not
always reasonable to use cross-standards C++ libraries (boost/poco/qt) for easy task like
.wav files encoding. This project shows how to implement multithreaded .wav to .mp3 encoder
with minimal dependencies. So far it requires only **libmp3lame** and **pthreads**.

## Build Linux

In the source code directory, run the following commands:

    $ cd 3rdparty
    $ python build_linux.py

This python script **build_linux.py** should build a static version of lame library.
GreenMp3Encoder will be linked with this library. No need to intall *libmp3lame-dev* into
your system.
If build is successful, you can check libmp3lame.a:

    $ ls build/lib

It is time to run **cmake**. I prefer to use cmake outside of the source code directory:

    $ mkdir ~/build-GreenMp3Encoder
    $ cd ~/build-GreenMp3Encoder
    $ cmake <path_to_source_code>
    $ make

Executable **gmp3enc** should be generated. To test:

    $ ./gmp3enc -v

Output:

    gmp3enc version 0.1.0 (https://github.com/greendev5/GreenMp3Encoder)
    Based on libmp3lame 3.99.5

## Build Windows

You have to intall Python 2.7 and MSVS 2013 on your machine before continue.
In the source code directory, run the following commands:

    > cd 3rdparty
    > python build_win32.py

This python script **build_win32.py** should build a static version of lame library and
pthread-w32. You don't have to intall **libmp3lame** and **pthreadw32** into your system.
Call cmake in selected folder:

    > cmake <path_to_source_code>
    > cmake --build .

## Usage

You can check usage information running the following command:

    $ ./gmp3enc -h

Encode single file:

    $ ./gmp3enc -i input.wav -o output.mp3

Encode all .wav files in a directory and save .mp3 in the same directory:

    $ ./gmp3enc -d -i ~/mymusic/ -i ~/mymusic/

On Linux you can stop encoding sending SIGTERN or SIGINT signals to the encoder process.
Or just Ctrl^C in terminal.

## Few Words About Application Design

GreenMp3Encoder process contains several threads:

* Main thread - for file system scanning, system signals handling and terminal output.
* Worker threads - just for wave data encoding.

For communication between threads I use message queues, based on pthread mutex and wait condition.

In general we have one producer and multiple concumers (worker threads). Worker
threads aslo enqueue encoding result  into message queueu.
This queue is checked by main thread in non-blocking way.

The encoding task - process whole .wav files by chunks using **lame_encode_buffer_int** method.

I was diging a lot in the lame frotend source code. I used their method of parsing RIFF files
headers and reading 2 channels wave.

## TODO and Limitations

1. No encoding progress bar
    
    My idea is implement simple terminal progress bar based on **ncureses** or another terminal library.
    That is why I chose this (a bit complecated) architecture of the application, with one management
    thread and several worker threads. Management thread doesn't perform any encoding, but it can check
    progress of tasks in worker threads. Also, this approach allows to run all terminal i/o only in the
    management thread.

2. Specify arch in build process

    For now you can build only 32 bit version for Windows, using MSVS 2013.
    For linux build arch is selected according to you default GCC toolchain.
    It would be good to allow user to choose arch in cmake and python build scripts.

3. Improve event loop for Windows version

    For linux event loop I used **epoll** and it looks good for timing and handling signals (SIGTER)
    for correct application shutdown.
    Windows event loop is just a simple call of Sleep(). Not shutdown signal handling. We completely rely
    on OS and its method of stopping threads.

4. Unicode support for Windows version

    Windows version of GreenMp3Encoder doesn't support UTF16 file names and command line arguments.
    If you try to encode file with not latin1 name, GreenMp3Encoder won't open this file and show 
    an error.