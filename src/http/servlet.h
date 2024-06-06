#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "http.h"
#include "http_session.h"
#include "mutex.h"

namespace orange {
namespace http {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string& name)
        :m_name(name) {}
    virtual ~Servlet() {}

    virtual int32_t handle(orange::http::HttpRequest::ptr request
                    , orange::http::HttpResponse::ptr response
                    , orange::http::HttpSession::ptr session) = 0;
    
    const std::string& getName() { return m_name; }
private:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t(orange::http::HttpRequest::ptr
            , orange::http::HttpResponse::ptr, orange::http::HttpSession::ptr)> callback;
    FunctionServlet(callback cb);

    int32_t handle(orange::http::HttpRequest::ptr request
                    , orange::http::HttpResponse::ptr response
                    , orange::http::HttpSession::ptr session) override;
private:
    callback m_cb;
};

class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef orange::RWMutex RWMutexType;

    ServletDispatch();
    int32_t handle(orange::http::HttpRequest::ptr request
                    , orange::http::HttpResponse::ptr response
                    , orange::http::HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);

    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    RWMutexType m_mutex;
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet();

    int32_t handle(orange::http::HttpRequest::ptr request
                    , orange::http::HttpResponse::ptr response
                    , orange::http::HttpSession::ptr session) override;
};

} // namespace orange
} // namespace http
