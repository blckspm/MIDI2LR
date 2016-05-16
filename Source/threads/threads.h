#pragma once
class ThreadRAII
{
public:
    enum class DtorAction
    {
        join, detach
    }; // see Item 10 for
       // enum class info
    ThreadRAII(std::thread&& t, DtorAction a) // in dtor, take
        : action(a), t(std::move(t))
    {} // action a on t
    ~ThreadRAII()
    { // see below for
        if (t.joinable())
        { // joinability test
            if (action == DtorAction::join)
            {
                t.join();
            }
            else
            {
                t.detach();
            }
        }
    }
    ThreadRAII(ThreadRAII&&) = default; // support
    ThreadRAII& operator=(ThreadRAII&&) = default; // moving
    std::thread& get()
    {
        return t;
    } // see below
private:
    DtorAction action;
    std::thread t;
};
/* this_thread::sleep_for(dur); this_thread::yield() */


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