#include "hiper.h"
#include "log.h"

#include <bitset>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string.h>

namespace hiper {

static hiper::Logger::ptr g_logger = LOG_NAME("system");

ByteArray::Node::Node(size_t s)
    : ptr(new char[s])
    , next(nullptr)
    , size(s)
{}

ByteArray::Node::Node()
    : ptr(nullptr)
    , next(nullptr)
    , size(0)
{}

ByteArray::Node::~Node()
{
    if (ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    : base_size_(base_size)
    , position_(0)
    , capacity_(base_size)
    , size_(0)
    , endian_(HIPER_BIG_ENDIAN)
    , root_(new Node(base_size))
    , cur_(root_)
{}

ByteArray::~ByteArray()
{
    Node* tmp = root_;
    while (tmp) {
        cur_ = tmp;
        tmp  = tmp->next;
        delete cur_;
    }
}

bool ByteArray::isLittleEndian() const
{
    return endian_ == HIPER_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val)
{
    if (val) {
        endian_ = HIPER_LITTLE_ENDIAN;
    }
    else {
        endian_ = HIPER_BIG_ENDIAN;
    }
}

void ByteArray::writeFint8(int8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value)
{
    if (endian_ != HIPER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value)
{
    if (endian_ != HIPER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value)
{
    if (endian_ != HIPER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value)
{
    if (endian_ != HIPER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value)
{
    if (endian_ != HIPER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value)
{
    if (endian_ != HIPER_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v)
{
    if (v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    }
    else {
        return v * 2;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v)
{
    if (v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    }
    else {
        return v * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v)
{
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v)
{
    return (v >> 1) ^ -(v & 1);
}


void ByteArray::writeInt32(int32_t value)
{
    writeUint32(EncodeZigzag32(value));
}

// 0000 0000 0000 0000 0000 0000 0101 0011     83
void ByteArray::writeUint32(uint32_t value)
{
    uint8_t tmp[5];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64(int64_t value)
{
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value)
{
    uint8_t tmp[10];
    uint8_t i = 0;
    while (value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    // 小于8位，直接截取
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat(float value)
{
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble(double value)
{
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string& value)
{
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string& value)
{
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string& value)
{
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string& value)
{
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string& value)
{
    write(value.c_str(), value.size());
}

int8_t ByteArray::readFint8()
{
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8()
{
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type)                       \
    type v;                            \
    read(&v, sizeof(v));               \
    if (endian_ == HIPER_BYTE_ORDER) { \
        return v;                      \
    }                                  \
    else {                             \
        return byteswap(v);            \
    }

int16_t ByteArray::readFint16()
{
    XX(int16_t);
}
uint16_t ByteArray::readFuint16()
{
    XX(uint16_t);
}

int32_t ByteArray::readFint32()
{
    XX(int32_t);
}

uint32_t ByteArray::readFuint32()
{
    XX(uint32_t);
}

int64_t ByteArray::readFint64()
{
    XX(int64_t);
}

uint64_t ByteArray::readFuint64()
{
    XX(uint64_t);
}

#undef XX

int32_t ByteArray::readInt32()
{
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32()
{
    uint32_t result = 0;
    for (int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if (b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        }
        else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64()
{
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64()
{
    uint64_t result = 0;
    for (int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if (b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        }
        else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

float ByteArray::readFloat()
{
    uint32_t v = readFuint32();
    float    value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double ByteArray::readDouble()
{
    uint64_t v = readFuint64();
    double   value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16()
{
    uint16_t    len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32()
{
    uint32_t    len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64()
{
    uint64_t    len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint()
{
    uint64_t    len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear()
{
    position_ = size_ = 0;
    capacity_         = base_size_;
    Node* tmp         = root_->next;
    while (tmp) {
        cur_ = tmp;
        tmp  = tmp->next;
        delete cur_;
    }
    cur_        = root_;
    root_->next = NULL;
}



// 向bytearray中写入buf中的数据
void ByteArray::write(const void* buf, size_t size)
{
    if (size == 0) {
        return;
    }
    addCapacity(size);

    size_t cur_shift = position_ % base_size_;   // 当前位置相对于当前缓冲区的偏移量
    size_t cur_left  = cur_->size - cur_shift;   // 当前缓冲区剩余的容量
    size_t buf_pos   = 0;                        // 追踪要写入的数据的位置

    while (size > 0) {
        if (cur_left >= size) {
            memcpy(cur_->ptr + cur_shift, (const char*)buf + buf_pos, size);
            if (cur_->size == (cur_shift + size)) {
                cur_ = cur_->next;
            }
            position_ += size;
            buf_pos += size;
            size = 0;
        }
        else {
            memcpy(cur_->ptr + cur_shift, (const char*)buf + buf_pos, cur_left);

            position_ += cur_left;
            buf_pos += cur_left;
            size -= cur_left;

            cur_      = cur_->next;
            cur_left  = cur_->size;
            cur_shift = 0;
        }
    }

    if (position_ > size_) {
        size_ = position_;
    }
}




void ByteArray::read(void* buf, size_t size)
{
    if (size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    size_t cur_shift = position_ % base_size_;
    size_t cur_left  = cur_->size - cur_shift;
    size_t buf_pos   = 0;

    while (size > 0) {
        if (cur_left >= size) {
            memcpy((char*)buf + buf_pos, cur_->ptr + cur_shift, size);
            if (cur_->size == (cur_shift + size)) {
                cur_ = cur_->next;
            }
            position_ += size;
            buf_pos += size;
            size = 0;
        }
        else {
            memcpy((char*)buf + buf_pos, cur_->ptr + cur_shift, cur_left);
            position_ += cur_left;
            buf_pos += cur_left;
            size -= cur_left;

            cur_      = cur_->next;
            cur_left  = cur_->size;
            cur_shift = 0;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > (size_ - position)) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = position % base_size_;
    size_t ncap = cur_->size - npos;
    size_t bpos = 0;
    Node* cur = cur_;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}


void ByteArray::setPosition(size_t v)
{
    if (v > capacity_) {
        throw std::out_of_range("set_position out of range");
    }
    position_ = v;
    if (position_ > size_) {
        size_ = position_;
    }
    cur_ = root_;
    while (v > cur_->size) {
        v -= cur_->size;
        cur_ = cur_->next;
    }
    if (v == cur_->size) {
        cur_ = cur_->next;
    }
}

bool ByteArray::writeToFile(const std::string& name) const
{
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if (!ofs) {
        LOG_ERROR(g_logger) << "writeToFile name=" << name << " error , errno=" << errno
                            << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = getReadSize();
    int64_t pos       = position_;
    Node*   cur       = cur_;

    while (read_size > 0) {
        int diff = pos % base_size_;
        // 当前节点中可用的字节数 len
        int64_t len = (read_size > (int64_t)base_size_ ? base_size_ : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    } 

    return true;
}

bool ByteArray::readFromFile(const std::string& name)
{
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if (!ifs) {
        LOG_ERROR(g_logger) << "readFromFile name=" << name << " error, errno=" << errno
                            << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buff(new char[base_size_], [](char* ptr) { delete[] ptr; });
    while (!ifs.eof()) {
        ifs.read(buff.get(), base_size_);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

void ByteArray::addCapacity(size_t size)
{
    if (size == 0) {
        return;
    }
    size_t old_cap = getCapacity();
    if (old_cap >= size) {
        return;
    }

    size         = size - old_cap;
    size_t count = ceil(1.0 * size / base_size_);
    Node*  tmp   = root_;
    while (tmp->next) {
        tmp = tmp->next;
    }

    Node* first = NULL; // 记录新创建的第一个Node
    for (size_t i = 0; i < count; ++i) {
        tmp->next = new Node(base_size_);
        if (first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        capacity_ += base_size_;
    }

    if (old_cap == 0) {
        cur_ = first;
    }
}

std::string ByteArray::toString() const
{
    std::string str;
    str.resize(getReadSize());
    if (str.empty()) {
        return str;
    }
    read(&str[0], str.size(), position_);
    return str;
}

std::string ByteArray::toHexString() const
{
    std::string       str = toString();
    std::stringstream ss;

    for (size_t i = 0; i < str.size(); ++i) {
        if (i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}


uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t       npos = position_ % base_size_;
    size_t       ncap = cur_->size - npos;
    struct iovec iov;
    Node*        cur = cur_;

    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len  = len;
            len          = 0;
        }
        else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len  = ncap;
            len -= ncap;
            cur  = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len,
                                   uint64_t position) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos  = position % base_size_;
    size_t count = position / base_size_;
    Node*  cur   = root_;
    while (count > 0) {
        cur = cur->next;
        --count;
    }

    size_t       ncap = cur->size - npos;
    struct iovec iov;
    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len  = len;
            len          = 0;
        }
        else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len  = ncap;
            len -= ncap;
            cur  = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len)
{
    if (len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t       npos = position_ % base_size_;
    size_t       ncap = cur_->size - npos;
    struct iovec iov;
    Node*        cur = cur_;
    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len  = len;
            len          = 0;
        }
        else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len  = ncap;

            len -= ncap;
            cur  = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

}   // namespace hiper