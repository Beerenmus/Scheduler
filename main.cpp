#include <condition_variable>
#include <mutex>
#include <list>
#include <queue>
#include <thread>
#include <functional>
#include <iostream>
#include <future>
#include <chrono>

using Task = std::packaged_task<void()>;
using Signal = std::future<void>;

class Scheduler {

    private:
        using Tasks = std::queue<Task>;
        using Threads = std::list<std::thread>;

        std::size_t m_count;
        std::mutex m_mutex;
        std::condition_variable condition;

        Tasks m_tasks;
        Threads m_threads;

        bool m_running;

    public:
        Scheduler(std::size_t count) : m_count(count), m_running(true) {

            for (std::size_t x = 0; x < m_count; x++) {

                m_threads.emplace_back([this]() {

                    while (m_running) {

                        Task task;

                        {
                            std::unique_lock<std::mutex> lock(m_mutex);
                            condition.wait(lock, [this]() {
                                return !m_tasks.empty() || !m_running;
                            });

                            if (!m_running && m_tasks.empty()) {
                                return;
                            }

                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        task();
                    }
                });
            }

            for (auto& thread : m_threads) {
                thread.detach();
            }
        }

        Signal addTask(std::function<void()>&& func) {

            Task task(std::move(func));
            auto future = task.get_future();

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_tasks.push(std::move(task));
            }

            condition.notify_one();

            return future;
        }

        void stop() {

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_running = false;
            }

            condition.notify_all();
        }

        ~Scheduler() {
            stop();
        }
};

int main() {

    Scheduler scheduler(3);

    std::mutex m;

    auto signalInstance = scheduler.addTask([&]() {

        m.lock();
        std::cout << "VkInstance erstellt" << std::endl;
        m.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    });

    auto signalDevice = scheduler.addTask([&]() {

        signalInstance.wait();
        
        m.lock();
        std::cout << "VkDevice erstellt" << std::endl;
        m.unlock();
    });

    auto signalSurface = scheduler.addTask([&]() {

        signalInstance.wait();

        m.lock();
        std::cout << "VkSurface erstellt" << std::endl;
        m.unlock();
    });


    signalDevice.wait();
    signalSurface.wait();

    scheduler.stop();

    return 0;
}

