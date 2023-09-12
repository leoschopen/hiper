#include "../hiper/base/hiper.h"

#include <algorithm>

static hiper::Logger::ptr g_logger = LOG_ROOT();

void test()
{
/*
 * 测试用例设计：
 * 随机生成长度为len，类型为type的数组，调用write_fun将这个数组全部写入块大小为base_len的ByteArray对象中，
 * 再将ByteArray的当前操作位置重设为0，也就是从起点开始，用read_fun重复读取数据，并与写入的数据比较，
 * 当读取出的数据与写入的数据全部相等时，该测试用例通过
 */
#define XX(type, len, write_fun, read_fun, base_len)                                 \
    {                                                                                \
        std::vector<type> vec;                                                       \
        for (int i = 0; i < len; ++i) {                                              \
            vec.push_back(42);                                                   \
        }                                                                            \
        hiper::ByteArray::ptr ba(new hiper::ByteArray(base_len));                    \
        for (auto& i : vec) {                                                        \
            ba->write_fun(i);                                                        \
        }                                                                            \
        ba->setPosition(0);                                                          \
        for (size_t i = 0; i < vec.size(); ++i) {                                    \
            type v = ba->read_fun();                                                 \
            HIPER_ASSERT(v == vec[i]);                                               \
        }                                                                            \
        HIPER_ASSERT(ba->getReadSize() == 0);                                        \
        LOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type ") len=" << len    \
                           << " base_len=" << base_len << " size=" << ba->getSize(); \
    }

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 5, writeFint32, readFint32, 1);
    XX(uint32_t, 5, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 5, writeInt32, readInt32, 1);
    XX(uint32_t, 5, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX
}

void test2(){
    hiper::ByteArray::ptr ba(new hiper::ByteArray(10));
    for (int i = 0; i < 10; ++i) {
        ba->writeInt64(i);
    }
    LOG_INFO(g_logger) << ba->toString();
    ba->setPosition(0);
    for (int i = 0; i < 10; ++i) {
        LOG_INFO(g_logger) << ba->readInt64();
    }
    LOG_INFO(g_logger) << ba->toString();
}

int main(int argc, char* argv[])
{
    test();
    return 0;
}