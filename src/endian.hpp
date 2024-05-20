#pragma once

#include <byteswap.h>
#include <stdint.h>

#define ORANGE_LITTLE_ENDIAN 1
#define ORANGE_BIG_ENDIAN 2

namespace orange {

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64(value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32(value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16(value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define ORANGE_BYTE_ORDER ORANGE_BIG_ENDIAN
#else
#define ORANGE_BYTE_ORDER ORANGE_LITTLE_ENDIAN
#endif

#if ORANGE_BYTE_ORDER == ORANGE_BIG_ENDIAN

template<class T>
T byteswapOnLittleEndian(T value) {
    return value;
}

template<class T>
T byteswapOnBigEndian(T value) {
    return bytesawap(value);
}

#else

template<class T>
T byteswapOnLittleEndian(T value) {
    return byteswap(value);
}

template<class T>
T byteswapOnBigEndian(T value) {
    return value;
}

#endif


} // namespace orange
