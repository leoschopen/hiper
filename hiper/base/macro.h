/*
 * @Author: Leo
 * @Date: 2023-08-06 09:51:01
 * @LastEditTime: 2023-08-06 10:47:57
 * @Description: 宏
 */

#ifndef HIPER_MACRO_H
#define HIPER_MACRO_H

#include <string.h>
#include <assert.h>

#if defined __GNUC__ || defined __llvm__
/// LIKELY 宏的封装, 告诉编译器优化,条件大概率成立
#   define HIPER_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKELY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define HIPER_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define HIPER_LIKELY(x)      (x)
#   define HIPER_UNLIKELY(x)      (x)
#endif


/// 断言宏封装
#define HIPER_ASSERT(x) \
    if(HIPER_UNLIKELY(!(x))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << hiper::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

/// 断言宏封装
#define HIPER_ASSERT2(x, w) \
    if(HIPER_UNLIKELY(!(x))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << hiper::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }


#endif // HIPER_MACRO_H
