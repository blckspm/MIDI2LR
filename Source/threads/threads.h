#pragma once
#include <atomic>

class ThreadRAII
{
public:
    enum class DtorAction
    {
        join, detach
    };
    ThreadRAII(std::thread&& t, DtorAction a)
        : die{ false }, action{ a }, t(std::move(t))
    {}
    virtual ~ThreadRAII()
    {
        if (t.joinable())
        {
            if (action == DtorAction::join)
            {
                die = true;
                t.join();
            }
            else
            {
                t.detach();
            }
        }
    }
    ThreadRAII(ThreadRAII&&) = default;
    ThreadRAII& operator=(ThreadRAII&&) = default;
    std::thread& get()
    {
        return t;
    }
    void pleaseDie()
    {
        die = true;
    }
private:
    std::atomic_bool die;
    DtorAction action;
    std::thread t;
};
/*
//thread_local interrupt_flag this_thread_interrupt_flag;
class interrupt_flag
{
    std::atomic<bool> flag;
    std::condition_variable* thread_cond;
    std::mutex set_clear_mutex;
public:
    interrupt_flag():
        thread_cond(0)
    {}
    void set()
    {
        flag.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        if (thread_cond)
        {
            thread_cond->notify_all();
        }
    }
    bool is_set() const
    {
        return flag.load(std::memory_order_relaxed);
    }
    void set_condition_variable(std::condition_variable& cv)
    {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = &cv;
    }
    void clear_condition_variable()
    {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = 0;
    }
    struct clear_cv_on_destruct
    {
        ~clear_cv_on_destruct()
        {
            this_thread_interrupt_flag.clear_condition_variable();
        }
    };
};

// see http://stackoverflow.com/questions/27751967/how-do-i-forward-a-function-and-a-parameter-pack-into-a-lambda-into-the-constru

class interruptible_thread
{
    std::thread internal_thread;
    interrupt_flag* flag;
public:
    template<typename F, typename A...>
    interruptible_thread(F&& f, A&&... args)
    {
        std::promise<interrupt_flag*> p;
        internal_thread = std::thread([](std::promise<interrupt_flag*> p, typename std::decay<F>::type&& f, typename std::decay<A>::type&&... args)
        {
            p.set_value(&this_thread_interrupt_flag);
            try
            {
                f(std::move(args)...);
            }
            catch (thread_interrupted const&)
            {
            }
        });
        flag = p.get_future().get();
    }
    ~interruptible_thread()
    {
        if (internal_thread.joinable())
            internal_thread.join();
    }
    void interrupt()
    {
        if (flag)
        {
            flag->set();
        }
    }
};

class noninterruptible_thread
{
    std::thread internal_thread;
public:
    template<typename F, typename Args...>
    noninterruptible_thread(F f, Args&&... args)
    {
        internal_thread = std::thread(f, std::forward<Args>(args)...);
    }
    ~noninterruptible_thread()
    {
        if (internal_thread.joinable())
            internal_thread.join();
    }
};

template<typename Predicate>
void interruptible_wait(std::condition_variable& cv,
    std::unique_lock<std::mutex>& lk,
    Predicate pred)
{
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    interrupt_flag::clear_cv_on_destruct guard;
    while (!this_thread_interrupt_flag.is_set() && !pred())
    {
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    interruption_point();
}

 this_thread::sleep_for(dur); this_thread::yield() 

std::queue<int> queue;
std::mutex queueMutex;
std::condition_variable queueCondVar;
void provider(int val)
{
// push different values (val til val+5 with timeouts of val milliseconds into the queue
    for (int i = 0; i < 6; ++i)
    {
        {
            std::lock_guard<std::mutex> lg(queueMutex);
            queue.push(val + i);
        } // release lock
        queueCondVar.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(val));
    }
}
void consumer(int num)
{
// pop values if available (num identifies the consumer)
    while (true)
    {
        int val;
        {
            std::unique_lock<std::mutex> ul(queueMutex);
            queueCondVar.wait(ul, []
            {
                return !queue.empty();
            });
            val = queue.front();
            queue.pop();
        } // release lock
        std::cout << "consumer " << num << ": " << val << std::endl;
    }
}

class interrupt_flag
{
    std::atomic<bool> flag;
    std::condition_variable* thread_cond;
    std::mutex set_clear_mutex;
public:
    interrupt_flag():
        thread_cond(0)
    {}
    void set()
    {
        flag.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        if (thread_cond)
        {
            thread_cond->notify_all();
        }
    }
    bool is_set() const
    {
        return flag.load(std::memory_order_relaxed);
    }
    void set_condition_variable(std::condition_variable& cv)
    {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = &cv;
    }
    void clear_condition_variable()
    {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = 0;
    }
    struct clear_cv_on_destruct
    {
        ~clear_cv_on_destruct()
        {
            this_thread_interrupt_flag.clear_condition_variable();
        }
    };
};

template<typename Predicate>
void interruptible_wait(std::condition_variable& cv,
    std::unique_lock<std::mutex>& lk,
    Predicate pred)
{
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    interrupt_flag::clear_cv_on_destruct guard;
    while (!this_thread_interrupt_flag.is_set() && !pred())
    {
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    interruption_point();
}



class interruptible_thread
{
    std::thread internal_thread;
    interrupt_flag* flag;
public:
    template<typename FunctionType>
    interruptible_thread(FunctionType f)
    {
        std::promise<interrupt_flag*> p;
        internal_thread = std::thread([f, &p]
        {
            p.set_value(&this_thread_interrupt_flag);
            try
            {
                f();
            }
            catch (thread_interrupted const&)
            {
            }
        });
        flag = p.get_future().get();
    }
    void interrupt()
    {
        if (flag)
        {
            flag->set();
        }
    }
};*/