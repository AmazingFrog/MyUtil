#ifndef _REFLEX_H_
#define _REFLEX_H_

#include <any>
#include <string>
#include <memory>
#include <utility>
#include <optional>
#include <iostream>
#include <functional>
#include <unordered_map>

namespace shochu {
using std::any;
using std::tuple;
using std::string;
using std::vector;
using std::nullopt;
using std::optional;
using std::reference_wrapper;

struct ConstructorBase {
    virtual void* create(const vector<any>& args) = 0;
    virtual bool isMatch(const vector<any>& args) = 0;
};

template<typename T, typename... Args>
struct ConstructotMetadata : public ConstructorBase {
    using ArgsType = tuple<Args...>;
    using Cls = T;
    constexpr static size_t ArgsSize = sizeof...(Args);

    string name;

    ConstructotMetadata() = delete;
    ConstructotMetadata(string clsName) : name(clsName) { }

    void* create(const vector<any>& args) override {
        if(ArgsSize != args.size()) {
            return nullptr;
        }
        return doCreate<ArgsSize>(args);
    }

    bool isMatch(const vector<any>& args) override {
        if(args.size() != ArgsSize) {
            return false;
        }
        return doMatch<0>(args);
    }

    template<size_t M, typename... ArgType>
    void* doCreate(const std::vector<std::any>& args, ArgType&&... arg) {
        if constexpr (M == 0) {
            return new T(std::forward<ArgType>(arg)...);
        }
        else {
            using thisArgType = std::tuple_element_t<M-1, ArgsType>;
            try {
                thisArgType thisArg = std::any_cast<thisArgType>(args[M-1]);
                return doCreate<M-1, thisArgType, ArgType...>(args, move(thisArg), std::forward<ArgType>(arg)...);
            }
            catch(const std::bad_any_cast& e) {
                std::cerr << "create fn [" << name << "], [" << M-1 <<"] arg type dismatch\n";
                return nullptr;
            }
        }
    }

    template<size_t M>
    bool doMatch(const vector<any>& args) {
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
    virtual void run(void* cls, void* res, const std::vector<std::any>& args)  = 0;
    virtual void run(void* cls, const std::vector<std::any>& args) = 0;
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

    void run(void* cls, void* res, const std::vector<std::any>& args) override {
        if(ArgsSize != args.size()) {
            return;
        }

        doRun<ArgsSize>(cls, res, args);
    }
    void run(void* cls, const std::vector<std::any>& args) override {
        if(ArgsSize != args.size()) {
            return;
        }

        doRun<ArgsSize>(cls, nullptr, args);
    }

    template<size_t M, typename... ArgType>
    void doRun(void* cls, void* res, const std::vector<std::any>& args, ArgType&&... arg) {
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
            using thisArgType = std::tuple_element_t<M-1, ArgsType>;
            try {
                thisArgType thisArg = std::any_cast<thisArgType>(args[M-1]);
                doRun<M-1, thisArgType, ArgType...>((Cls*)cls, res, args, move(thisArg), std::forward<ArgType>(arg)...);
            }
            catch(const std::bad_any_cast& e) {
                std::cerr << "run fn [" << name << "], [" << M-1 <<"] arg type dismatch\n";
            }
        }
    }
};

struct MemberBase {
    virtual void getValue(void* cls, void* res) = 0;
    virtual void setValue(void* cls, const void* v) = 0;
    virtual void* get(void* cls) = 0;
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
    static void regConstructor(const string& name) {
        conMap().emplace_back(new ConstructotMetadata<T, Args...>(name));
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
    static void runFn(T& cls, const string& fnName, Res& res, const vector<std::any>& args) {
        auto fnIt = fnMap().find(fnName);
        if(fnIt == fnMap().end()) {
            std::cout << "don't find fn [" << fnName << "]\n";
            return;
        }
        fnIt->second->run(&cls, &res, args);
    }

    static void runFn(T& cls, const string& fnName, const vector<std::any>& args) {
        auto fnIt = fnMap().find(fnName);
        if(fnIt == fnMap().end()) {
            std::cout << "don't find fn [" << fnName << "]\n";
            return;
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

    static optional<T*> create(const vector<any>& args) {
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
    Reflex_##cls() {

#define REFLEX_FN(name, fn) shochu::Reflex<T>::regFn(name, fn);
#define REFLEX_CON(name, ...) shochu::Reflex<T>::regConstructor<__VA_ARGS__>(name);
#define REFLEX_MEM(name, mem) shochu::Reflex<T>::regMember(name, mem);

#define REFLEX_END(cls) \
    }                   \
};                      \
static Reflex_##cls __reflex_##cls;

}

#endif // _REFLEX_H_
