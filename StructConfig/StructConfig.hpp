#ifndef STRUCTCONFIG_H
#define STRUCTCONFIG_H

#include <string>

#include "tinyxml2.h"

namespace shochu
{
namespace toStrWapper {
    constexpr unsigned int stringLeng = 32;
    inline std::string toStr(bool i) {
        return std::string(i ? "true" : "false");
    }
    inline std::string toStr(int i) {
        char t[stringLeng] = { 0 };
        sprintf(t, "%d", i);
        return std::string(t);
    }
    inline std::string toStr(double i) {
        char t[stringLeng] = { 0 };
        sprintf(t, "%lf", i);
        return std::string(t);
    }
    inline std::string toStr(float i) {
        char t[stringLeng] = { 0 };
        sprintf(t, "%lf", i);
        return std::string(t);
    }
    inline std::string toStr(std::string i) {
        return i;
    }
}; //toStrWapper
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
    inline void fromStr(const std::string& s, float& v) {
        sscanf(s.c_str(), "%f", &v);
    }
    inline void fromStr(const std::string& s, std::string& v) {
        v = s;
    }
}; //fromStrWapper

template<int>
struct uid {};

template<typename T,typename... Args>
struct isBase{};
template<typename T>
struct isBase<T> {
    using type = std::false_type;
    static constexpr bool value = type::value;
};
template<typename T,typename thisType,typename... Args>
struct isBase<T,thisType,Args...> {
    using type = typename std::conditional<std::is_same<T, thisType>::value, std::true_type, typename isBase<T, Args...>::type >::type;
    static constexpr bool value = type::value;
};
#define IsBaseType(type) isBase<type,int,double,float,bool,std::string>::value

/**
    从 nowElem 构造 val
*/
template<bool cond,typename T>
struct ChooseSetValueFunc {};
template<typename T>
struct ChooseSetValueFunc<true,T> {
    inline static void setValue(const tinyxml2::XMLElement* nowElem,T& val) {
        fromStrWapper::fromStr(std::string(nowElem->GetText()), val);
    }
};
template<typename T>
struct ChooseSetValueFunc<false,T> {
    inline static void setValue(const tinyxml2::XMLElement* nowElem, T& val) {
        val.fromXMLElement(nowElem);
    }
};

/**
    从 doc 创建xmlelement，xmlelement由 cls 和 thisUid 创建
*/
template<bool cond, int N,typename outClass>
struct ChooseGetValueFunc {};
template<int N, typename outClass>
struct ChooseGetValueFunc<true, N, outClass> {
    inline static tinyxml2::XMLElement* getXMLFromVal(tinyxml2::XMLDocument* doc,uid<N> thisUid,outClass* cls) {
        tinyxml2::XMLElement* nowElem = doc->NewElement(cls->getName(thisUid));
        nowElem->SetAttribute("type", cls->getType(thisUid));

        tinyxml2::XMLText* text = doc->NewText(toStrWapper::toStr(cls->getValue(thisUid)).c_str());
        nowElem->InsertEndChild(text);

        return nowElem;
    }
};
template<int N, typename outClass>
struct ChooseGetValueFunc<false, N, outClass> {
    inline static tinyxml2::XMLElement* getXMLFromVal(tinyxml2::XMLDocument* doc, uid<N> thisUid, outClass* cls) {
        return cls->getValue(thisUid).toXMLElement(doc);
    }
};

/**
    循环展开添加一个成员到root
*/
template<int N,typename outClass>
struct AppendChildToRoot {
    inline static void set(outClass* cls,tinyxml2::XMLDocument* doc, tinyxml2::XMLElement* root) {
        uid<N> thisUid;
        tinyxml2::XMLElement* nowElem = cls->getXMLElementFromValue(thisUid, doc);
        nowElem->SetAttribute("type", cls->getType(thisUid));
        nowElem->SetAttribute("uid", N);
        root->InsertFirstChild(nowElem);
        AppendChildToRoot<N - 1, outClass>::set(cls, doc, root);
    }
};
template<typename outClass>
struct AppendChildToRoot<0, outClass> {
    inline static void set(outClass* cls,tinyxml2::XMLDocument* doc, tinyxml2::XMLElement* root) {}
}; 

/**
    循环展开从root中获取一个element用来初始化成员
*/
template<int N,typename outClass>
struct GetChildFromRoot {
    inline static void get(outClass* cls, const tinyxml2::XMLElement* root){
        uid<N> thisUid;
        const tinyxml2::XMLElement* nowElem = root->FirstChildElement(cls->getName(thisUid));
        cls->setValueByXMLElement(thisUid, nowElem);
        GetChildFromRoot<N - 1, outClass>::get(cls,root); 
    }
}; 
template<typename outClass>
struct GetChildFromRoot<0, outClass> {
    inline static void get(outClass* cls, const tinyxml2::XMLElement* root) {}
};

#define RegisterStruct_Begin(name)\
struct name{\
private:\
    using outClass = name;\
    enum { beg_menber = __COUNTER__ };\
    tinyxml2::XMLDocument doc;

#define RegisterStruct_Menber(type,name)\
public:\
    type name;\
private:\
    enum { beg_##name = __COUNTER__ - beg_menber};\
\
    constexpr auto getName(uid<beg_##name>){return (IsBaseType(type))?#name:#type;}\
    constexpr auto getType(uid<beg_##name>) {return (IsBaseType(type))?#type:"struct";}\
    auto& getValue(uid<beg_##name>) {return name;}\
    const auto& getValue(uid<beg_##name>) const {return name;}\
\
    void setValueByXMLElement(uid<beg_##name> thisUid,const tinyxml2::XMLElement* nowElem){\
        ChooseSetValueFunc<IsBaseType(type),type>::setValue(nowElem,this->name);\
    }\
    tinyxml2::XMLElement* getXMLElementFromValue(uid<beg_##name> thisUid,tinyxml2::XMLDocument* doc){\
        return ChooseGetValueFunc<IsBaseType(type),(beg_##name),outClass>::getXMLFromVal(doc,thisUid,this);\
    }\
\
    friend struct AppendChildToRoot<beg_##name,outClass>;\
    friend struct GetChildFromRoot<beg_##name,outClass>;\
    friend struct ChooseSetValueFunc<IsBaseType(type),type>;\
    friend struct ChooseGetValueFunc<IsBaseType(type),beg_##name,outClass>;

#define RegisterStruct_End(name)\
private:\
    const static size_t menberSize = __COUNTER__ - beg_menber - 1;\
public:\
    name() = default;\
    name(const name& rhs){\
        if(this == &rhs){\
            return;\
        }\
        outClass* r = const_cast<outClass*>(&rhs);\
        r->doc.DeepCopy(&(this->doc));\
        this->fromXMLElement(this->doc.FirstChild()->ToElement());\
    }\
    name& operator=(const name& rhs){\
        if(this == &rhs){\
            return *this;\
        }\
        outClass* r = const_cast<outClass*>(&rhs);\
        r->doc.DeepCopy(&(this->doc));\
        this->fromXMLElement(this->doc.FirstChild()->ToElement());\
        return *this;\
    }\
    inline tinyxml2::XMLElement* toXMLElement(tinyxml2::XMLDocument* document){\
        tinyxml2::XMLElement* root = document->NewElement(#name);\
        AppendChildToRoot<menberSize,outClass>::set(this,document,root);\
        return root;\
}\
    inline void fromXMLElement(const tinyxml2::XMLElement* root){\
        GetChildFromRoot<outClass::menberSize,outClass>::get(this,root);\
    }\
\
    void saveToFile(const char* filename){\
        this->doc.Clear();\
        tinyxml2::XMLElement* root = this->toXMLElement(&(this->doc));\
        this->doc.InsertEndChild(root);\
        this->doc.SaveFile(filename);\
    }\
    void saveToFile(const std::string& f){\
        this->saveToFile(f.c_str());\
    }\
    void loadFromFile(const char* filename){\
        this->doc.Clear();\
        this->doc.LoadFile(filename);\
        this->fromXMLElement(this->doc.FirstChild()->ToElement());\
    }\
    void loadFromFile(const std::string& f){\
        this->loadFromFile(f.c_str());\
    }\
}; //RegisterStruct_Begin

}; //shochu

#endif // !STRUCTCONFIG_H
