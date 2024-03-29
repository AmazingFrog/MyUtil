﻿#ifndef ICDBASE_HPP
#define ICDBASE_HPP

#include <limits>
#include <string>
#include <vector>
#include <cstring>
#include <typeinfo>
#include <functional>
#include <type_traits>

#include <iostream>

// #include <QtGlobal>

// 对不同平台 counter 宏进行统一
#ifdef Q_OS_WIN
#define __MY_COUNTER __COUNTER__
#else
#define __MY_COUNTER __COUNTER__
#endif

namespace ICD
{
// 提供重载参数
template<int N>
struct __uuid{};

// 因为用的c++11，没有c++14的make_index_sequence用，所以自己重新实现一个
// std::integer_sequence
template<size_t... I>
struct __my_index_sequence {};
// std::make_index_sequence
template<size_t N, size_t... M>
struct __my_make_index_sequence : public __my_make_index_sequence<N - 1, N -1, M...> {};
template<size_t... M>
struct __my_make_index_sequence<0, M...> : public __my_index_sequence<M...> {};

// copyValue模板类 提供从 [字节流] 到 [字段] 的拷贝
// 两个运算符重载分别为拷贝 单个字段 和 拷贝数组
// bool 表示dstType是否是基本类型
template<bool, typename dstType>
struct __copyValue
{
    static inline int copy(const uint8_t*, dstType&, int) { return 0; }
    static inline int copy(const uint8_t*, dstType*, size_t, int) { return 0; }
};
// copyValue 偏特化
// 如果是基本类型，直接赋值
template<typename dstType>
struct __copyValue<true, dstType>
{
    static inline int copy(const uint8_t* data, dstType& val, int remainBytes)
    {
        if(remainBytes < sizeof(dstType))
        {
            return -(int)sizeof(dstType);
        }
        memcpy(&val, data, sizeof(dstType));
        return sizeof(dstType);
    }
    static inline int copy(const uint8_t* data, dstType* val, size_t n, int remainBytes)
    {
        if(remainBytes < sizeof(dstType) * n)
        {
            return -(int)(sizeof(dstType) * n);
        }
        memcpy(val, data, sizeof(dstType) * n);
        return sizeof(dstType) * n;
    }
};
// 如果不是基本类型，则必须为用 ICD_DEF_BEG, ICD_DEF_END 定义过的结构体
// 采用结构体内的 from函数 来拷贝数据
template<typename dstType>
struct __copyValue<false, dstType>
{
    static inline int copy(const uint8_t* data, dstType& val, int remainBytes)
    {
        return val.from(data, remainBytes);
    }
    static inline int copy(const uint8_t* data, dstType* val, size_t n, int remainBytes)
    {
        int total = 0;
        for(size_t i=0;i<n;++i)
        {
            int offset = (val + i)->from(data, remainBytes);
            if(offset < 0)
            {
                return -total + offset;
            }
            data += offset;
            total += offset;
            remainBytes -= offset;
        }
        return total;
    }
};

// pasteValue模板类 提供从 [字段] 到 [字节流] 的拷贝
// 两个运算符重载分别为拷贝 单个字段 和 拷贝数组
// bool 表示dstType是否是基本类型
template<bool, typename dstType>
struct __pasteValue
{
    static inline int paste(void*, dstType&, int) { return 0; }
    static inline int paste(void*, dstType*, size_t, int) { return 0; }
};
template<typename dstType>
struct __pasteValue<true, dstType>
{
    static inline size_t paste(void* data, dstType& val, int remainBytes)
    {
        if(remainBytes < sizeof(dstType))
        {
            return -(int)sizeof(dstType);
        }
        memcpy(data, &val, sizeof(dstType));
        return sizeof(dstType);
    }
    static inline size_t paste(void* data, dstType* val, size_t n, int remainBytes)
    {
        if(remainBytes < sizeof(dstType) * n)
        {
            return -(int)(sizeof(dstType) * n);
        }
        memcpy(data, val, sizeof(dstType) * n);
        return sizeof(dstType) * n;
    }
};
template<typename dstType>
struct __pasteValue<false, dstType>
{
    static inline int paste(void* data, dstType& val, int remainBytes)
    {
        return val.to(data, remainBytes);
    }
    static inline int paste(void* data, dstType* val, size_t n, int remainBytes)
    {
        uint8_t* beg = (uint8_t*)data;
        int total = 0;
        for(int i=0;i<n;++i)
        {
            int offset = (val + i)->to(beg, remainBytes);
            if(offset < 0)
            {
                return -total + offset;
            }
            beg += offset;
            total += offset;
            remainBytes -= offset;
        }
        return total;
    }
};

// 获取该类型的字节长度
template<bool, typename ty>
struct __getLenHelper
{
    static size_t value(void*)
    {
        return 0;
    }
};
template<typename ty>
struct __getLenHelper<true, ty>
{
    static size_t value(void*)
    {
        return sizeof(ty);
    }
};
template<typename ty>
struct __getLenHelper<false, ty>
{
    static size_t value(void* cls)
    {
        return ((ty*)cls)->calcICDlen();
    }
};

// 把所有字段反序列化
struct __unserialFieldHelper
{
    template<typename ty, size_t N, size_t... I>
    inline static typename std::enable_if<sizeof...(I)!=0, int>::type unserial(ty* cls, const void* data, ICD::__my_index_sequence<N, I...>, int  remainBytes)
    {
        int offset = cls->__unserialField(ICD::__uuid<N+1>(), data, remainBytes);
        if(offset < 0)
        {
            return offset;
        }
        int after = ICD::__unserialFieldHelper::unserial<ty, I...>(cls, (const uint8_t*)data+offset, ICD::__my_index_sequence<I...>{}, remainBytes - offset);
        if(after < 0)
        {
            return -offset + after;
        }
        return offset + after;
    }
    template<typename ty, size_t N, size_t... I>
    inline static typename std::enable_if<sizeof...(I)==0, int>::type unserial(ty* cls, const void* data, ICD::__my_index_sequence<N, I...>, int  remainBytes)
    {
        return cls->__unserialField(ICD::__uuid<N+1>(), data, remainBytes);
    }
};

// 把所有字段序列化
struct __serialFieldHelper
{
    template<typename ty, size_t N, size_t... I>
    inline static typename std::enable_if<sizeof...(I)!=0, int>::type serial(ty* cls, void* data, ICD::__my_index_sequence<N, I...>, int  remainBytes)
    {
        int offset = cls->__serialField(ICD::__uuid<N+1>(), data, remainBytes);
        if(offset < 0)
        {
            return offset;
        }
        int after = ICD::__serialFieldHelper::serial<ty, I...>(cls, (uint8_t*)data+offset, ICD::__my_index_sequence<I...>{}, remainBytes - offset);
        if(after < 0)
        {
            return -offset + after;
        }
        return offset + after;
    }
    template<typename ty, size_t N, size_t... I>
    inline static typename std::enable_if<sizeof...(I)==0, int>::type serial(ty* cls, void* data, ICD::__my_index_sequence<N, I...>, int  remainBytes)
    {
        return cls->__serialField(ICD::__uuid<N+1>(), data, remainBytes);
    }
};

// 计算当前结构体的icd长度
template<int N, typename ty>
struct __calcICDLenHelper
{
    static size_t calc(ty* cls)
    {
        return cls->__calcFieldLen(ICD::__uuid<N>()) + ICD::__calcICDLenHelper<N-1, ty>::calc(cls);
    }
};
template<typename ty>
struct __calcICDLenHelper<0, ty>
{
    static size_t calc(ty*) { return 0; }
};

// 将所有基本类型初始化为0
template<int N, typename cls>
struct __initFieldLoop
{
    inline static void init(cls* c)
    {
        c->__initField(__uuid<N>());
        __initFieldLoop<N-1, cls>::init(c);
    }
};
template<typename cls>
struct __initFieldLoop<0, cls>
{
    inline static void init(cls*) { }
};

// 将基本类型初始化为0
template<bool, typename t>
struct __initField
{
    inline static void init(t&) { }
    inline static void init(t*, size_t) { }
};
template<typename t>
struct __initField<true, t>
{
    inline static void init(t& __t) { __t = 0; }
    inline static void init(t* __t, size_t __n) { memset(__t, 0, __n * sizeof(t)); }
};

template<typename T>
struct isDefByIcd
{
    template<typename ty>
    constexpr inline static bool test(decltype(ty::__ICDDef)*)
    {
        return true;
    }
    template<typename ty>
    constexpr inline static bool test(...)
    {
        return false;
    }
    constexpr static bool value = test<T>(0);
};
#define isNotDefByIcd(ty) !ICD::isDefByIcd<ty>::value

} // namespace ICD

#define COMMENT(str)

#define ICD_DEF_BEG \
    const static size_t __start = __MY_COUNTER; \
    constexpr static size_t __ICDDef = 114514;

// 定义字段isDefByIcd
// ty     字段类型
// name   字段名
// 建议ty尽量采用 int32_t 这种类型，提供明确的字段长度的语义
// 返回值为 初始化该变量使用的字节数
#define ICD_DEF_FIELD(ty, name) \
    enum { __field_##name = __MY_COUNTER - __start }; \
    ty name; \
    int __unserialField(ICD::__uuid<__field_##name>, const void* _data, int _remainBytes) \
    {\
        return ICD::__copyValue<!ICD::isDefByIcd<ty>::value, ty>::copy((const uint8_t*)_data, name, _remainBytes); \
    }\
    int __serialField(ICD::__uuid<__field_##name>, void* _data, int  _remainBytes) \
    {\
        return ICD::__pasteValue<!ICD::isDefByIcd<ty>::value, ty>::paste(_data, name, _remainBytes); \
    }\
    size_t __calcFieldLen(ICD::__uuid<__field_##name>) \
    {\
        return sizeof(ty); \
    }\
    void __initField(ICD::__uuid<__field_##name>)\
    {\
        ICD::__initField<!ICD::isDefByIcd<ty>::value, ty>::init(name);\
    }

// 定义定长数组
// 参数和 ICD_DEF_FIELD 类似
// len 为数组长度
// 返回值为 初始化该变量使用的字节数
#define ICD_DEF_FIX_LEN_ARRAY_FIELD(ty, name, len) \
    enum { __field_##name = __MY_COUNTER - __start }; \
    ty name[len]; \
    int64_t __unserialField(ICD::__uuid<__field_##name>, const void* _data, int _remainBytes) \
    {\
        return ICD::__copyValue<!ICD::isDefByIcd<ty>::value, ty>::copy((const uint8_t*)_data, (ty*)name, len, _remainBytes); \
    }\
    int __serialField(ICD::__uuid<__field_##name>, void* _data, int _remainBytes) \
    {\
        return ICD::__pasteValue<!ICD::isDefByIcd<ty>::value, ty>::paste(_data, (ty*)name, len, _remainBytes); \
    }\
    size_t __calcFieldLen(ICD::__uuid<__field_##name>) \
    {\
        size_t total = 0; \
        for(decltype(len) i=0;i<len;++i) \
        {\
            total += ICD::__getLenHelper<!ICD::isDefByIcd<ty>::value, ty>::value(name + i); \
        }\
        return total; \
    }\
    void __initField(ICD::__uuid<__field_##name>)\
    {\
        ICD::__initField<!ICD::isDefByIcd<ty>::value, ty>::init(name, len);\
    }

// 定义变长数组
// num   变长数组个数变量名, 必须为前面 ICD_DEF_FIELD 定义的
// ty    变长数组的元素类型
// name  变长数组变量名
// 返回值为 初始化该变量使用的字节数
// 变长数组采用 std::vector 存储
// 适合于 [x, y, x个结构体, y个结构体] 这种类型
#define ICD_DEF_VAR_LEN_ARRAY_FORWARD_FIELD(num, ty, name) \
    enum { __field_##name = __MY_COUNTER - __start }; \
    std::vector<ty> name; \
    int __unserialField(ICD::__uuid<__field_##name>, const void* _data, int _remainBytes) \
    {\
        const uint8_t* _beg = (const uint8_t*)_data; \
        int _totalEelemSize = 0; \
        for(decltype(num) i=0;i<num;++i) \
        {\
            ty _t; \
            int _elemSize = ICD::__copyValue<!ICD::isDefByIcd<ty>::value, ty>::copy(_beg, _t, _remainBytes); \
            if(_elemSize < 0) \
            {\
                return -_totalEelemSize + _elemSize; \
            }\
            _totalEelemSize += _elemSize; \
            _beg += _elemSize; \
            _remainBytes -= _elemSize; \
            name.push_back(std::move(_t)); \
        }\
        return _totalEelemSize; \
    }\
    int __serialField(ICD::__uuid<__field_##name>, void* _data, int _remainBytes) \
    {\
        uint8_t* _beg = (uint8_t*)_data; \
        int _totalElemSize = 0; \
        for(size_t i=0;i<name.size() && i<num;++i) \
        {\
            auto& _e = name[i]; \
            int _elemSize = ICD::__pasteValue<!ICD::isDefByIcd<ty>::value, ty>::paste(_beg, _e, _remainBytes); \
            if(_elemSize < 0) \
            {\
                return -_totalElemSize + _elemSize; \
            }\
            _totalElemSize += _elemSize; \
            _beg += _elemSize; \
            _remainBytes -= _elemSize; \
        }\
        return _totalElemSize; \
    }\
    size_t __calcFieldLen(ICD::__uuid<__field_##name>) \
    {\
        size_t len = 0; \
        for(int i=0;i<name.size() && i<num;++i) \
        {\
            len += ICD::__getLenHelper<!ICD::isDefByIcd<ty>::value, ty>::value(&name[i]); \
        }\
        return len; \
    }\
    void __initField(ICD::__uuid<__field_##name>) { name.clear(); }

// 定义变长数组
// numTy 变长数组个数变量的类型
// num   变长数组个数变量名
// ty    变长数组的元素类型
// name  变长数组变量名
// 返回值为 初始化该变量使用的字节数
// 变长数组采用 std::vector 存储
// 将字节流中开头长度为 sizeof(numTy) 大小的数据用来初始化 num, 之后的数据用来初始化 name
// 适合于 [x, x个结构体] 这种类型
#define ICD_DEF_VAR_LEN_ARRAY_FIDLD(numTy, num, ty, name) \
    ICD_DEF_FIELD(numTy, num) \
    ICD_DEF_VAR_LEN_ARRAY_FORWARD_FIELD(num, ty, name)

// 定义占位符
// size 字节大小
#define ICD_DEF_NULL(size) ICD_DEF_NULL_HELPER1(size, __LINE__)
#define ICD_DEF_NULL_HELPER1(size, line) ICD_DEF_NULL_HELPER2(size, line)
#define ICD_DEF_NULL_HELPER2(size, name) \
    enum { __field_##name = __MY_COUNTER - __start }; \
    int __unserialField(ICD::__uuid<__field_##name>, const void*, int _remainBytes) \
    {\
        return (_remainBytes<size)?0:size; \
    }\
    int __serialField(ICD::__uuid<__field_##name>, void*, int _remainBytes) \
    {\
        return (_remainBytes<size)?0:size; \
    }\
    size_t __calcFieldLen(ICD::__uuid<__field_##name>) \
    {\
        return size;\
    }\
    void __initField(ICD::__uuid<__field_##name>) { }

#define ICD_DEF_END(cls) \
    const static size_t __fieldNum = __MY_COUNTER - __start - 1; \
    COMMENT("len为缓冲区的长度") \
    COMMENT("返回值大于0，则成功") \
    COMMENT("返回值小于0，则失败，绝对值为直到出现不能初始化的字段一共使用的字节数(包括不能初始化的这个字段大小)") \
    int from(const void* data, int len = std::numeric_limits<int>::max()) \
    {\
        ICD::__initFieldLoop<__fieldNum, cls>::init(this);\
        return ICD::__unserialFieldHelper::unserial(this, data, ICD::__my_make_index_sequence<__fieldNum>{}, len); \
    }\
    int to(void* data, int len = std::numeric_limits<int>::max()) \
    {\
        return ICD::__serialFieldHelper::serial(this, data, ICD::__my_make_index_sequence<__fieldNum>{}, len); \
    }\
    COMMENT("计算当前结构体需要多少字节存储, 前提是有值, 计算才有意义") \
    size_t calcICDlen() \
    {\
        return ICD::__calcICDLenHelper<__fieldNum, std::remove_pointer<decltype(this)>::type>::calc(this); \
    }\
    COMMENT("构造函数中初始化所有基本类型的字段为0") \
    cls()\
    {\
        ICD::__initFieldLoop<__fieldNum, cls>::init(this);\
    }

/**
 * @code
 * struct Test
 * {
 *     ICD_DEF_BEG
 *
 *     ICD_DEF_FIELD(uint32, m_id)
 *     ICD_DEF_FIX_LEN_ARRAY_FIELD(char, m_name)
 *
 *     ICD_DEF_END(Test)
 * };
 *
 * struct Test1
 * {
 *     ICD_DEF_BEG
 *
 *     ICD_DEF_FIELD(uint32_t, m_id)
 *     ICD_DEF_FIELD(Test, m_test)
 *
 *     ICD_DEV_END(Test1)
 * };
 *
 * struct Test2
 * {
 *     ICD_DEF_BEG
 *
 *     ICD_DEF_VAR_LEN_ARRAY_FIDLD(uint32_t, num, uint32_t, m_data)
 *
 *     ICD_DEV_END(Test2)
 * };
 * @endcode
 */

#endif // ICDBASE_HPP
