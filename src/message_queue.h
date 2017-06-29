#ifndef GMP3ENC_MESSAGE_QUEUE_
#define GMP3ENC_MESSAGE_QUEUE_

#include <pthread.h>
#include <queue>
#include <list>

namespace GMp3Enc {

class MutexGuard
{
public:
    MutexGuard(pthread_mutex_t *mutex)
        : isLocked_(true)
        , mutex_(mutex)
    {
        lock();
    }

    ~MutexGuard()
    {
        unlock();
    }

    inline bool isLocked() const
    {
        return isLocked_;
    }

    void lock()
    {
        isLocked_ = true;
        pthread_mutex_lock(mutex_);
    }

    void unlock()
    {
        if (isLocked_) {
            pthread_mutex_unlock(mutex_);
            isLocked_ = false;
        }
    }

private:
    bool isLocked_;
    pthread_mutex_t *mutex_;
};

enum MessageQueueRcvResult
{
    MsgQResSuccess,
    MsgQResEmpty,
    MsgQResInvalid
};

template <typename T>
class MessageQueue
{
public:
    MessageQueue()
        : isInitialized_(false)
        , isValid_(false)
    {
    }

    ~MessageQueue()
    {
        if (isInitialized_) {
            pthread_cond_destroy(&q_condv_);
            pthread_mutex_destroy(&q_mutex_);
        }
    }

    bool init()
    {
        int r = pthread_mutex_init(&q_mutex_, NULL);
        if (r)
            return false;
        r = pthread_cond_init(&q_condv_, NULL);
        if (r) {
            pthread_mutex_destroy(&q_mutex_);
            return false;
        }
        isInitialized_ = true;
        isValid_ = true;
        return true;
    }

    MessageQueueRcvResult recv(T &item, bool wait)
    {
        MutexGuard g(&q_mutex_);

        while (true) {
            if (!isValid_)
                return MsgQResInvalid;
            if (!queue_.empty()) {
                item = queue_.front();
                queue_.pop();
                return MsgQResSuccess;
            }
            if (wait)
                pthread_cond_wait(&q_condv_, &q_mutex_);
        }

        return MsgQResEmpty;
    }

    MessageQueueRcvResult recvAll(std::list<T> &items, bool wait)
    {
        MutexGuard g(&q_mutex_);

        while (true) {
            if (!isValid_)
                return MsgQResInvalid;
            if (!queue_.empty()) {
                items.clear();
                while (!queue_.empty()) {
                    items.push_back(queue_.front());
                    queue_.pop();
                }
                return MsgQResSuccess;
            }
            if (wait)
                pthread_cond_wait(&q_condv_, &q_mutex_);
        }

        return MsgQResEmpty;
    }

    void send(const T &item)
    {
        MutexGuard g(&q_mutex_);
        queue_.push(item);
        pthread_cond_signal(&q_condv_);
    }

    void invalidate()
    {
        MutexGuard g(&q_mutex_);
        isValid_ = false;
        pthread_cond_broadcast(&q_condv_);
    }

    bool isValid() const
    {
        MutexGuard g(&q_mutex_);
        return isValid_;
    }

    bool isInitialized() const
    {
        return isInitialized_;
    }

private:
    bool isInitialized_;
    bool isValid_;
    std::queue<T> queue_;
    pthread_mutex_t q_mutex_;
    pthread_cond_t  q_condv_;
};

}

#endif
