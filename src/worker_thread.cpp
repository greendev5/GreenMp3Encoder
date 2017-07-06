#include "worker_thread.h"

using namespace GMp3Enc;

WorkerThread::WorkerThread(EncodingTaskQueue &taskQueue, EncodingResultQueue &resultQueue)
    : taskQueue_(taskQueue)
    , resultQueue_(resultQueue)
    , isRunning_(false)
    , currentTask_(NULL)
    , buffer_(NULL)
{
}

WorkerThread::~WorkerThread()
{
    if (buffer_)
        delete[] buffer_;
}

bool WorkerThread::start()
{
    if (isRunning_)
        return false;

    // Every thread has own buffer for performing encoding:
    try {
        buffer_ = new uint8_t[EncodingTask::ENCODING_BUFFER_SIZE];
    } catch(std::bad_alloc &e) {
        GMP3ENC_LOGGER_ERROR(
                    "Failed to allocate internal buffer for thread. Size: %zu",
                    EncodingTask::ENCODING_BUFFER_SIZE);
        buffer_ = NULL;
        return false;
    }

    int r = pthread_mutex_init(&cancelMutex_, NULL);
    if (r)
        return false;
    pthread_mutex_lock(&cancelMutex_);

    r = pthread_create(
                &pthreadId_,
                NULL,
                &threadFunc,
                reinterpret_cast<void*>(this));
    if (r) {
        pthread_mutex_unlock(&cancelMutex_);
        return false;
    }

    isRunning_ = true;
    return true;
}

void WorkerThread::join()
{
    if (!isRunning_)
        return;
    pthread_join(pthreadId_, NULL);
    isRunning_ = false;
}

void WorkerThread::sendCancelSignal()
{
    if (isRunning_)
        pthread_mutex_unlock(&cancelMutex_);
}

bool WorkerThread::checkCancelationSignal()
{
    if (!pthread_mutex_trylock(&cancelMutex_)) {
        pthread_mutex_unlock(&cancelMutex_);
        return true;
    }
    return false;
}

void WorkerThread::exec()
{
    MessageQueueRcvResult r;
    do {
        r = taskQueue_.recv(currentTask_, true);

        if (r == MsgQResSuccess) {
            EncodingNotification ntf;

            // Notify main thread that we started encoding:
            ntf.task = currentTask_;
            ntf.type = EncodingNotification::EncodingStarted;
            ntf.result = EncodingTask::EncodingSuccess;
            resultQueue_.send(ntf);

            // Using this pointer to check when we must interrupt
            currentTask_->setExecutor(this);

            // Do encoding:
            EncodingTask::EncodingResult r = currentTask_->encode();

            // Nonify main thread that incoding was completed:
            ntf.task = currentTask_;
            ntf.type = EncodingNotification::EncodingFinished;
            ntf.result = r;
            resultQueue_.send(ntf);

        }

    } while (r != MsgQResInvalid);
}

void* WorkerThread::threadFunc(void *h)
{
    WorkerThread *obj = static_cast<WorkerThread*>(h);
    obj->exec();
    return NULL;
}
