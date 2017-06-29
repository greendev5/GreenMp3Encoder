#include "encoding_task.h"

using namespace GMp3Enc;

EncodingTask::EncodingTask()
    : executor_(NULL)
{
}

EncodingTask::~EncodingTask()
{
}

EncodingTask* EncodingTask::create(const std::string &wavSource, const std::string &mp3Destination, size_t taskId)
{
    return NULL;
}


EncodingTask::EncodingResult EncodingTask::encode()
{
    return EncodingSystemError;
}

void EncodingTask::setExecutor(WorkerThread *executor)
{
    executor_ = executor;
}
