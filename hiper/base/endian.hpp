/*
 * @Author: Leo
 * @Date: 2023-09-05 15:50:41
 * @LastEditTime: 2023-09-05 20:11:21
 * @Description: 字节序操作
 */

 
#ifndef HIPER_ENDIAN_H
#define HIPER_ENDIAN_H

#include <cstdio>
#include <type_traits>
#include <stdint.h>
#include <byteswap.h>


#define HIPER_LITTLE_ENDIAN 1
#define HIPER_BIG_ENDIAN 2

namespace hiper {

// 字节序转换, 8字节
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

// 字节序转换, 4字节
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

// 字节序转换, 2字节
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define HIPER_BYTE_ORDER HIPER_BIG_ENDIAN 
#else
#define HIPER_BYTE_ORDER HIPER_LITTLE_ENDIAN
#endif

#if HIPER_BYTE_ORDER == HIPER_BIG_ENDIAN

template <class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

template <class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else

template <class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

template <class T>
T byteswapOnBigEndian(T t) {
    return t;
}

#endif

}

#endif // HIPER_ENDIAN_H