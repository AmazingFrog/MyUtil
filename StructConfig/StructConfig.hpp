#ifndef STRUCTCONFIG_H
#define STRUCTCONFIG_H

#include <string>
#include <memory>
#include <fstream>
#include <algorithm>
#include <unordered_map>

namespace shochu
{
namespace fromStrWapper {
    inline void fromStr(const std::string& s, bool& v) {
        v = (s == "true");
    }
    inline void fromStr(const std::string& s, int& v) {
        sscanf(s.c_str(), "%d", &v);
    }
    inline void fromStr(const std::string& s, double& v) {
        sscanf(s.c_str(), "%lf", &v);
    }
    inline void fromStr(const std::string& s, std::string& v) {
        v = s;
    }
}; //fromStrWapper
namespace toStrWapper {
    template<typename T>
    inline std::string toStr(const T& v) {
        return std::to_string(v);
    }
    template<>
    inline std::string toStr<std::string>(const std::string& s) {
        return s;
    }
}

template<typename T, typename... Args>
struct isBase{};
template<typename T>
struct isBase<T> {
    using type = std::false_type;
    static constexpr bool value = type::value;
};
template<typename T, typename thisType, typename... Args>
struct isBase<T, thisType, Args...> {
    using type = typename std::conditional<std::is_same<T, thisType>::value, std::true_type, typename isBase<T, Args...>::type >::type;
    static constexpr bool value = type::value;
};
#define IsBaseType(type) shochu::isBase<type, int, double, bool, std::string>::value

class SCNode {
public:
    SCNode() = default;
    void key(const std::string& k) { this->k = k; }
    void val(const std::string& v) { this->v = v; }
    void appendChild(const SCNode& child) { std::string idx = child.attributes.at("uid"); this->childs.emplace(idx, child); }
    void appendChild(SCNode&& child) { std::string idx = child.attributes.at("uid"); this->childs.emplace(std::move(idx), child); }
    void setAttr(const std::string& k, const std::string& v) { this->attributes[k] = v; }
    void setAttr(std::string&& k, std::string&& v) { this->attributes[k] = v; }
    void isBase(bool i) { this->isB = i; }

    typedef std::unordered_map<std::string, std::string>::const_iterator AttrIter;
    typedef std::unordered_map<std::string, shochu::SCNode>::const_iterator ChildIter;
    inline bool isBase() const { return this->isB; }
    const std::string& key() const { return this->k; }
    const std::string& val() const { return this->v; }
    AttrIter attrBegin() const { return this->attributes.cbegin(); }
    AttrIter attrEnd() const { return this->attributes.cend(); }
    ChildIter childBegin() const { return this->childs.cbegin(); }
    ChildIter childEnd() const { return this->childs.cend(); }

private:
    std::string k;
    std::string v;
    std::unordered_map<std::string, shochu::SCNode> childs;
    std::unordered_map<std::string, std::string> attributes;
    bool isB;

    template<int N, typename T>
    friend struct GetChildFromRoot;

};

class SCNodeFactoryInterface {
public:
    virtual std::string serialization(const SCNode& root) = 0;
    virtual SCNode unserialization(const std::string& str) = 0;
    virtual ~SCNodeFactoryInterface() {}

};

template<int>
struct uid {};

/**
 * 将节点的值设置到对应成员
 * cond 为是否是基本类型
*/
template<bool cond, typename T>
struct ChooseSetValueFunc {};
template<typename T>
struct ChooseSetValueFunc<true, T> {
    inline static void setValue(const SCNode& nowElem, T& val) {
        shochu::fromStrWapper::fromStr(std::string(nowElem.val()), val);
    }
};
template<typename T>
struct ChooseSetValueFunc<false, T> {
    inline static void setValue(const SCNode& nowElem, T& val) {
        val.fromSCNode(nowElem);
    }
};

/**
    从成员变成节点
    cond 为是否是基本类型
*/
template<bool cond, int N, typename outClass>
struct ChooseGetValueFunc {};
template<int N, typename outClass>
struct ChooseGetValueFunc<true, N, outClass> {
    inline static SCNode getValue(shochu::uid<N> thisUid, outClass* cls) {
        SCNode n;
        n.key(cls->getName(thisUid));
        n.setAttr("type", cls->getType(thisUid));
        n.setAttr("uid", shochu::toStrWapper::toStr(N));
        n.val(toStrWapper::toStr(cls->getValue(thisUid)));
        n.isBase(true);
        return n;
    }
};
template<int N, typename outClass>
struct ChooseGetValueFunc<false, N, outClass> {
    inline static SCNode getValue(shochu::uid<N> thisUid, outClass* cls) {
        SCNode n = cls->getValue(thisUid).toSCNode();
        n.key(cls->getName(thisUid));
        n.setAttr("type", cls->getType(thisUid));
        n.setAttr("uid", shochu::toStrWapper::toStr(N));
        n.isBase(false);
        return n;
    }
};

/**
    循环展开 添加一个成员到root
*/
template<int N, typename outClass>
struct AppendChildToRoot {
    inline static void set(outClass* cls, shochu::SCNode& root) {
        shochu::uid<N> thisUid;
        root.appendChild(std::move(cls->val2SCNode(thisUid)));
        shochu::AppendChildToRoot<N - 1, outClass>::set(cls, root);
    }
};
template<typename outClass>
struct AppendChildToRoot<0, outClass> {
    inline static void set(outClass* cls, shochu::SCNode& root) {}
}; 

/**
    循环展开 从root中获取一个节点用来初始化成员
*/
template<int N, typename outClass>
struct GetChildFromRoot {
    inline static void get(outClass* cls, const SCNode& root) {
        shochu::uid<N> thisUid;
        auto idx = shochu::toStrWapper::toStr(N);
        cls->SCNode2val(thisUid, root.childs.at(idx));
        shochu::GetChildFromRoot<N - 1, outClass>::get(cls, root);
    }
}; 
template<typename outClass>
struct GetChildFromRoot<0, outClass> {
    inline static void get(outClass* cls, const SCNode& root) {}
};

// #define COMMENT(str) /\
// /str
#define COMMENT(str) 

#define RegisterStruct_Begin(name)\
struct name {\
private:\
    using outClass = name;\
    enum { beg_menber = __COUNTER__ };

#define RegisterStruct_Menber(type, name)\
public:\
    type name;\
private:\
    enum { beg_##name = __COUNTER__ - beg_menber};\
\
    COMMENT(当前成员的一些属性)\
    constexpr auto getName(shochu::uid<beg_##name>) const {return #name;}\
    constexpr auto getType(shochu::uid<beg_##name>) const {return (IsBaseType(type))?#type:"struct:"#type;}\
    type& getValue(shochu::uid<beg_##name>) {return name;}\
    const type& getValue(shochu::uid<beg_##name>) const {return name;}\
\
    COMMENT(当前成员和SCNode的转换)\
    void SCNode2val(shochu::uid<beg_##name> thisUid, const shochu::SCNode& n) {\
        shochu::ChooseSetValueFunc<IsBaseType(type), type>::setValue(n, this->name);\
    }\
    shochu::SCNode val2SCNode(shochu::uid<beg_##name> thisUid) {\
        return shochu::ChooseGetValueFunc<IsBaseType(type), beg_##name, outClass>::getValue(thisUid, this);\
    }\
\
    friend struct shochu::ChooseSetValueFunc<IsBaseType(type), type>;\
    friend struct shochu::ChooseGetValueFunc<IsBaseType(type), beg_##name, outClass>;\
    friend struct shochu::AppendChildToRoot<beg_##name, outClass>;\
    friend struct shochu::GetChildFromRoot<beg_##name, outClass>;

#define RegisterStruct_End(name)\
private:\
    constexpr static size_t menberSize = __COUNTER__ - beg_menber - 1;\
    std::shared_ptr<shochu::SCNodeFactoryInterface> factory;\
public:\
    COMMENT(当前结构体和SCNode的转换)\
    void fromSCNode(const shochu::SCNode& root) {\
        shochu::GetChildFromRoot<menberSize, outClass>::get(this, root);\
    }\
    shochu::SCNode toSCNode() {\
        shochu::SCNode root;\
        root.key(#name);\
        root.isBase(false);\
        shochu::AppendChildToRoot<menberSize, outClass>::set(this, root);\
        return root;\
    }\
public:\
    name() : factory(new XMLFactory()) {}\
    void setFactory(std::shared_ptr<shochu::SCNodeFactoryInterface> f) { this->factory = f; }\
    void setFactory(shochu::SCNodeFactoryInterface* f) { this->factory.reset(f); }\
    void saveToFile(const char* file) {\
        std::fstream f;\
        f.open(file, std::ios::out);\
        if(f.is_open()) {\
            auto s = this->factory->serialization(this->toSCNode());\
            f.write(s.c_str(), s.size());\
            f.close();\
        }\
    }\
    void loadFromFile(const char* file) {\
        std::fstream f;\
        f.open(file, std::ios::in);\
        if(f.is_open()) {\
            std::string str;\
            while(!f.eof()) {\
                std::string s;\
                std::getline(f, s);\
                str.append(std::move(s));\
            }\
            f.close();\
            this->fromSCNode(this->factory->unserialization(str));\
        }\
    }\
}; //RegisterStruct_Begin

}; //shochu

// 默认使用tinyxml2
#include "tinyxml2.h"
class XMLFactory : public shochu::SCNodeFactoryInterface {
public:
    std::string serialization(const shochu::SCNode& root) {
        auto r = this->doc.NewElement(root.key().c_str());
        for(auto i=root.childBegin();i!=root.childEnd();++i) {
            this->serializationHelper(i->second, r);
        }
        this->doc.InsertEndChild(r);
        tinyxml2::XMLPrinter p;
        this->doc.Print(&p);
        return std::string(p.CStr());
    }

    shochu::SCNode unserialization(const std::string& str) {
        doc.Parse(str.c_str());
        auto root = doc.RootElement();

        shochu::SCNode r;
        r.key(root->Name());
        r.isBase(false);
        auto child = root->FirstChildElement();

        while(child) {
            shochu::SCNode sc = this->unserializationHelper(child);
            r.appendChild(std::move(sc));
            child = child->NextSiblingElement();
        }

        return r;
    }

private:
    tinyxml2::XMLDocument doc;

private:
    void serializationHelper(const shochu::SCNode& root, tinyxml2::XMLElement* r) {
        auto keyElem = this->doc.NewElement(root.key().c_str());

        for(auto i=root.attrBegin();i!=root.attrEnd();++i) {
            keyElem->SetAttribute(i->first.c_str(), i->second.c_str());
        }

        auto valElem = this->doc.NewText(root.val().c_str());
        if(root.isBase()) {
            keyElem->InsertEndChild(valElem);
        }
        else {
            for(auto i=root.childBegin();i!=root.childEnd();++i) {
                this->serializationHelper(i->second, keyElem);
            }
        }
        r->InsertEndChild(keyElem);
    }

    shochu::SCNode unserializationHelper(tinyxml2::XMLElement* r) {
        shochu::SCNode ret;

        ret.key(r->Name());
        auto attr = r->FirstAttribute();

        // 设置属性
        while(attr) {
            ret.setAttr(std::string(attr->Name()), std::string(attr->Value()));
            attr = attr->Next();
        }

        std::string typeStr = r->Attribute("type");
        // 如果当前节点是一个结构体
        if(typeStr.compare(0, sizeof("struct")-1, "struct") == 0) {
            ret.isBase(false);
            auto child = r->FirstChildElement();
            while(child) {
                ret.appendChild(std::move(this->unserializationHelper(child)));
                child = child->NextSiblingElement();
            }
        }
        else {
            ret.isBase(true);
            ret.val(r->FirstChild()->Value());
        }

        return ret;
    }

};

#endif // STRUCTCONFIG_H
