#include "thread_pool.h"
#include "logging_utils.h"

using namespace GMp3Enc;

ThreadPool::ThreadPool(size_t threadsCount)
{
    workers_.resize(threadsCount);
    for (size_t i = 0; i < workers_.size(); i++)
        workers_[i] = new WorkerThread(taskQueue_, resultMsgQueue_);
}

ThreadPool::~ThreadPool()
{
    stopThreads();
}

bool ThreadPool::runThreads()
{
    if (!taskQueue_.init()) {
        GMP3ENC_LOGGER_ERROR("Failed to init taskQueue.");
        return false;
    }
    if (!resultMsgQueue_.init()) {
        GMP3ENC_LOGGER_ERROR("Failed to init resultMsgQueue_.");
        return false;
    }

    for (size_t i = 0; i < workers_.size(); i++) {
        if (!workers_[i]->start()) {
            GMP3ENC_LOGGER_ERROR("Failed to run worker.");
            stopThreads();
            return false;
        }
    }

    return true;
}

void ThreadPool::stopThreads()
{
    if (taskQueue_.isInitialized())
        taskQueue_.invalidate();

    bool hasStoppedThreads = false;
    for (size_t i = 0; i < workers_.size(); i++) {
        if (workers_[i]->isRunning()) {
            workers_[i]->sendCancelSignal();
            workers_[i]->join();
            hasStoppedThreads = true;
        }
    }
    if (hasStoppedThreads)
        GMP3ENC_LOGGER_DEBUG("Worker threads were stopped.");
}

void ThreadPool::readThreadMessages(
        std::list<EncodingTask*> &startedTasks,
        std::list<EncodingTask*> &finishedTasks)
{
    startedTasks.clear();
    finishedTasks.clear();

    if (!resultMsgQueue_.isInitialized())
        return;

    std::list<EncodingNotification> ntfs;
    resultMsgQueue_.recvAll(ntfs, false);

    if (ntfs.empty())
        return;

    std::list<EncodingNotification>::iterator it;
    for (it = ntfs.begin(); it != ntfs.end(); ++it) {
        if (it->type == EncodingNotification::EncodingStarted)
            startedTasks.push_back(it->task);
        else if (it->type == EncodingNotification::EncodingFinished)
            finishedTasks.push_back(it->task);
    }
}

bool ThreadPool::executeAsyncTask(EncodingTask *task)
{
    if (!taskQueue_.isInitialized() || !workers_[0]->isRunning())
        return false;

    taskQueue_.send(task);
}
