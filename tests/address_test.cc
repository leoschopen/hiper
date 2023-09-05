#include "../hiper/base/hiper.h"

hiper::Logger::ptr g_logger = LOG_ROOT();

void test() {
    std::vector<hiper::Address::ptr> addrs;

    LOG_INFO(g_logger) << "begin";
    bool v = hiper::Address::Lookup(addrs, "localhost:3080");
    //bool v = hiper::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //bool v = hiper::Address::Lookup(addrs, "www.hiper.top", AF_INET);
    LOG_INFO(g_logger) << "end";
    if(!v) {
        LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = hiper::Address::LookupAny("localhost:4080");
    if(addr) {
        LOG_INFO(g_logger) << *addr;
    } else {
        LOG_ERROR(g_logger) << "error";
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<hiper::Address::ptr, uint32_t> > results;

    bool v = hiper::Address::GetInterfaceAddresses(results);
    if(!v) {
        LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = hiper::IPAddress::Create("www.hiper.top");
    auto addr = hiper::IPAddress::Create("127.0.0.8");
    if(addr) {
        LOG_INFO(g_logger) << addr->toString();
    }
}

void test_ipv6() {
    // auto addr = hiper::IPAddress::Create("127.0.0.1", AF_INET6);
    auto addr = hiper::IPv6Address::Create("2605:52c0:1001:1261:4512:4542:4576::");
    if(addr) {
        LOG_INFO(g_logger) << addr->toString();
        LOG_INFO(g_logger) << addr->broadcastAddress(98)->toString();
        LOG_INFO(g_logger) << addr->networkAddress(98)->toString();
    }
}

int main(int argc, char** argv) {
    // test_ipv4();
    test_ipv6();
    //test_iface();
    // test();
    return 0;
}
