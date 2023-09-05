#include <iostream>

int main() {
#ifdef __BYTE_ORDER__
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        std::cout << "Little Endian" << std::endl;
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        std::cout << "Big Endian" << std::endl;
    #elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
        std::cout << "PDP Endian" << std::endl;
    #else
        std::cout << "Unknown Endianness" << std::endl;
    #endif
#else
    std::cout << "Endianness detection not supported" << std::endl;
#endif

    return 0;
}