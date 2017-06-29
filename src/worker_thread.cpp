#include "worker_thread.h"

using namespace GMp3Enc;

WorkerThread::WorkerThread(EncodingTaskQueue &taskQueue, EncodingResultQueue &resultQueue)
    : taskQueue_(taskQueue)
    , resultQueue_(resultQueue)
    , isRunning_(false)
    , currentTask_(NULL)
{
}

WorkerThread::~WorkerThread()
{
}

bool WorkerThread::start()
{
    if (isRunning_)
        return false;

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
            // Notify main thread that we started encoding.
            EncodingNotification ntf;
            ntf.task = currentTask_;
            ntf.type = EncodingNotification::EncodingStarted;
            ntf.result = EncodingTask::EncodingSuccess;
            resultQueue_.send(ntf);

            currentTask_->setExecutor(this);
            currentTask_->encode();
        }

    } while (r != MsgQResInvalid);
}

void* WorkerThread::threadFunc(void *h)
{
    WorkerThread *obj = static_cast<WorkerThread*>(h);
    obj->exec();
    return NULL;
}
