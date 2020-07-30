# UnitTest

## 简介
单元测试类

## 用法
```c++
//自定义结构体需要特例化isEQ和isGT
struct MyStruct{};
template<>
bool UnitTestCase<MyStruct>::isEQ(const MyStruct &l, const MyStruct &r) {
    return l == r;
}
template<>
bool UnitTestCase<MyStruct>::isGT(const MyStruct &l, const MyStruct &r) {
    return l > r;
}

MyStruct t1;
MyStruct t2;

Expect_EQ(t1,t2);
Expect_EQ(1,2);
...
Run_All_TestCase();
```