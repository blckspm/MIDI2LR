#pragma once
//much of the following is adapted from Stroustrup C++ Programming Language 4th ed
#include <thread>
#include <chrono>
#include <atomic>

//RAII version of thread
class guarded_thread: std::thread
{
public:
    using std::thread::thread;
    ~guarded_thread()
    {
        if (joinable()) join();
    }
};

template<typename T>
class Sync_queue
{
public:
    void put(const T& val)
    {
        std::lock_guard<std::mutex> lck(mtx);
        q.push_back(val);
        cond.notify_one();
    };
    void put(T&& val)
    {
        std::lock_guard<std::mutex> lck(mtx);
        q.emplace_back(std::forward<T>(val));
        cond.notify_one();
    };
    bool get(T& val)
    {
        if (!running.load())
            return false;
        std::unique_lock<std::mutex> lck(mtx);
        cond.wait(lck, [this]
        {
            return !q.empty() || !running.load();
        });
        if (running.load() && !q.empty())
        {
            val = q.front();
            q.pop_front();
            return true;
        }
        return false;
    };
    bool get(T& val, std::chrono::steady_clock::duration d)
    {
        if (!running.load())
            return false;
        std::unique_lock<std::mutex> lck(mtx);
        bool expired = cond.wait_for(lck, d, [this]
        {
            return !q.empty() || !running.load();
        });
        if (running.load() && !q.empty())
        {
            val = q.front();
            q.pop_front();
            return true;
        }
        return false;
    };
    void stop()
    {
        running.store(false);
        cond.notify_all();
    };
    void restart()
    {
        running.store(true);
    };
private:
    std::mutex mtx;
    std::condition_variable cond;
    std::list<T> q;
    std::atomic_bool running{ true };
};
