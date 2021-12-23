# StructConfig

## 简介
提供saveToFile和loadFromFile接口,将配置类中的参数简单地保存到文件中
算是实现了简单的编译期反射(?
基本类型目前只支持int,double,bool,std::string
同时支持使用RegisterStruct宏注册的自定义结构体

## 用法
1. 继承`shochu::SCNodeFactoryInterface`实现用户自定义的序列化方式，并通过`shochu::SCNode::setFactory`设置给指定的结构体，默认使用xml格式(xml使用的是[tinyxml2](https://github.com/leethomason/tinyxml2))

1. 代码中定义结构体并使用
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

## note
1. 继承`shochu::SCNodeFactoryInterface`进行序列化时，要将`shochu::SCNode`的key, attr进行序列化，如果是基本类型的话，序列化val，否则遍历childs生成子节点添加到当前节点进行序列化
1. 反序列化时，要设置`shochu::SCNode`的key, isBase, 如果是基本类型就设置val,否则构建子节点添加到当前节点的child

## TODO
- [x] 支持更多类型(包括自定义类型)
- [x] 提供保存为json的功能(改为用户提供序列化方式)
