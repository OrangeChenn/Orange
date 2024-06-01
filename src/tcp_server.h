#include <memory>
#include <vector>

#include "address.h"
#include "iomanager.h"
#include "socket.h"

namespace orange {

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(orange::IOManager* worker = orange::IOManager::GetThis()
            , orange::IOManager* accept_worker = orange::IOManager::GetThis());
    virtual ~TcpServer();

    virtual bool bind(orange::Address::ptr addr);
    virtual bool bind(const std::vector<orange::Address::ptr>& addrs, std::vector<orange::Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    std::string getName() const { return m_name; }
    void setName(const std::string& v) { m_name = v; }

    bool isStop() const { return m_isStop; }
protected:
    virtual void startAccept(orange::Socket::ptr sock);
    virtual void handleClient(orange::Socket::ptr sock);
private:
    std::vector<orange::Socket::ptr> m_socks;
    orange::IOManager* m_worker;
    orange::IOManager* m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_isStop;
};

} // namespace orange
