#ifndef GMP3ENC_WORKER_THREAD_
#define GMP3ENC_WORKER_THREAD_

#include <stdint.h>

#include "encoding_task.h"
#include "message_queue.h"

namespace GMp3Enc
{

typedef MessageQueue<EncodingTask*> EncodingTaskQueue;
typedef MessageQueue<EncodingNotification> EncodingResultQueue;

class WorkerThread
{
public:
    WorkerThread(EncodingTaskQueue &taskQueue,
                 EncodingResultQueue &resultQueue);
    ~WorkerThread();

    bool start();
    void join();
    void sendCancelSignal();
    bool checkCancelationSignal();

    inline bool isRunning() const { return isRunning_; }

    inline uint8_t* internalBuffer() { return buffer_; }

private:
    WorkerThread& operator=(const WorkerThread&) {}
    void exec();
    static void* threadFunc(void* h);

    EncodingTaskQueue &taskQueue_;
    EncodingResultQueue &resultQueue_;
    pthread_t pthreadId_;
    bool isRunning_;
    EncodingTask *currentTask_;
    pthread_mutex_t cancelMutex_;
    uint8_t *buffer_;
};

}

#endif
