#include "bytearray.h"

#include <math.h>
#include <string.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "endian.hpp"
#include "log.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

ByteArray::Node::Node(size_t s) 
    :ptr(new char[s])
    ,size(s)
    ,next(nullptr) {
}

ByteArray::Node::Node()
    :ptr(nullptr)
    ,size(0)
    ,next(nullptr) {
}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size) 
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(ORANGE_LITTLE_ENDIAN)
    ,m_root(new Node(base_size))
    ,m_cur(m_root) {
}

ByteArray::~ByteArray() {
    Node* tmp = m_root;
    m_root = nullptr;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

bool ByteArray::isLittleEndian() const {
    return ORANGE_LITTLE_ENDIAN == m_endian;
}

void ByteArray::setisLittleEndian(bool value) {
    if(value) {
        m_endian = ORANGE_LITTLE_ENDIAN;
    } else {
        m_endian = ORANGE_BIG_ENDIAN;
    }
}

// write F(固定长度)
void ByteArray::writeFint8  (int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8 (uint8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFint16 (int16_t value) {
    if(m_endian != ORANGE_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != ORANGE_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32 (int32_t value) {
    if(m_endian != ORANGE_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != ORANGE_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64 (int64_t value) {
    if(m_endian != ORANGE_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != ORANGE_BYTE_ORDER) {
        byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v >= 0) {
        return v * 2;
    } else {
        return ((uint32_t)(-v)) * 2 - 1;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v) {
    if(v >= 0) {
        return v * 2;
    } else {
        return ((uint64_t)(-v)) * 2 - 1;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}

void ByteArray::writeInt32  (int32_t value) {
    writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32 (uint32_t value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = ((value & 0x7f) | 0x80);
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64  (int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64 (uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = ((value & 0x7f) | 0x80);
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat  (float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(v));
    writeUint32(v);
}

void ByteArray::writeDouble (double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(v));
    writeUint64(v);
}

// length:int16, data
void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

// length:int32, data
void ByteArray::writeStringF32(const std::string& value) {
    writeFint32(value.size());
    write(value.c_str(), value.size());
}

// length:int64, data
void ByteArray::writeStringF64(const std::string& value) {
    writeFint64(value.size());
    write(value.c_str(), value.size());
}

// length:Varint, data
void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

// data
void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());
}

// read
int8_t   ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t  ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == ORANGE_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

int16_t  ByteArray::readFint16() {
    XX(int16_t);
}

uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

int32_t  ByteArray::readFint32() {
    XX(int32_t);
}

uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

int64_t  ByteArray::readFint64() {
    XX(int64_t);
}

uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

#undef XX

int32_t  ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32() {
    uint32_t v = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            v |= (((uint32_t)b) << i);
            break;
        } else {
            v |= ((uint32_t)(b & 0x7f) << i);
        }
    }
    return v;
}

int64_t  ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64() {
    uint32_t v = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            v |= (((uint32_t)b) << i);
            break;
        } else {
            v |= ((uint32_t)(b & 0x7f) << i);
        }
    }
    return v;
}

float    ByteArray::readFloat() {
    uint32_t v = readFint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double   ByteArray::readDouble() {
    uint64_t v = readFint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

// length:int16, data
std::string ByteArray::readStringF16() {
    uint16_t len = readFint16();
    std::string buff(len, 0);
    read(&buff[0], len);
    return buff;
}

// length:int32, data
std::string ByteArray::readStringF32() {
    uint32_t len = readFint32();
    std::string buff(len, 0);
    read(&buff[0], len);
    return buff;
}

// length:int64, data
std::string ByteArray::readStringF64() {
    uint64_t len = readFint64();
    std::string buff(len, 0);
    read(&buff[0], len);
    return buff;
}

// length:Varint, data
std::string ByteArray::readStringVint() {
    uint64_t len = readFint64();
    std::string buff(len, 0);
    read(&buff[0], len);
    return buff;
}

// 内部操作
void ByteArray::clear() {
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

void ByteArray::write(const void* buf, size_t size) {
    if(0 == size) {
        return;
    }
    addCapacity(size);

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;

    while(size > 0) {
        if(ncap >= size) {
            memcpy(m_cur->ptr + npos, static_cast<const char*>(buf) + bpos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy(m_cur->ptr + npos, static_cast<const char*>(buf) + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            m_cur = m_cur->next;
            size -= ncap;
            ncap = m_cur->size;
            npos = 0;
        }
    }
    if(m_position > m_size) {
        m_size = m_position;
    }
}

void ByteArray::read(void* buf, size_t size) {
    if(size > getReadSize()) {
        throw std::out_of_range("read not enougth len");
    }

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;

    while(size > 0) {
        if(ncap >= size) {
            memcpy(static_cast<char*>(buf) + bpos, m_cur->ptr + npos, size);
            if((npos + size) == m_cur->size) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy(static_cast<char*>(buf) + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            size -= ncap;
            bpos += ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > getReadSize()) {
        throw std::out_of_range("read not enougth len");
    }
    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy(static_cast<char*>(buf) + bpos, cur->ptr + npos, size);
            if((npos + size) == cur->size) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy(static_cast<char*>(buf) + bpos, cur->ptr + npos, ncap);
            position += ncap;
            size -= ncap;
            bpos += ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

void ByteArray::setPosition(size_t v) {
    if(v > m_capacity) {
        throw std::out_of_range("setPosition out of range");
    }
    m_position = v;
    if(v > m_size) {
        m_size = v;
    }
    m_cur = m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

bool ByteArray::writeToFile(const std::string& name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        ORANGE_LOG_ERROR(g_logger) << "writeToFile error, filename=" << name
                << " errno=" << errno << " strerror=" << strerror(errno);
        return false;
    }
    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_cur;
    while(read_size > 0) {
        int64_t diff = pos % m_baseSize;
        int64_t len = (read_size > (int64_t)m_baseSize ? (int64_t)m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        pos += len;
        read_size -= len;
        cur = cur->next;
    }
    return true;
}

bool ByteArray::readFromFile(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        ORANGE_LOG_ERROR(g_logger) << "readToFile error, filename=" << name
                << " errno=" << errno << " strerror=" << strerror(errno);
        return false;
    }
    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr){ delete[] ptr; });
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }
    return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) {
    len = len > getReadSize() ? getReadSize() : len;
    if(0 == len) {
        return 0;
    }
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_baseSize - npos;
    struct iovec iv;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iv.iov_base = cur->ptr + npos;
            iv.iov_len = len;
            len = 0;
        } else {
            iv.iov_base = cur->ptr + npos;
            iv.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iv);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, size_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(0 == len) {
        return 0;
    }
    uint64_t size = len;

    size_t npos = position % m_baseSize;
    struct iovec iv;

    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }
    size_t ncap = cur->size - npos;

    while(len > 0) {
        if(ncap >= len) {
            iv.iov_base = cur->ptr + npos;
            iv.iov_len = len;
            len = 0;
        } else {
            iv.iov_base = cur->ptr + npos;
            iv.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iv);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(0 == len) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_baseSize - npos;
    struct iovec iv;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iv.iov_base = cur->ptr + npos;
            iv.iov_len = len;
            len = 0;
        } else {
            iv.iov_base = cur->ptr + npos;
            iv.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iv);
    }
    return size;
}

void ByteArray::addCapacity(size_t size) {
    if(size == 0) {
        return;
    }
    size_t old_cap = getCapacity();
    if(old_cap >= size) {
        return;
    }

    size -= old_cap;
    size_t count = ceil((size * 1.0) / m_baseSize);
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }

    Node* first = nullptr;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == nullptr) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }
    if(old_cap == 0) {
        m_cur = first;
    }
}

} // namespace orange
