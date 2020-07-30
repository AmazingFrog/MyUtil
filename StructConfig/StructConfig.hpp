#ifndef STRUCTCONFIG_H
#define STRUCTCONFIG_H

#include <string>

#include "tinyxml2.h"

namespace shochu
{
template<int>
struct uid {};

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

#define RegisterStruct_Begin(name)\
struct name{\
private:\
    using outClass = name;\
    enum { beg_menber = __COUNTER__ };\
    tinyxml2::XMLDocument doc;\
    template<int N>\
    class LoopAppendChild;\
    template<int N>\
    class LoopGetChild;

#define RegisterStruct_Menber(type,name)\
public:\
    type name;\
private:\
    enum { beg_##name = __COUNTER__ - beg_menber};\
    constexpr auto getName(uid<beg_##name>){return #name;}\
    constexpr auto getType(uid<beg_##name>) {return #type;}\
    auto& getValue(uid<beg_##name>) {return name;}\
    const auto& getValue(uid<beg_##name>) const {return name;}\
    void setValue(uid<beg_##name>,const std::string& valStr){\
        fromStrWapper::fromStr(valStr,this->name);\
    }\
    friend class LoopAppendChild<beg_##name>;\
    friend class LoopGetChild<beg_##name>;

#define RegisterStruct_End(name)\
private:\
    const static size_t menberSize = __COUNTER__ - 1;\
    template<int N>\
    class LoopAppendChild {\
        public:\
        inline tinyxml2::XMLElement* toXMLElem(outClass* cls,tinyxml2::XMLDocument* doc,tinyxml2::XMLElement* root) {\
            uid<N> thisUid;\
            tinyxml2::XMLElement* ret = doc->NewElement(cls->getName(thisUid));\
            ret->SetAttribute("type",cls->getType(thisUid));\
            ret->SetAttribute("uid",N);\
            tinyxml2::XMLText* text = doc->NewText(toStrWapper::toStr(cls->getValue(thisUid)).c_str());\
            ret->InsertFirstChild(text);\
            root->InsertFirstChild(ret);\
            LoopAppendChild<N-1> nextSibling;\
            nextSibling.toXMLElem(cls,doc,root);\
            return ret;\
        }\
    };\
    template<>\
    class LoopAppendChild<0>{\
        public:\
        inline tinyxml2::XMLElement* toXMLElem(outClass* cls,tinyxml2::XMLDocument* doc,tinyxml2::XMLElement* root) {\
            return nullptr;\
        }\
    };\
    template<int N>\
    class LoopGetChild{\
        public:\
        inline void getChdildValue(outClass* cls,const tinyxml2::XMLElement* root){\
            uid<N> thisUid; \
            cls->setValue(thisUid,std::string(root->FirstChildElement(cls->getName(thisUid))->GetText()));\
            LoopGetChild<N-1>().getChdildValue(cls,root);\
        }\
    };\
    template<>\
    class LoopGetChild<0>{\
        public:\
        inline void getChdildValue(outClass* cls,const tinyxml2::XMLElement* root){\
            return;\
        }\
    };\
public:\
    name() = default;\
    name(const name& rhs){\
        if(this == &rhs){\
            return;\
        }\
        outClass* r = const_cast<outClass*>(&rhs);\
        this->doc.DeepClone(&(r->doc));\
    }\
    name& operator=(const name& rhs){\
        if(this == &rhs){\
            return *this;\
        }\
        outClass* r = const_cast<outClass*>(&rhs);\
        this->doc.DeepClone(&(r->doc));\
        return *this;\
    }\
    void saveToFile(const char* filename){\
        LoopAppendChild<menberSize> t;\
        this->doc.Clear();\
        tinyxml2::XMLElement* root = doc.NewElement(#name);\
        t.toXMLElem(this,&doc,root);\
        this->doc.InsertFirstChild(root);\
        this->doc.SaveFile(filename);\
    }\
    void saveToFile(const std::string& f){\
        this->saveToFile(f.c_str());\
    }\
    void loadFromFile(const char* filename){\
        this->doc.Clear();\
        this->doc.LoadFile(filename);\
        LoopGetChild<outClass::menberSize> t;\
        t.getChdildValue(this,doc.FirstChild()->ToElement());\
    }\
    void loadFromFile(const std::string& f){\
        this->loadFromFile(f.c_str());\
    }\
}; //RegisterStruct_Begin

}; //shochu

#endif // !STRUCTCONFIG_H
