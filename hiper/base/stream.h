/*
 * @Author: Leo
 * @Date: 2023-10-02 20:04:20
 * @LastEditors: Please set LastEditors
 * @Description: 流接口
 */

#ifndef HIPER_STREAM_H
#define HIPER_STREAM_H

#include "bytearray.h"

#include <memory>

namespace hiper {

class Stream {
    public:
    typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream() {}

    virtual int read(void* buffer, size_t length) = 0;
    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    // 从流中读取指定长度的数据，并将其存储在缓冲区buffer中
    virtual int readFixSize(void* buffer, size_t length);

    // 从流中读取指定长度的数据，并将其存储在ByteArray中
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    virtual int write(const void* buffer, size_t length) = 0;
    virtual int write(ByteArray::ptr ba, size_t length) = 0;
    virtual int writeFixSize(const void* buffer, size_t length);
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    virtual void close() = 0;
};

}

#endif