#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <vector>
#include <queue>
#include <functional>

class ThreadPool {
public:
    ThreadPool(int num)
        : m_thread_nums(num)
    {
        try {
            for (int i = 0; i < m_thread_nums; ++i) {  
                // 构造时创建一定数量的线程
                m_threads.emplace_back([this] { workloop(); });
            }
        }
        catch(...) {
            m_thread_nums = m_threads.size();  // 如果创建线程时遇到了问题
            if (m_thread_nums == 0) {
                throw std::runtime_error("Failed to create any thread");
            }
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(m_mtx);  // m_stop 和 m_tasks 关联，需要用同一把锁
            m_stop = true;
        }
        m_cv.notify_all();  // 注意要通知后才能唤醒线程，让它们结束
        for (auto& thr : m_threads) {
            thr.join();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    // 每个线程都执行的循环
    void workloop() {
        // 每个线程都不断从 m_tasks 中取任务执行，直到 m_stop
        while (1) {
            std::unique_lock<std::mutex> lock(m_mtx);   // 条件变量要搭配能自由上锁解锁的类
            m_cv.wait(lock, [this]() { return !m_tasks.empty() || m_stop; });
            if (m_tasks.empty() && m_stop) {
                return;  // 停止后仍然会完成队列中没有完成的任务
            }

            std::function<void ()> task = std::move(m_tasks.front());  // 使用移动提高性能
            m_tasks.pop();
            lock.unlock();

            task();
        }
    }

    template<typename Func, typename... Args>
    auto add_task(Func&& func, Args&&... args) 
        -> std::future<std::invoke_result_t<Func, Args...>>   // 返回类型复杂时用这种方式写
    {
        using return_type = std::invoke_result_t<Func, Args...>;  // 函数返回类型

        if (m_stop) {
            throw std::runtime_error("ThreadPool is already stopped!");
        }

        /* C++ 20 写法，Lambda 可以捕获模板参数包
        auto task_ptr = std::make_shared<std::packaged_task<return_type()>>( 
            [func = std::forward<Func>(func), ... args = std::forward<Args>(args)]() mutable {  // 注意这里需要 mutable，因为调用对象的函数调用符可能不是 const 的
                func(args...);
        });
        */

        /* C++ 17 使用 std::bind 写法
        auto task_ptr = std::make_shared<std::packaged_task<return_type()>>( 
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );
        */

        // C++ 17 使用 Lambda + std::apply
        auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
            [func = std::forward<Func>(func), 
                tup = std::make_tuple(std::forward<Args>(args)...)]() mutable 
            {
                return std::apply(std::move(func), std::move(tup));
            }
        );

        {  // 添加任务
            std::lock_guard<std::mutex> lock(m_mtx);
            m_tasks.emplace([task_ptr]() {
                (*task_ptr)();
            });

        }  // 使用局部作用域让 lock 在这里解锁

        m_cv.notify_one();
        return task_ptr->get_future();
    }

    int threads_nums() { return m_thread_nums; }

private:
    int m_thread_nums;
    bool m_stop = false;
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;  // 使用统一的 function 包裹任务
    std::condition_variable m_cv;   // 用来通知取任务执行或停止
    std::mutex m_mtx;   // 用来锁 m_tasks
};


// 测试代码
int main() {
    ThreadPool pool(4);

    // 提交无返回值任务
    auto result1 = pool.add_task([]() {
        std::cout << "Hello from thread " << std::this_thread::get_id() << std::endl;
    });
    result1.get(); // 等待完成

    // 提交有返回值任务
    auto result2 = pool.add_task([](int a, int b) {
        return a + b;
    }, 10, 20);
    std::cout << "10 + 20 = " << result2.get() << std::endl;

    // 提交多个任务并收集 future
    std::vector<std::future<int>> results;
    for (int i = 0; i < 10; ++i) {
        results.emplace_back(pool.add_task([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return i * i;
        }));
    }

    for (auto &result : results) {
        std::cout << result.get() << " ";
    }
    std::cout << std::endl;

    return 0;
}