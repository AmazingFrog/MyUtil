#include "messageQueue.h"

#include <thread>
#include <unistd.h>

#include "../ThreadPool/threadPool.hpp"

shochu::Event::Event(const std::string& t) : topic_(t){
}

void shochu::Event::insert(const std::string& k, const std::any& v) {
    this->data[k] = v;
}
void shochu::Event::insert(const std::string& k, std::any&& v) {
    this->data.emplace(k, std::forward<std::any>(v));
}

std::any& shochu::Event::operator[](const std::string& k) {
    return this->data[k];
}

shochu::MessageQueue* shochu::MessageQueue::getInstance() {
    static shochu::MessageQueue lib;
    return &lib;
}

shochu::MessageQueue::MessageQueue() {
    this->isQuit = false;
    this->funcID = 1;
    std::thread(&shochu::MessageQueue::handlePostMessageThread, this).detach();
}

shochu::MessageQueue::~MessageQueue() {
    this->isQuit = true;

    struct timespec t{.tv_sec=1, .tv_nsec=0};
    while(this->isQuit) {
        nanosleep(&t, nullptr);
    }
}

void shochu::MessageQueue::unregisterSubscripter(int no) {
    this->funcs.erase(no);
}

void shochu::MessageQueue::postMessage(const shochu::Event& e) {
    this->qMutex.lock();
    this->q.push(e);
    this->qMutex.unlock();
}

void shochu::MessageQueue::postMessage(shochu::Event&& e) {
    this->qMutex.lock();
    this->q.push(std::forward<shochu::Event>(e));
    this->qMutex.unlock();
}

void shochu::MessageQueue::handlePostMessageThread() {
    struct timespec t{ .tv_sec=1, .tv_nsec=0 };
    while(!this->isQuit) {
        if(!this->q.empty()) {
            this->qMutex.lock();
            auto e = this->q.front();
            this->q.pop();
            this->qMutex.unlock();

            // 处理消息
            auto& ids = this->topic2Funcs[e.topic()];
            for(auto id=ids.begin();id!=ids.end();) {
                if(this->funcs.find(*id) == this->funcs.end()) {
                    id = ids.erase(id);
                }
                else {
                    this->funcs[*id](e);
                    ++id;

                    // std::shared_ptr<CallFuncType> task(new CallFuncType(dealMeeageUseThreadPool));
                    // task->data = std::make_tuple(this->funcs[*id].get(), e);
                    // ThreadPool::getInstance()->addTask(task);
                }
            }
        }

        nanosleep(&t, nullptr);
    }

    this->isQuit = false;
}

