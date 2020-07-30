# StructString

## 简介
提供saveToFile和loadFromFile接口,将配置类中的参数简单地保存到xml文件中
算是实现了简单的反射(?
xml使用的是[tinyxml2](https://github.com/leethomason/tinyxml2)
目前只支持int,double,float,bool,std::string

## 用法
```c++
RegisterStruct_Begin(MyConfig)
RegisterStruct_Menber(int,i)
RegisterStruct_Menber(double,d)
RegisterStruct_Menber(float,f)
RegisterStruct_Menber(bool,b)
RegisterStruct_Menber(std::string,s)
RegisterStruct_End(MyConfig)


MyConfig t1;
t1.i = 1;
t1.d = 1.1;
t1.f = 1.2f;
t1.b = false;
t1.s = "str";
t1.saveToFile("MyConfig.xml");

MyConfig t2;
t2.loadFromFile("MyConfig.xml");
//use config
```

## TODO
- [ ] 支持更多类型(包括自定义类型)
- [ ] 提供保存为json的功能
