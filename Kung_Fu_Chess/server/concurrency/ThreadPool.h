#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// A fixed-size pool of worker threads pulling jobs from one shared queue -
// no job is pinned to a particular worker; whichever thread is free next
// picks up whatever's queued next. Generic, standalone infrastructure -
// knows nothing about GameSession/rooms/networking; submit() takes any
// no-argument callable. Threads are created once at construction and
// joined at destruction - no resizing.
class ThreadPool {
public:
    // Defaults to the number of hardware threads available (falls back to
    // 1 if that can't be detected, rather than constructing a pool with no
    // workers at all).
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> job);

private:
    void workerLoop();

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stopping_ = false;
};
