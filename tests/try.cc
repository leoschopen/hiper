#include <cstdio>
#include <iostream>
#include <memory>
#include <bitset>
#include <netinet/in.h>
#include <type_traits>
using namespace std;


static uint32_t EncodeZigzag32(const int32_t& v)
{
    if (v < 0) {
        return ((uint32_t)(-v) << 1)  - 1;
    }
    else {
        return v << 1;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v)
{
    return (v >> 1) ^ -(v & 1);
}

// 0000 0000 0000 0000 0000 0000 0101 0011     83
void writeUint32(uint32_t value)
{
    uint8_t tmp[5];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
}


void writeInt32(int32_t value)
{
    writeUint32(EncodeZigzag32(value));
}


int main() {
    int32_t originalValue = -42;
    uint32_t encodedValue = EncodeZigzag32(originalValue);
    int32_t decodedValue = DecodeZigzag32(encodedValue);

    std::cout << "Original: " << std::bitset<32>(originalValue) << std::endl;
    std::cout << "Original: " << std::bitset<32>((-originalValue)) << std::endl;
    std::cout << "Encoded : " << std::bitset<32>(encodedValue) << std::endl;
    std::cout << "Decoded : " << std::bitset<32>(decodedValue) << std::endl;

    writeInt32(originalValue);

    return 0;
}