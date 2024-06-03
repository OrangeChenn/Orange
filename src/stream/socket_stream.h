#pragma once

#include <memory>
#include "stream.h"
#include "socket.h"

namespace orange {

class SocketStream : public orange::Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(orange::Socket::ptr socket, bool owner = true);
    ~SocketStream();

    int read(void* buffer, size_t length) override;
    int read(orange::ByteArray::ptr ba, size_t length) override;
    int write(const void* buffer, size_t length) override;
    int write(const orange::ByteArray::ptr ba, size_t length) override;
    void close() override;

    bool isConnected() const;
    orange::Socket::ptr getSocket() { return m_socket; }
private:
    orange::Socket::ptr m_socket;
    bool m_owner;
};

} // namespace orange
