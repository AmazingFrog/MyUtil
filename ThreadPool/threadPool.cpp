#include "threadPool.hpp"

#include <unistd.h>

shochu::ThreadPool* shochu::ThreadPool::getInstance() {
    static ThreadPool p;
    return &p;
}

shochu::ThreadPool::ThreadPool() :
    threadNum(4),
    taskMaxNum(10),
    isQuit(false) {
    using namespace std;
    for(unsigned int i=0;i<this->threadNum;++i) {
        this->threads.push_back(move(unique_ptr<thread>(new thread(&ThreadPool::workThread, this))));
        this->threads[i]->detach();
    }

    std::thread(&ThreadPool::checkThread, this).detach();
}

shochu::ThreadPool::~ThreadPool() {
    this->isQuit = true;
    this->cv.notify_all();
    //TODO: wait thread finish
    
}

void shochu::ThreadPool::addTask(std::shared_ptr<TaskInterface> task, shochu::ThreadPool::AddTaskMethod method) {
    // if(method != shochu::ThreadPool::Discard || this->q.size() < this->taskMaxNum) {
    if(method != shochu::ThreadPool::Discard) {
        this->qMutex.lock();
        this->q.push(task);
        this->qMutex.unlock();
    }
}

void shochu::ThreadPool::setTaskMaxNum(unsigned int num) {
    this->taskMaxNum = num;
}

void shochu::ThreadPool::setThreadNum(unsigned int num) {
    this->threadNum = num;
}

void shochu::ThreadPool::workThread() {
    while(!this->isQuit) {
        this->cv.wait(this->cvMutex);
        if(this->q.size()) {
            this->qMutex.lock();
            if(this->q.size()) {
                auto task = this->q.front();
                this->q.pop();
                this->qMutex.unlock();

                task->run();
            }
            else {
                this->qMutex.unlock();
            }
        }
    }
}

void shochu::ThreadPool::checkThread() {
    struct timespec t = {.tv_sec=0, .tv_nsec= 50 * 1000};
    while(!this->isQuit) {
        if(this->q.size()) {
            this->cv.notify_one();
        }

        nanosleep(&t, nullptr);
    }
}
