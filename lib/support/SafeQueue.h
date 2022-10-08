//
// From https://stackoverflow.com/a/16075550/252308
//
#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>

#include <QDebug>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
public:
    SafeQueue(void)
        : interrupted{false}
        , q()
        , m()
        , c()
    {}

    ~SafeQueue(void)
    {}

    /**
     * @brief Add an element to the queue.
     * @param t
     */
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    // .
    //

    /**
     * @brief Get the "front"-element
     * If the queue is empty, wait till a element is available.
     * If interrupt is called, return nullptr.
     * @return
     */
    T dequeue()
    {
        std::unique_lock<std::mutex> lock(m);
        while (q.empty())
        {
            if (interrupted) return nullptr;
            // release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
        }
        T val = q.front();
        q.pop();
        return val;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m);
        std::queue<T> empty;
        std::swap( q, empty );
        c.notify_one();
    }

    void interrupt()
    {
        std::lock_guard<std::mutex> lock(m);
        interrupted = true;
        c.notify_one();
    }

private:
    bool interrupted;
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};

#endif
