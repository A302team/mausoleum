#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ConcurrentQueue {
public:
    void push(T item);
    bool pop(T& out);
    void close();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool closed_ = false;
};

#include "common/ConcurrentQueue.inl"
