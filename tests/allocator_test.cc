#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mimalloc-2.1/mimalloc.h>   // Make sure you have the correct path to the mimalloc header
#include <jemalloc/jemalloc.h>
#include <string.h>
#include <sys/time.h>

class DefaultStackAllocator {

public:
    static void* Alloc(size_t size) { return std::malloc(size); }

    static void Dealloc(void* vp, size_t /* size */) { std::free(vp); }
};

class MiMallocStackAllocator {
public:
    static void* Alloc(size_t size) { return mi_malloc(size); }

    static void Dealloc(void* vp, size_t /* size */) { mi_free(vp); }
};

class JemallocStackAllocator {
public:
    static void* Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void* vp, size_t /* size */) { free(vp); }
};

int main()
{
    constexpr size_t numIterations  = 10000000;
    // constexpr size_t allocationSize = 1024;

    {
        std::cout << "Using mimalloc:" << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < numIterations; ++i) {
            int allocationSize = 10 * 4 + rand() % 1024;
            void* memory = MiMallocStackAllocator::Alloc(allocationSize);
            memset(memory, rand() % 128, allocationSize);
            MiMallocStackAllocator::Dealloc(memory, allocationSize);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Time taken: " << duration << " ms" << std::endl;
    }

    {
        std::cout << "Using jemalloc:" << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < numIterations; ++i) {
            int allocationSize = 10 * 4 + rand() % 1024;
            void* memory = JemallocStackAllocator::Alloc(allocationSize);
            memset(memory, rand() % 128, allocationSize);
            JemallocStackAllocator::Dealloc(memory, allocationSize);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Time taken: " << duration << " ms" << std::endl;
    }

    {
        std::cout << "Using default malloc:" << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < numIterations; ++i) {
            int allocationSize = 10 * 4 + rand() % 1024;
            void* memory = DefaultStackAllocator::Alloc(allocationSize);
            memset(memory, rand() % 128, allocationSize);
            DefaultStackAllocator::Dealloc(memory, allocationSize);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Time taken: " << duration << " ms" << std::endl;
    }

    return 0;
}
