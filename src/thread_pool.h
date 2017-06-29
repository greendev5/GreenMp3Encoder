#ifndef GMP3ENC_THREAD_POOL_
#define GMP3ENC_THREAD_POOL_

#include <vector>
#include <list>
#include "worker_thread.h"

namespace GMp3Enc
{

class ThreadPool
{
public:
    ThreadPool(size_t threadsCount);
    ~ThreadPool();

    bool runThreads();
    void stopThreads();

    void readThreadMessages(
            std::list<EncodingTask*> &startedTasks,
            std::list<EncodingTask*> &finishedTasks);

    bool executeAsyncTask(EncodingTask *task);

private:
    ThreadPool(const ThreadPool&) {}
    ThreadPool& operator=(const ThreadPool&) {}

    EncodingTaskQueue taskQueue_;
    EncodingResultQueue resultMsgQueue_;
    std::vector<WorkerThread*> workers_;
    std::list<EncodingTask*> runningTasks_;
};

}

#endif
