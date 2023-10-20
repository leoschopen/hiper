/*
 * @Author: Leo
 * @Date: 2023-10-04 16:50:26
 * @Description: 
 */
#include "../hiper/base/hiper.h"
#include <iostream>
#include <memory>

using namespace std;
using namespace hiper;
static Logger::ptr g_logger = LOG_ROOT();

class TestServer : public TcpServer {
public:
    typedef shared_ptr<TestServer> ptr;
    
};