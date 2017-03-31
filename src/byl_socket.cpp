//============================================================================
// Name        : cpp.cpp
// Author      : Yulong Bai
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "byl_socket.hpp"

static void deleter(int *pf) {
    assert(pf && "deleter");
    if (close(*pf) == -1)
        err_report("close");
    delete pf;
}

static struct sockaddr_storage set_sockaddr(const char *addr,
                                            const char *port,
                                            socklen_t &len,
                                            int dd) {
    struct sockaddr_storage ret_addr;
    memset(&ret_addr, 0, sizeof(struct sockaddr_storage));
    /**
    * for Unix socket, if address has a name, need to use unlink to
    * remove the file on the filesystem.
    */
    if (dd == AF_UNIX) {
        struct sockaddr_un *p = (struct sockaddr_un *) &ret_addr;

        p->sun_family = AF_UNIX;

        // only abstract style unix socket allowed
        // TODO : VERY SUBTLE BUGS! the 'length' together with name string dictates the socket's identity.
        len = sizeof(p->sun_family) + 1
              + std::min(sizeof(p->sun_path) - 2, strlen(addr));
        strncpy(p->sun_path + 1, addr, sizeof(p->sun_path) - 2);
    } else if (dd == AF_INET) {
        struct sockaddr_in *p = (struct sockaddr_in *) &ret_addr;
        len = sizeof *p;
        p->sin_family = AF_INET;
        assert(port);
        p->sin_port = htons((uint16_t) atoi(port));
        if (inet_pton(AF_INET, addr, &(p->sin_addr)) <= 0)
            err_report_and_throw("inet_pton");

    } else if (dd == AF_INET6) {
        struct sockaddr_in6 *p = (struct sockaddr_in6 *) &ret_addr;
        len = sizeof *p;
        p->sin6_family = AF_INET6;
        assert(port);
        p->sin6_port = htons((uint16_t) atoi(port));
        if (inet_pton(AF_INET6, addr, &(p->sin6_addr)) <= 0)
            err_report_and_throw("inet_pton");

    } else {
        err_report_and_throw("Not valid Domain");
    }
    return ret_addr;
}

bylSocket::Socket::Socket(Domain d, Type t)
        : m_pfd(new int(-1), deleter),
          m_domain(d),
          m_type(t),
          m_status(Status::UNINITIALIZED) {
    *m_pfd = ::socket(static_cast<int>(d), static_cast<int>(t), 0);
    if (*m_pfd == -1) {
        err_report_and_throw("socket");
    }
    m_status = Status::FREE;
}

void bylSocket::Socket::set_opt(Options o, time_t sec, long int nsec) {
    if (m_type != Type::STREAM && o == Options::KEEPALIVE) {
        err_report("KEEPALIVE only for connection based socket!");
        return;
    }
    if (m_type != Type::DGRAM && o == Options::DGRAM_BROADCAST) {
        err_report("DGRAM_BROADCAST only for DGRAM based socket!");
        return;
    }
    if ((o == Options::REUSEADDR || o == Options::REUSEPORT)
        && m_status != Status::FREE) {
        err_report("Options::REUSEADDR or Options::REUSEPORT "
                           "Must set before bound");
        return;
    }
    /**
     * TODO optval must not be type bool
     *      otherwise throwing invalid argument error
     */
    int optval = true;
    void *p = &optval;
    socklen_t len = sizeof optval;
    struct timeval t = {sec, nsec};
    if (o == Options::RCVTIMEO || o == Options::SNDTIMEO) {
        p = &t;
        len = sizeof t;
    }
    if (setsockopt(*m_pfd, SOL_SOCKET,
                   static_cast<int>(o), p, len) == -1)
        err_report_and_throw("setsockopt");
}

bylSocket::Socket::Socket(int fd, Domain d, Type t, Status ss)
        : m_pfd(new int(fd), deleter),
          m_domain(d),
          m_type(t),
          m_status(ss) {}

void bylSocket::Socket::bind(const char *local, const char *port) {
    assert_n_throw(m_status == Status::FREE);

    socklen_t slen;
    struct sockaddr_storage addr;
    addr = set_sockaddr(local,
                        port,
                        slen,
                        static_cast<int>(m_domain));
    if (::bind(*m_pfd, (sockaddr *) &addr, slen))
        err_report_and_throw("bind");
    m_status = Status::BINDED;
}

void bylSocket::Socket::connect(const char *remote, const char *port) {
    assert_n_throw(m_status == Status::FREE || m_status == Status::BINDED);

    socklen_t slen;
    struct sockaddr_storage addr
            = set_sockaddr(remote,
                           port,
                           slen,
                           static_cast<int>(m_domain));
    if (::connect(*m_pfd, (sockaddr *) &addr, slen))
        err_report_and_throw("connect");
    m_status = Status::CONNECTED;
}

void bylSocket::Socket::listen(int backlog) {
    assert_n_throw(m_status == Status::BINDED && m_type == Type::STREAM);

    if (::listen(*m_pfd, backlog) == -1)
        err_report_and_throw("listen");
    m_status = Status::LISTENING;
}

bylSocket::Socket bylSocket::Socket::accept() {
    assert_n_throw(m_status == Status::LISTENING && m_type == Type::STREAM);

    int fd = ::accept(*m_pfd, NULL, NULL);
    if (fd == -1)
        err_report_and_throw("accept");
    return Socket(fd, m_domain, m_type, Status::CONNECTED);
}

void bylSocket::BufferedSocket::fsend(const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    int len = vsnprintf(m_buff, BUFSZ, format, argptr);
    va_end(argptr);
    if (len < 0) {
        err_report("vsnprintf");
        return;
    }

    if (len >= BUFSZ) {
        /** according to the mannual **/
        err_report("fsend truncated.");
        len = BUFSZ - 1;
    }

    m_buff[len] = '\0';
    if (::send(*m_pfd, m_buff, len + 1, 0) <= 0) {
        err_report_and_throw("send");
    }
}
void bylSocket::BufferedSocket::send(const char *str) {
    int len = std::min((size_t) (BUFSZ), strlen(str));
    memcpy(m_buff, str, len);
    if (len >= BUFSZ) {
        err_report("send truncated.");
        len = BUFSZ - 1;
    }
    m_buff[len] = '\0';
    if (::send(*m_pfd, m_buff, len + 1, 0) <= 0) {
        err_report_and_throw("send");
    }
}
const char *bylSocket::BufferedSocket::recv(int n) {
    assert_n_throw(m_status == bylSocket::Status::BINDED
                   || m_status == bylSocket::Status::CONNECTED);
    if (n < 0 || n > BUFSZ - 1) {
        err_report("n: out of range");
        return m_buff;
    }
    auto len = ::recv(*m_pfd, m_buff, n, 0);
    if (len <= 0) {
        err_report_and_throw("recv");
    }
    m_buff[len] = '\0';
    return m_buff;
}
bylSocket::BufferedSocket::BufferedSocket(Domain d, Type t) : Socket(d, t) {}
bylSocket::BufferedSocket::BufferedSocket(const Socket &o) : Socket(o) {}
bylSocket::BufferedSocket::BufferedSocket(const bylSocket::BufferedSocket &o) : Socket(o) {}

bylSocket::BufferedSocket &bylSocket::
BufferedSocket::operator=(const bylSocket::BufferedSocket &o) {
    Socket::operator=(o);
    return *this;
}

bylSocket::BufferedSocket::
BufferedSocket(bylSocket::BufferedSocket &&o) : Socket::Socket(std::move(o)) {}

bylSocket::BufferedSocket &bylSocket::
BufferedSocket::operator=(bylSocket::BufferedSocket &&o) {
    Socket::operator=(std::move(o));
    return *this;
}
bylSocket::BufferedSocket::BufferedSocket(bylSocket::Socket &&o)
        : Socket(std::move(o)) {

}

bylSocket::ListenedSocket::ListenedSocket(Domain d,
                                          const char *port,
                                          const char *local,
                                          int backlog) : Socket(d, Type::STREAM) {
    this->set_opt(Options::REUSEADDR);
    this->set_opt(Options::REUSEPORT);
    this->bind(local, port);
    listen(backlog);
}

bylSocket::ListenedSocket::ListenedSocket(const char *local, int backlog)
        : ListenedSocket(Domain::UNIX, "xxxx", local, backlog) {}

