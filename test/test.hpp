#ifndef _TEST_H_
#define _TEST_H_

#include <string>
#include <vector>
#include <variant>

#include "../Reflex/reflex.hpp"

namespace shochu {
using std::string;
using std::vector;
using std::tuple;
using std::cout;
using std::to_string;

static int TEST_CASE_NUM = 1;

string to_string(const string& s) {
    return "\"" + s + "\"";
}

string to_string(const char* s) {
    return "\"" + string(s) + "\"";
}

string to_string(bool b) {
    return b?"true":"false";
}

template<size_t N, typename... Args>
struct Tuple2String {
    static void to(const tuple<Args...>& a, string& s) {
        if constexpr (N != sizeof...(Args)) {
            s.append(to_string(std::get<N>(a)));
            s.append(", ");
            Tuple2String<N+1, Args...>::to(a, s);
        }
    }
};

template<typename... Args>
string to_string(const tuple<Args...>& a) {
    string s("(");
    Tuple2String<0, Args...>::to(a, s);
    s.pop_back();
    s.pop_back();
    s.push_back(')');
    return s;
}

template<typename Res, typename FuncRet>
concept hasOperatorEqual = requires(Res r, FuncRet f) {
    r == f;
};

template<typename T>
concept hasToString = requires(T a) {
    { to_string(a) } -> std::same_as<string>;
};

template<typename T>
concept hasIter = requires(T a) {
    std::begin(a);
    std::end(a);
};

template<typename T>
requires hasToString<T>
string printRes(const T& p) {
    return to_string(p);
}

template<typename T>
requires hasIter<T>
string printRes(const T& p) {
    if(std::begin(p) == std::end(p)) {
        return "[]";
    }
    string s;
    s.push_back('[');
    for(auto&& i : p) {
        s.append(printRes(i));
        s.append(", ");
    }
    s.pop_back();
    s.pop_back();
    s.push_back(']');
    return s;
}

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
        errStr = ", expect res is " + printRes(res) + " and calc res is " + printRes(r);
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
        errStr = ", expect res is " + printRes(res) + " and calc res is " + printRes(r);
    }
    
    cout << "test case [" << TEST_CASE_NUM++ << "]: result => "
         << (cmpRes?"\033[32mok\033[0m":"\033[31mfialure\033[0m");
    cout << ", time use: " << (end-beg)*1.0/CLOCKS_PER_SEC*1000 << "ms";
    if(!cmpRes) {
        cout << errStr;
    }
    cout << "\n";
}

struct typeHolder {
    virtual bool operator==(void* rhs) = 0;
};

template<typename T>
struct typeHolderImpl : public typeHolder {
    T val;
    typeHolderImpl() = delete;
    typeHolderImpl(T v) : val(v) {}

    bool operator==(void* rhs) override {
        return val == *(T*)rhs;
    }
};

// template<typename T>
// void test(T& cls, const vector<string>& cmds, const vector<std::any>& args, const vector<variant<int, double>>& res) {
//     if(cmds.size() != args.size()) {
//         std::cerr << "cmds and args dismatch\n";
//         return;
//     }

//     Reflex<T> re;
//     for(int i=0;i<cmds.size();++i) {
//         re.getFn(cmds[i])->run(&cls, nullptr, args[i]);
//     }
// }


}
#endif // _TEST_H_
