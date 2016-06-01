#pragma once
//much of the following is adapted from Stroustrup C++ Programming Language 4th ed

//RAII version of thread
struct guarded_thread: thread
{
    using thread::thread;
    ˜guarded_thread()
    {
        if (t.joinable()) t.join();
    }
};

template<typename T>
class Sync_queue
{
public:
    void put(const T& val)
    {
        lock_guard<mutex> lck(mtx);
        q.push_back(val);
        cond.notify_one();
    };
    void put(T&& val)
    {
        lock_guard<mutex> lck(mtx);
        q.emplace_back(std::forward<T>(val));
        cond.notify_one();
    };
    bool get(T& val)
    {
        if (!running.load())
            return false;
        unique_lock<mutex> lck(mtx);
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
    bool get(T& val, steady_clock::duration d)
    {
        if (!running.load())
            return false;
        unique_lock<mutex> lck(mtx);
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
    mutex mtx;
    condition_variable cond;
    list<T> q;
    atomic_bool running{ true };
};
