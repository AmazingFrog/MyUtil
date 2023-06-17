#ifndef _TEST_H_
#define _TEST_H_

#include <any>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "../util.hpp"
#include "../concept.hpp"
#include "../Reflex/reflex.hpp"

namespace shochu {
using namespace shochu::util;
using namespace shochu::concepts;

using std::any;
using std::cout;
using std::vector;
using std::string;
using std::to_string;
using std::unique_ptr;

static int TEST_CASE_NUM = 1;

template<typename T>
struct getMenFnClass;

template<typename Func, typename T>
struct getMenFnClass<Func T::*>{
    using type = T;
};

template<typename Func, typename Res, typename... Args>
requires std::is_member_function_pointer_v<Func>
void test(Func testFunc, Res res, Args... args) {
    using namespace shochu;
    string errStr;
    typename getMenFnClass<Func>::type s;
    clock_t beg = clock();
    auto r = (s.*testFunc)(args...);
    clock_t end = clock();
    bool cmpRes = r == res;
    if(!cmpRes) {
        errStr = ", expect res is " + toString(res) + " and calc res is " + toString(r);
    }
    
    cout << "test case [" << TEST_CASE_NUM++ << "]: result => "
         << (cmpRes?"\033[32mok\033[0m":"\033[31mfialure\033[0m");
    cout << ", time use: " << (end-beg)*1.0/CLOCKS_PER_SEC*1000 << "ms";
    if(!cmpRes) {
        cout << errStr;
    }
    cout << "\n";
}

template<typename Func, typename Res, typename... Args>
void test(Func testFunc, Res res, Args... args) {
    using namespace shochu;
    string errStr;
    clock_t beg = clock();
    auto r = testFunc(args...);
    clock_t end = clock();
    bool cmpRes = r == res;
    if(!cmpRes) {
        errStr = ", expect res is " + toString(res) + " and calc res is " + toString(r);
    }
    
    cout << "test case [" << TEST_CASE_NUM++ << "]: result => "
         << (cmpRes?"\033[32mok\033[0m":"\033[31mfialure\033[0m");
    cout << ", time use: " << (end-beg)*1.0/CLOCKS_PER_SEC*1000 << "ms";
    if(!cmpRes) {
        cout << errStr;
    }
    cout << "\n";
}

struct TypeBase {
    any data;

    TypeBase() = default;
    template<typename T>
    TypeBase(T t) : data(t) {}

    virtual bool operator==(const TypeBase& rhs) = 0;
    virtual bool operator==(const any& rhs) = 0;

    virtual string toString(const any& o) = 0;

    virtual ~TypeBase() {}
};

template<typename T>
struct Type : public TypeBase {
    Type() = default;
    Type(T&& t) : TypeBase(std::forward<T>(t)) {}

    bool operator==(const TypeBase& rhs) override {
        return this->operator==(rhs.data);
    }
    bool operator==(const any& rhs) override {
        if(!data.has_value() && !rhs.has_value()) {
            return true;
        }
        if(data.type() != rhs.type()) {
            return false;
        }
        return std::any_cast<T>(data) == std::any_cast<T>(rhs);
    }

    string toString(const any& o) {
        try{
            return shochu::toString(any_cast<T>(o));
        }
        catch(const std::bad_any_cast&) {
            return "";
        }
    }
};
template<>
struct Type<decltype(nullptr)> : public TypeBase {
    Type() = default;
    Type(decltype(nullptr)) : TypeBase(nullptr) {}

    bool operator==(const TypeBase& rhs) override {
        return this->operator==(rhs.data);
    }
    bool operator==(const any& rhs) override {
        return data.type() == rhs.type();
    }

    string toString(const any& o) override {
        return "void";
    }
};

template<typename... Args>
vector<unique_ptr<TypeBase>> constructTypeBase(Args&&... args) {
    using shochu::operator<<;
    vector<unique_ptr<TypeBase>> r;
    (r << ... << unique_ptr<TypeBase>(new Type(std::forward<Args>(args))));
    return r;
}

void test(const vector<unique_ptr<TypeBase>>& expect, const vector<any>& res) {
    if(expect.size() != res.size()) {
        cout << "size don't match, [expect].size() == " << expect.size()
             << " and [res].size() == " << res.size() << "\n";
        return;
    }
    int n = expect.size();
    for(int i=0;i<n;++i) {
        if(*expect[i] != res[i]) {
            cout << "res [" << i << "] don't equal, expect [" << expect[i]->toString(expect[i]->data) << "]"
                 << ", and calc [" << expect[i]->toString(res[i]) << "]\n";
        }
    }
    cout << "test finish\n";
}

template<typename Cls>
void test(const vector<string>& cmds, vector<vector<any>>& args, const vector<unique_ptr<TypeBase>>& expect) {
    // auto res = runCmds<Cls>(cmds, args);
    // test(expect, res);
    if(cmds.empty()) {
        cout << "cmds is empty\n";
        return;
    }

    using re = Reflex<Cls>;
    Cls* cls = nullptr;
    int b = 0;
    int n = cmds.size();

    if(cmds[0] == re::className()) {
        auto r = re::create(args[0]);
        if(r) {
            cls = r.value();
            b = 1;
        }
        else {
            cout << "create [" << re::className() << "] failure\n";
            return;
        }
    }
    else {
        if constexpr (std::is_constructible_v<Cls>) {
            cls = new Cls();
        }
        else {
            cout << "don't find constructer [" << re::className() << "()]\n";
            return;
        }
    }
    
    clock_t beg;
    clock_t end;
    string errStr;
    for(;b<n;++b) {
        std::any res;
        bool cmpRes = false;
        beg = clock();
        if(re::runFn(*cls, cmds[b], res, args[b])) {
            end = clock();
            cmpRes = (*expect[b]) == res;

            if(!cmpRes) {
                errStr = ", expect res is " + (*expect[b]).toString((*expect[b]).data)
                       + " and calc res is " + (*expect[b]).toString(res);
            }
        }
        else {
            cout << "run function error\n";
            return;
        }

        cout << "test case [" << TEST_CASE_NUM++ << "]: result => "
             << (cmpRes?"\033[32mok\033[0m":"\033[31mfialure\033[0m");
        cout << ", time use: " << (end-beg)*1.0/CLOCKS_PER_SEC*1000 << "ms";
        cout << (cmpRes?errStr:"") << "\n";
    }

    delete cls;
}

}
#endif // _TEST_H_
