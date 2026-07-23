#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadCount) {
    if (threadCount == 0)
        threadCount = 1; // hardware_concurrency() can return 0 when undetectable
    workers_.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i)
        workers_.emplace_back([this] { workerLoop(); });
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_)
        if (worker.joinable())
            worker.join();
}

void ThreadPool::submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.push(std::move(job));
    }
    cv_.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
            // Drain whatever's still queued before actually stopping, even
            // once stopping_ is set - a job submitted just before shutdown
            // still deserves to run rather than being silently dropped.
            if (stopping_ && jobs_.empty())
                return;
            job = std::move(jobs_.front());
            jobs_.pop();
        }
        job();
    }
}
