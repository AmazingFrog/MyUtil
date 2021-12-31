#include <queue>
#include <mutex>
#include <tuple>
#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>

/**
 * @brief 线程池
 * @code {.cpp}
 * shared_ptr<shochu::ThreadPoolTask<Args...>> task(new shochu::ThreadPoolTask<Args...>(func));
 * task->data = makr_tuple(...);
 * ThreadPool::getInstance()->addTask(task);
 * @endcode
 * @note 用make_shared会报错
 */
namespace shochu {

class TaskInterface {
public:
    virtual void run() = 0;
};

template<typename... Args>
class ThreadPoolTask : public TaskInterface {
public:
    ThreadPoolTask() = delete;
    ThreadPoolTask(std::function<void(Args...)> f) : func(f) {}
    std::tuple<Args...> data;

private:
    std::function<void(Args...)> func;
    friend class ThreadPool;

    template<size_t... i>
    void callFuncHelper(std::index_sequence<i...>) {
        this->func(std::get<i>(this->data)...);
    }

    void run() override {
        this->callFuncHelper(std::make_index_sequence<sizeof...(Args)>());
    }
};

class ThreadPool {
public:
    static ThreadPool* getInstance();

    ~ThreadPool();
    enum AddTaskMethod {
        Push,
        Discard
    };
    void addTask(std::shared_ptr<TaskInterface> task, AddTaskMethod method = Push);

    void setThreadNum(unsigned int num);
    void setTaskMaxNum(unsigned int num);

private:
    unsigned int threadNum;
    unsigned int taskMaxNum;

    std::mutex qMutex;
    std::queue<std::shared_ptr<TaskInterface>> q;

    bool isQuit;
    std::mutex cvMutex;
    std::condition_variable_any cv;

    std::vector<std::unique_ptr<std::thread>> threads;

private:
    ThreadPool();
    void workThread();
    void checkThread();

};

}
