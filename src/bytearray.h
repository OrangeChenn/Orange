#pragma once 

#include <stdint.h>
#include <sys/uio.h>

#include <memory>
#include <string>
#include <vector>

namespace orange {

class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;
    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;
        size_t size;
        Node* next;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    // write F(固定长度)
    void writeFint8  (int8_t value);
    void writeFuint8 (uint8_t value);
    void writeFint16 (int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32 (int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64 (int64_t value);
    void writeFuint64(uint64_t value);

    void writeInt32  (int32_t value);
    void writeUint32 (uint32_t value);
    void writeInt64  (int64_t value);
    void writeUint64 (uint64_t value);

    void writeFloat  (float value);
    void writeDouble (double value);

    // length:int16, data
    void writeStringF16(const std::string& value);
    // length:int32, data
    void writeStringF32(const std::string& value);
    // length:int64, data
    void writeStringF64(const std::string& value);
    // length:Varint, data
    void writeStringVint(const std::string& value);
    // data
    void writeStringWithoutLength(const std::string& value);

    // read
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float    readFloat();
    double   readDouble();

    // length:int16, data
    std::string readStringF16();
    // length:int32, data
    std::string readStringF32();
    // length:int64, data
    std::string readStringF64();
    // length:Varint, data
    std::string readStringVint();

    // 内部操作
    void clear();

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;

    size_t getPosition() const { return m_position; }
    void setPosition(size_t v);

    size_t getBaceSize() const { return m_baseSize; }
    size_t getReadSize() const { return m_size - m_position; }
    size_t getSize() const { return m_size; }

    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);

    std::string toString() const;
    std::string toHexString() const;

    bool isLittleEndian() const;
    void setisLittleEndian(bool value);

    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull);
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, size_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
private:
    void addCapacity(size_t size);
    size_t getCapacity() const { return m_capacity - m_position; }
private:
    size_t m_baseSize;
    size_t m_position;
    size_t m_capacity;
    size_t m_size;
    int8_t m_endian;
    Node* m_root;
    Node* m_cur;
};

} // namespace orange
