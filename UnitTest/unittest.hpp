#pragma once
#ifndef UNITTEST_H
#define UNITTEST_H

#include <cmath>
#include <string>
#include <vector>
#include <iostream>

namespace shochu{

enum TestMode
{
    Expect,
    Assert
};

enum TestMethod
{
    TestMethod_EQ,
    TestMethod_NE,
    TestMethod_GT,
    TestMethod_GE,
    TestMethod_LT,
    TestMethod_LE
};
static const char* TestMethodStr[] = 
{
    " == ",
    " != ",
    " > ",
    " >= ",
    " < ",
    " <= "
};

//测试用例的基本接口
class UnitTestCaseInterface
{
public:
    virtual int run() = 0;
    virtual std::string getTestString() = 0;
    virtual ~UnitTestCaseInterface(){}
};

//测试用例的具体实现
template<typename T>
class UnitTestCase : public UnitTestCaseInterface
{
public:
    UnitTestCase(){}
    UnitTestCase(const T& lvalue,const  T& rvalue, TestMethod method, TestMode mode, const char *lvalStr, const char *rvalStr, const char* fileName, const unsigned long fileLine){
        this->setTestCase(lvalue,rvalue,method,mode,lvalStr,rvalStr,fileName,fileLine);
    }
    
    void setTestCase(const T& lvalue, const T& rvalue, TestMethod method, TestMode mode, const char *lvalStr, const char *rvalStr, const char* fileName, const unsigned long fileLine){
        this->lval = lvalue;
        this->rval = rvalue;
        this->method = method;
        this->mode = mode;
        
        char str[1024];
        sprintf(str,"(%s:%lu) %s [%s%s%s]",fileName,fileLine,mode==TestMode::Expect?"expect":"assert",lvalStr,TestMethodStr[method],rvalStr);
        this->testString = str;
    }
    
    //0 成功
    //1 失败
    //-1 退出
    int run(){
        bool testRes = false;
        
        switch(this->method)
        {
        case TestMethod::TestMethod_EQ:
            testRes = this->isEQ(this->lval,this->rval);
            break;
        case TestMethod::TestMethod_NE:
            testRes = this->isNE(this->lval,this->rval);
            break;
        case TestMethod::TestMethod_GT:
            testRes = this->isGT(this->lval,this->rval);
            break;
        case TestMethod::TestMethod_GE:
            testRes = this->isGE(this->lval,this->rval);
            break;
        case TestMethod::TestMethod_LT:
            testRes = this->isLT(this->lval,this->rval);
            break;
        case TestMethod::TestMethod_LE:
            testRes = this->isLE(this->lval,this->rval);
            break;
        }
        
        int ret = 0;
        if(this->mode==TestMode::Assert){
            ret = testRes?0:-1;
        }
        else{
            ret = !testRes;
        }
        
        return ret;
    }
    std::string getTestString(){
        return this->testString;
    }
    
protected:
    virtual bool isEQ(const T& l,const T& r){return l == r;}
    virtual bool isGT(const T& l,const T& r){return l > r;}
    
    bool isGE(const T& l,const T& r){return isGT(l,r) || isEQ(l,r);}
    bool isNE(const T& l,const T& r){return !isEQ(l,r);}
    bool isLT(const T& l,const T& r){return !isGE(l,r);}
    bool isLE(const T& l,const T& r){return !isGT(l,r);}
private:
    T lval;
    T rval;
    TestMethod method;
    TestMode mode;
    std::string testString;
};

//不同类型的测试用例的特化函数
template<>
bool UnitTestCase<double>::isEQ(const double &l, const double &r){
    return std::abs(l-r)<1e-8;
}
template<>
bool UnitTestCase<double>::isGT(const double &l, const double &r){
    return (l-r)>1e-8;
}

template<>
bool UnitTestCase<float>::isEQ(const float &l, const float &r){
    return std::abs(l-r)<1e-6;
}
template<>
bool UnitTestCase<float>::isGT(const float &l, const float &r){
    return (l-r)>1e-6;
}

//单元测试实例
class UnitTest
{
public:
    static void run(){
        int all = (int)UnitTest::getInstance().size();
        int runNumber = 0;
        int success = 0;
        
        for(auto i=UnitTest::getInstance().begin();i!=UnitTest::getInstance().end();++i,++runNumber){
            int runStatus = (*i)->run();
            std::cout << (*i)->getTestString() << " and result is " << (runStatus==0?"[success]":"[fail]") << std::endl;
            runStatus==0?++success:0;
            if(runStatus == -1){
                break;
            }
        }
        
        char str[2048] = {0};
        sprintf(str,"all test case number is [%d]\nrun test case number is [%d]\nsuccess test case number is [%d]\nfail test case number is [%d]",all,runNumber,success,runNumber-success);
        std::cout << str << std::endl;
    }
    
    static void addTestCase(UnitTestCaseInterface* testCase){
        UnitTest::getInstance().push_back(testCase);
    }
    
    static std::vector<UnitTestCaseInterface*>& getInstance(){
        static std::vector<UnitTestCaseInterface*> allTestCase;
        return allTestCase;
    }
};

#define Expect_EQ(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_EQ,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));
#define Expect_NE(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_NE,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));
#define Expect_GT(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_GT,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));
#define Expect_GE(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_GE,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));
#define Expect_LT(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_LT,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));
#define Expect_LE(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_LE,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));

#define Assert_EQ(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_EQ,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));
#define Assert_NE(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_NE,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));
#define Assert_GT(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_GT,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));
#define Assert_GE(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_GE,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));
#define Assert_LT(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_LT,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));
#define Assert_LE(lval,rval) UnitTest::addTestCase(new UnitTestCase<decltype(lval)>((lval),(rval),TestMethod::TestMethod_LE,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));

#define Expect_StrEQ(lval,rval) UnitTest::addTestCase(new UnitTestCase<std::string>(std::string((lval)),std::string((rval)),TestMethod::TestMethod_EQ,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));
#define Expect_StrNE(lval,rval) UnitTest::addTestCase(new UnitTestCase<std::string>(std::string((lval)),std::string((rval)),TestMethod::TestMethod_NE,TestMode::Expect,#lval,#rval,__FILE__,__LINE__));

#define Assert_StrEQ(lval,rval) UnitTest::addTestCase(new UnitTestCase<std::string>(std::string((lval)),std::string((rval)),TestMethod::TestMethod_EQ,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));
#define Assert_StrNE(lval,rval) UnitTest::addTestCase(new UnitTestCase<std::string>(std::string((lval)),std::string((rval)),TestMethod::TestMethod_NE,TestMode::Assert,#lval,#rval,__FILE__,__LINE__));

#define Expect_True(val)  Expect_EQ(val,true)
#define Expect_False(val) Expect_EQ(val,false)

#define Assert_True(val)  Assert_EQ(val,true)
#define Assert_False(val) Assert_EQ(val,false)

#define Run_All_TestCase() do{\
    UnitTest::run();\
    for(auto i=UnitTest::getInstance().begin();i!=UnitTest::getInstance().end();++i){\
        delete *i;\
    }\
    UnitTest::getInstance().clear();}\
    while(0)

} // shochu
#endif // UNITTEST_H
