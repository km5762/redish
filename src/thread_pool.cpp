//
// Created by d4wgr on 5/5/2025.
//

#include "thread_pool.h"

#include <queue>
#include <thread>

ThreadPool::ThreadPool(const size_t size) {
    m_workers.reserve(size);
    for (size_t i{0}; i < size; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::unique_lock lock{m_tasks_mutex};
                m_task_available.wait(lock, [this] { return !m_tasks.empty() || m_done; });

                if (m_done) {
                    return;
                }

                std::function task = m_tasks.front();
                task();
                m_tasks.pop();
                lock.unlock();
            }
        });
    }
}

void ThreadPool::enqueue(const std::function<void()> &task) { {
        std::lock_guard lock{m_tasks_mutex};
        m_tasks.push(task);
    }
    m_task_available.notify_one();
}

ThreadPool::~ThreadPool() {
    m_done = true;
    m_task_available.notify_all();
    for (auto &worker: m_workers) {
        worker.join();
    }
}


