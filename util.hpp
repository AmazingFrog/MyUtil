#ifndef _UTIL_H_
#define _UTIL_H_

#include <any>
#include <tuple>
#include <string>
#include <iostream>

#include "concept.hpp"
#include "Reflex/reflex.hpp"

namespace shochu {
namespace util {
using namespace shochu::concepts;

using std::any;
using std::cout;
using std::pair;
using std::tuple;
using std::string;
using std::to_string;

// 声明
template<typename T>
requires hasToString<T>
string toString(const T& p);

template<typename T>
requires (hasIter<T> && !std::same_as<T, string>)
string toString(const T& p);

// ---

template<>
string to_string<string>(const string& s) {
    return "\"" + s + "\"";
}

template<>
string to_string<const char*>(const char* const& s) {
    return "\"" + string(s) + "\"";
}

template<>
string to_string<bool>(const bool& b) {
    return b?"true":"false";
}

template<>
string to_string<nullptr_t>(const nullptr_t&) {
    return "null";
}

template<typename T1, typename T2>
string to_string(const pair<T1, T2>& a) {
    string s("(");
    s.append(toString(a.first));
    s.append(", ");
    s.append(toString(a.second));
    s.push_back(')');
    return s;
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

template<typename T>
requires hasToString<T>
string toString(const T& p) {
    return to_string(p);
}

template<typename T>
requires (hasIter<T> && !std::same_as<T, string>)
string toString(const T& p) {
    if(std::begin(p) == std::end(p)) {
        return "[]";
    }
    string s;
    s.push_back('[');
    for(auto&& i : p) {
        s.append(toString(i));
        s.append(", ");
    }
    s.pop_back();
    s.pop_back();
    s.push_back(']');
    return s;
}

struct MultiArray {
    template<typename T, typename... Args>
    constexpr static auto createV(T&& v, int dim, Args... o) {
        return std::vector<decltype((MultiArray::createV<T>(std::forward<T>(v), o...)))>
                    (dim, MultiArray::createV<T>(std::forward<T>(v), o...));
    }
    template<typename T>
    constexpr static std::vector<T> createV(T&& v, int dim) {
        return std::vector<T>(dim, std::forward<T>(v));
    }
    template<typename T, typename... Args>
    constexpr static auto create(int dim, Args... o) {
        return std::vector<decltype((MultiArray::create<T>(o...)))>(dim, MultiArray::create<T>(o...));
    }
    template<typename T>
    constexpr static std::vector<T> create(int dim) {
        return std::vector<T>(dim);
    }
};

template<typename Cls>
vector<any> runCmds(const vector<string>& cmds, vector<vector<any>>& args) {
    if(cmds.size() != args.size() || cmds.empty()) {
        return {};
    }
    using re = Reflex<Cls>;

    Cls* cls = nullptr;
    int b = 0;
    int n = cmds.size();

    vector<any> res;
    if(cmds[0] == re::className()) {
        auto r = re::create(args[0]);
        if(r) {
            cls = r.value();
            b = 1;
            res.emplace_back(any{nullptr});
        }
        else {
            cout << "create [" << re::className() << "] failure\n";
            return {};
        }
    }
    else {
        if constexpr (std::is_constructible_v<Cls>) {
            cls = new Cls();
        }
        else {
            cout << "don't find constructer [" << re::className() << "()]\n";
            return {};
        }
    }

    for(int i=b;i<n;++i) {
        any r;
        if(re::runFn(*cls, cmds[i], r, args[i])) {
            res.emplace_back(r);
        }
        else {
            res.clear();
            break;
        }
    }

    delete cls;
    return res;
}

template<typename T>
vector<T>& operator<<(vector<T>& th, T&& v) {
    th.emplace_back(std::forward<T>(v));
    return th;
}

} // util
} // shochu

#endif // _UTIL_H_
