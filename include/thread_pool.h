//
// Created by d4wgr on 5/5/2025.
//

#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>


class ThreadPool {
public:
    explicit ThreadPool(size_t size = std::thread::hardware_concurrency());

    void enqueue(const std::function<void()> &task);

    ~ThreadPool();

private
:
    std::queue<std::function<void()> > m_tasks{};
    std::mutex m_tasks_mutex{};
    std::condition_variable m_task_available{};
    std::atomic_bool m_done{false};
    std::vector<std::thread> m_workers{};
};


#endif //THREAD_POOL_H
