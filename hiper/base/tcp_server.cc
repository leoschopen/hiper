#include "config.h"
#include "hiper.h"
#include "log.h"

#include <cstdint>

namespace hiper {

static hiper::Logger::ptr              g_logger                  = LOG_NAME("system");
static hiper::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = hiper::Config::Lookup(
    "tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

TcpServer::TcpServer(hiper::IOManager* worker, hiper::IOManager* accept_worker)
    : io_worker_(worker)
    , accept_worker_(accept_worker)
    , recv_timeout_(g_tcp_server_read_timeout->getValue())
    , server_name_("hiper/1.0.0")
    , type_("tcp")
    , is_stop_(true)
{}

TcpServer::~TcpServer()
{
    for (auto& i : sockets_) {
        i->close();
    }
    sockets_.clear();
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails)
{
    for (auto& addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if (!sock->bind(addr)) {
            LOG_ERROR(g_logger) << "bind fail errno=" << errno << " errstr=" << strerror(errno)
                                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()) {
            LOG_ERROR(g_logger) << "listen fail errno=" << errno << " errstr=" << strerror(errno)
                                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        sockets_.push_back(sock);
    }

    if (!fails.empty()) {
        sockets_.clear();
        return false;
    }

    for (auto& i : sockets_) {
        LOG_INFO(g_logger) << "server bind success: " << *i;
    }

    return true;
}

bool TcpServer::bind(hiper::Address::ptr addr)
{
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

void TcpServer::startAccept(Socket::ptr sock)
{
    while (!is_stop_) {
        Socket::ptr client = sock->accept();
        if (client) {
            client->setRecvTimeout(recv_timeout_);
            io_worker_->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
            // io_worker_->schedule([this, client]() { this->handleClient(client); });
        }
        else {
            LOG_ERROR(g_logger) << "accept errno=" << errno << " errstr=" << strerror(errno);
        }
    }
}

bool TcpServer::start()
{
    if (!is_stop_) {
        return true;
    }
    is_stop_ = false;
    for (auto& sock : sockets_) {
        accept_worker_->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop()
{
    is_stop_  = true;
    auto self = shared_from_this();
    accept_worker_->schedule([this, self]() {
        for (auto& sock : sockets_) {
            sock->cancelAll();
            sock->close();
        }
        sockets_.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client)
{
    LOG_INFO(g_logger) << "handleClient: " << *client;
}

std::string TcpServer::toString(const std::string& prefix)
{
    std::stringstream ss;
    ss << prefix << "[type=" << type_ << " name=" << server_name_
       << " io_worker=" << (io_worker_ ? io_worker_->getName() : "")
       << " accept=" << (accept_worker_ ? accept_worker_->getName() : "")
       << " recv_timeout=" << recv_timeout_ << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for (auto& i : sockets_) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}

}   // namespace hiper