#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace te {

class ThreadPool {
public:
    ThreadPool(uint32_t num_threads) {
        for (uint32_t i = 0; i < num_threads; ++i) {
            m_threads.emplace_back([this]() { this->worker_loop(); });
        }
    }
    ~ThreadPool() { shutdown(); }
    void shutdown() {
        m_running = false;
        m_condition.notify_all();
        for (auto& t : m_threads) {
            if (t.joinable()) t.join();
        }
    }
private:
    void worker_loop() {
        while (m_running) {
            std::this_thread::yield();
        }
    }
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_running{true};
    std::condition_variable m_condition;
};

} // namespace te
