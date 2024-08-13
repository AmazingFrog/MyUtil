#ifndef _REFLEX_H_
#define _REFLEX_H_

#include <any>
#include <array>
#include <string>
#include <memory>
#include <utility>
#include <optional>
#include <iostream>
#include <functional>
#include <string_view>
#include <unordered_map>

// 动态反射
namespace shochu {
using std::any;
using std::tuple;
using std::string;
using std::vector;
using std::nullopt;
using std::optional;
using std::reference_wrapper;

struct ConstructorBase {
    virtual void* create(vector<any>& args) = 0;
    virtual bool isMatch(vector<any>& args) = 0;

    virtual ~ConstructorBase() {}
};

template<typename T, typename... Args>
struct ConstructotMetadata : public ConstructorBase {
    using ArgsType = tuple<Args...>;
    using Cls = T;
    constexpr static size_t ArgsSize = sizeof...(Args);

    string name;

    ConstructotMetadata() = delete;
    ConstructotMetadata(string clsName) : name(clsName) { }

    void* create(vector<any>& args) override {
        if(ArgsSize != args.size()) {
            return nullptr;
        }
        return doCreate<ArgsSize>(args);
    }

    bool isMatch(vector<any>& args) override {
        if(args.size() != ArgsSize) {
            return false;
        }
        return doMatch<0>(args);
    }

    template<size_t M, typename... ArgType>
    void* doCreate(std::vector<std::any>& args, ArgType&&... arg) {
        if constexpr (M == 0) {
            return new T(std::forward<ArgType>(arg)...);
        }
        else {
            try {
                using thisArgType = std::tuple_element_t<M-1, ArgsType>;
                return doCreate<M-1, thisArgType, ArgType...>(args, std::any_cast<thisArgType>(args[M-1]), std::forward<ArgType>(arg)...);
            }
            catch(const std::bad_any_cast& e) {
                std::cerr << "create fn [" << name << "], [" << M-1 << "] arg type mismatch\n";
                return nullptr;
            }
        }
    }

    template<size_t M>
    bool doMatch(vector<any>& args) {
        if constexpr (M == ArgsSize) {
            return true;
        }
        else {
            using thisArgType = std::tuple_element_t<M, ArgsType>;
            try {
                thisArgType thisArg = std::any_cast<thisArgType>(args[M]);
                return doMatch<M+1>(args);
            }
            catch(const std::bad_any_cast& e) {
                return false;
            }
        }
    }

};

struct FuncBase {
    virtual bool run(void* cls, void* res, std::vector<std::any>& args)  = 0;
    virtual bool run(void* cls, std::any& res, std::vector<std::any>& args) = 0;
    virtual bool run(void* cls, std::vector<std::any>& args) = 0;

    virtual ~FuncBase() {}
};

template<typename Cls, typename Res, typename... Args>
struct FuncMetadata : public FuncBase {
    using R = Res;
    using FuncType = std::function<Res(Cls*, Args...)>;
    using ArgsType = tuple<Args...>;
    using Class = Cls;
    const static size_t ArgsSize = sizeof...(Args);

    string name;
    FuncType fn;

    FuncMetadata() = delete;
    FuncMetadata(string fnName, Res (Cls::*f)(Args...)) : name(fnName), fn(f) {}

    bool run(void* cls, void* res, std::vector<std::any>& args) override {
        if(ArgsSize != args.size()) {
            return false;
        }

        return doRun<ArgsSize>(cls, res, args);
    }
    bool run(void* cls, std::any& res, std::vector<std::any>& args) override {
        if(ArgsSize != args.size()) {
            return false;
        }

        if constexpr (std::is_same_v<Res, void>) {
            res = nullptr;
            return run(cls, nullptr, args);
        }
        else {
            Res r;
            bool b = run(cls, &r, args);
            res = b?r:res;
            return b;
        }
        
    }
    bool run(void* cls, std::vector<std::any>& args) override {
        return run(cls, nullptr, args);
    }

    template<size_t M, typename... ArgType>
    bool doRun(void* cls, void* res, std::vector<std::any>& args, ArgType&&... arg) {
        if constexpr (M == 0) {
            if constexpr (std::is_same_v<Res, void>) {
                fn((Cls*)cls, std::forward<ArgType>(arg)...);
            }
            else {
                if(res == nullptr) {
                    fn((Cls*)cls, std::forward<ArgType>(arg)...);
                }
                else {
                    *(Res*)res = fn((Cls*)cls, std::forward<ArgType>(arg)...);
                }
            }
        }
        else {
            try {
                using thisArgType = std::tuple_element_t<M-1, ArgsType>;
                doRun<M-1, thisArgType, ArgType...>((Cls*)cls, res, args, std::any_cast<thisArgType>(args[M-1]), std::forward<ArgType>(arg)...);
            }
            catch(const std::bad_any_cast& e) {
                std::cerr << "run fn [" << name << "], [" << M-1 <<"] arg type mismatch\n";
                return false;
            }
        }

        return true;
    }
};

struct MemberBase {
    virtual void getValue(void* cls, void* res) = 0;
    virtual void setValue(void* cls, const void* v) = 0;
    virtual void* get(void* cls) = 0;

    virtual ~MemberBase() {}
};

template<typename Cls, typename T>
struct MemberMetadata : public MemberBase {
    using Ty = T;
    
    Ty Cls::* p;
    string name;
    MemberMetadata(string memName, T (Cls::* pmem)) : p(pmem), name(memName) {}

    void getValue(void* cls, void* res) override {
        if(res != nullptr) {
            *(T*)res = ((Cls*)cls)->*p;
        }
    }
    void setValue(void* cls, const void* v) override {
        if(v != nullptr) {
            ((Cls*)cls)->*p = *(const T*)v;
        }
    }
    void* get(void* cls) {
        return &(((Cls*)cls)->*p);
    }
};

template<typename T>
struct Reflex {
    using FnMapType = std::unordered_map<string, std::unique_ptr<FuncBase>>;
    using MemMapType = std::unordered_map<string, std::unique_ptr<MemberBase>>;
    using ConstructorMapType = std::vector<std::unique_ptr<ConstructorBase>>;

    static string& className() {
        static string n;
        return n;
    }
    static FnMapType& fnMap() {
        static FnMapType f;
        return f;
    }
    static MemMapType& memberMap() {
        static MemMapType m;
        return m;
    }
    static ConstructorMapType& conMap() {
        static ConstructorMapType m;
        return m;
    }

    template<typename Res, typename... Args>
    static void regFn(const string& fnName, Res(T::*f)(Args...)) {
        fnMap().emplace(fnName, new FuncMetadata(fnName, f));
    }

    template<typename Mem>
    static void regMember(const string& mem, Mem T::*p) {
        memberMap().emplace(mem, new MemberMetadata(mem, p));
    }

    template<typename... Args>
    static void regConstructor() {
        conMap().emplace_back(new ConstructotMetadata<T, Args...>(className()));
    }

    static optional<FuncBase*> getFn(const string& fnName) {
        auto fnIt = fnMap().find(fnName);
        if(fnIt == fnMap().end()) {
            std::cerr << "don't find fn [" << fnName << "]\n";
            return nullopt;
        }

        return fnIt->second.get();
    }

    template<typename Res>
    requires (!std::same_as<Res, any>)
    static bool runFn(T& cls, const string& fnName, Res& res, vector<std::any>& args) {
        auto fnIt = fnMap().find(fnName);
        if(fnIt == fnMap().end()) {
            std::cout << "don't find fn [" << fnName << "]\n";
            return false;
        }
        return fnIt->second->run(&cls, &res, args);
    }

    static bool runFn(T& cls, const string& fnName, std::any& res, vector<std::any>& args) {
        auto fnIt = fnMap().find(fnName);
        if(fnIt == fnMap().end()) {
            std::cout << "don't find fn [" << fnName << "]\n";
            return false;
        }
        return fnIt->second->run(&cls, res, args);
    }

    static bool runFn(T& cls, const string& fnName, vector<std::any>& args) {
        auto fnIt = fnMap().find(fnName);
        if(fnIt == fnMap().end()) {
            std::cout << "don't find fn [" << fnName << "]\n";
            return false;
        }
        fnIt->second->run(&cls, args);
    }

    template<typename R>
    static optional<reference_wrapper<R>> getMenber(T& cls, const string& memName) {
        auto mem = memberMap().find(memName);
        if(mem == memberMap().end()) {
            std::cerr << "don't find member [" << memName << "]\n";
            return nullopt;
        }
        return *((R*)mem->second->get(&cls));
    }

    template<typename... Args>
    static T create(Args... args) {
        return T(args...);
    }

    static optional<T*> create(vector<any>& args) {
        for(auto& c : conMap()) {
            if(c->isMatch(args)) {
                auto r = (T*)c->create(args);
                return r==nullptr?nullopt:optional{r};
            }
        }
        return nullopt;
    }

    static void clear() {
        fnMap().clear();
        memberMap().clear();
        conMap().clear();
    }

};

#define REFLEX_BEG(cls)     \
struct Reflex_##cls {       \
    using T = cls;          \
    Reflex_##cls() {        \
        shochu::Reflex<T>::className() = #cls;

#define REFLEX_FN(name, fn) shochu::Reflex<T>::regFn(name, fn);
#define REFLEX_CON(...) shochu::Reflex<T>::regConstructor<__VA_ARGS__>();
#define REFLEX_MEM(name, mem) shochu::Reflex<T>::regMember(name, mem);

#define REFLEX_END(cls) \
    }                   \
};                      \
static Reflex_##cls __reflex_##cls;

}

// 静态反射
namespace shochu {
namespace detail {
#define tostring(...) #__VA_ARGS__

template<char... c>
struct const_string {
    constexpr static int size{ sizeof...(c) };
    constexpr static char str[size]{ c... };
};

template<char... c>
struct count_field_number {
    template<size_t i, size_t n>
    constexpr static auto count() {
        if constexpr (i == n) {
            return 0;
        }
        else {
            return (const_string<c...>::str[i] == ',') + count<i+1, n>();
        }
    }
    constexpr static auto val{ 1 + count<0, sizeof...(c)>() };
};

template<size_t i, size_t n, typename F, char... c>
constexpr auto str_2_field_arr(F fn) {
    if constexpr (i == n) {
        std::array<std::string_view, count_field_number<c...>::val> arr;
        int idx{ 0 };
        int first{ 0 };
        for(int j{0};j<const_string<c...>::size;++j) {
            if(const_string<c...>::str[j] == ',') {
                arr[idx++] = { &const_string<c...>::str[first], &const_string<c...>::str[j] };
                first = j + 1;
            }
        }
        arr[idx] = { &const_string<c...>::str[first], &const_string<c...>::str[const_string<c...>::size] };
        return arr;
    }
    else if constexpr (fn(i) == ' ') {
        return str_2_field_arr<i+1, n, F, c...>(fn);
    }
    else {
        return str_2_field_arr<i+1, n, F, c..., fn(i)>(fn);
    }
}

#define expand1(...) __VA_ARGS__
#define expand2(...) expand1(expand1(expand1(expand1(__VA_ARGS__))))
#define expand3(...) expand2(expand2(expand2(expand2(__VA_ARGS__))))
#define expand4(...) expand3(expand3(expand3(expand3(__VA_ARGS__))))
#define  expand(...) expand4(expand4(expand4(expand4(__VA_ARGS__))))
#define parens ()
#define expand_type(T, ...) __VA_OPT__(expand(expand_type_helper(T, __VA_ARGS__)))
#define expand_type_helper(T, f, ...) decltype(T::f) T::*, __VA_OPT__(expand_type_helper_again parens (T, __VA_ARGS__))
#define expand_type_helper_again() expand_type_helper
#define expand_var(T, ...) __VA_OPT__(expand(expand_var_helper(T, __VA_ARGS__)))
#define expand_var_helper(T, f, ...) &T::f, __VA_OPT__(expand_var_helper_again parens (T, __VA_ARGS__))
#define expand_var_helper_again() expand_var_helper

#define build_field_arr(str) shochu::detail::str_2_field_arr<0, sizeof(str)>([](size_t i) constexpr { return str[i]; })
} // detail

#define make_meta_info(T, ...) \
template<typename Ty> \
struct meta_info_t { \
    constexpr static auto field_name{ build_field_arr(#__VA_ARGS__) }; \
    constexpr static tuple field_type{ expand_var(T, __VA_ARGS__) }; \
    template<size_t i> \
    using get_type = remove_cvref_t<remove_pointer_t<tuple_element_t<i, decltype(field_type)>>>; \
    template<size_t i> \
    constexpr string_view get_field_name() { return field_name[i]; }; \
    template<size_t i, typename T> \
    static void write(Ty& v, const T& val) { \
        v.*(get<i>(field_type)) = val; \
    } \
    template<size_t i> \
    static auto read(Ty& v) { \
        return v.*(get<i>(field_type)); \
    } \
};
}

#endif // _REFLEX_H_
