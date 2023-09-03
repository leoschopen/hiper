#include "fdmanager.h"
#include <sys/select.h>

namespace hiper {

FdCtx::FdCtx(int fd)
{
    is_init_       = false;
    is_socket_     = false;
    is_closed_     = false;
    user_nonblock_ = false;
    sys_nonblock_  = false;
    recv_timeout_  = -1;
    send_timeout_  = -1;
    fd_            = fd;
    init();
}

bool FdCtx::init() {
    if(is_init_) {
        return true;
    }
    recv_timeout_ = -1;
    send_timeout_ = -1;

    struct stat fd_stat;
    if(-1 == fd_set(fd_, &fd_stat)) {
        is_init_ = false;
        is_socket_ = false;
    } else {
        is_init_ = true;
        is_socket_ = S_ISSOCK(fd_stat.st_mode);
    }

    if(is_socket_){
        int flags = fcntl_old(fd_, F_GETFL, 0);
    }
}

}   // namespace hiper