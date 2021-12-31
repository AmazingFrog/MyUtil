#ifndef _MESSAGE_QUEUE_H_
#define _MESSAGE_QUEUE_H_

#include <any>
#include <queue>
#include <mutex>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief 消息队列类
 * @code {.cpp}
 * class Handler {
 *     void func(shochu::Event e);
 * }
 * Handler handler;
 * MessageQueue::getInstance()->registerScripter<Handler>("topic", std::bind(&Handler::func, this, std::placeholders::_1));
 * 
 * Event e("topic");
 * e["key"] = std::any(val);
 * MessageQueue::getInstance()->postMessage(e);
 * @endcode
 */
namespace shochu {

class Event {
public:
    Event() {}
    Event(const std::string& t);

    void insert(const std::string& k, const std::any& v);
    void insert(const std::string& k, std::any&& v);

    std::any& operator[](const std::string& k);

    inline const std::string& topic() const {
        return this->topic_;
    }

private:
    std::string topic_;
    std::unordered_map<std::string, std::any> data;

};

class MessageQueue {
public:
    static MessageQueue* getInstance();

    // 返回槽编号 取消注册时使用
    int registerSubscripter(const std::string& topic, std::function<void(Event)> func) {
        this->funcs.emplace(funcID, func);
        this->topic2Funcs[topic].insert(funcID);
        ++funcID;

        return funcID - 1;
    }

    void unregisterSubscripter(int no);

    void postMessage(const Event& e);
    void postMessage(Event&& e);

private:
    MessageQueue();
    ~MessageQueue();

    int funcID;
    std::unordered_map<int, std::function<void(Event)>> funcs;

    std::unordered_map<std::string, std::unordered_set<int>> topic2Funcs;
    std::queue<Event> q;
    std::mutex qMutex;
    bool isQuit;

private:
    // 处理投递来的消息
    // 如果有消息，则通过线程执行
    void handlePostMessageThread();

};

}

#endif // _MESSAGE_QUEUE_H_
