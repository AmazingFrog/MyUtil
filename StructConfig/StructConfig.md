# StructString

## 简介
提供saveToFile和loadFromFile接口,将配置类中的参数简单地保存到xml文件中
算是实现了简单的反射(?
xml使用的是[tinyxml2](https://github.com/leethomason/tinyxml2)
基本类型目前只支持int,double,float,bool,std::string
同时支持使用RegisterStruct宏注册的自定义结构体

## 用法
```c++
RegisterStruct_Begin(Test)
RegisterStruct_Menber(int, t)
RegisterStruct_End(Test)


RegisterStruct_Begin(MyStruct)
RegisterStruct_Menber(int,i)
RegisterStruct_Menber(Test,tt)
RegisterStruct_Menber(double,d)
RegisterStruct_End(MyStruct)


MyStruct test;
test.i = 10;
test.tt.t = 11;
test.d = 2.2;
test.saveToFile("MyConfig.xml");

MyStruct t2;
t2.loadFromFile("MyConfig.xml");
//use config
```
```xml
<MyStruct>
    <i type="int" uid="1">10</i>
    <Test type="struct" uid="2">
        <t type="int" uid="1">11</t>
    </Test>
    <d type="double" uid="3">2.200000</d>
</MyStruct>

```

## TODO
- [x] 支持更多类型(包括自定义类型)
- [ ] 提供保存为json的功能
