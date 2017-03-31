//
// Created by yulong on 3/31/17.
//
#include "tmpl_socket.h"

namespace bylSocket {
namespace Tmpl {

static void deleter(int *pf) {
    assert(pf && "deleter");
    if (close(*pf) == -1)
        err_report("close");
    delete pf;
}

template<Domain s_d, Type s_t>
Socket<s_d, s_t>
::Socket() : Socket(-1, Status::UNINITIALIZED) {
    *m_pfd = ::socket(static_cast<int>(s_d), static_cast<int>(s_t), 0);
    if (*m_pfd == -1) {
        err_report_and_throw("socket");
    }
    m_status = Status::FREE;
}

template<Domain d>
void set_sockaddr(const char *addr,
                  const char *port,
                  struct sockaddr_storage &ret_addr,
                  socklen_t &len);

template<>
void set_sockaddr<Domain::UNIX>(const char *addr,
                                const char *port,
                                struct sockaddr_storage &ret_addr,
                                socklen_t &len) {
    port = nullptr;
    struct sockaddr_un *p = (struct sockaddr_un *) &ret_addr;
    p->sun_family = AF_UNIX;
    len = sizeof(p->sun_family) + 1
          + std::min(sizeof(p->sun_path) - 2, strlen(addr));
    strncpy(p->sun_path + 1, addr, sizeof(p->sun_path) - 2);
}

template<>
void set_sockaddr<Domain::IP4>(const char *addr,
                               const char *port,
                               struct sockaddr_storage &ret_addr,
                               socklen_t &len) {

    struct sockaddr_in *p = (struct sockaddr_in *) &ret_addr;
    len = sizeof *p;
    p->sin_family = AF_INET;
    assert(port);
    p->sin_port = htons((uint16_t) atoi(port));
    if (inet_pton(AF_INET, addr, &(p->sin_addr)) <= 0)
        err_report_and_throw("inet_pton");
}

template<>
void set_sockaddr<Domain::IP6>(const char *addr,
                               const char *port,
                               struct sockaddr_storage &ret_addr,
                               socklen_t &len) {

    struct sockaddr_in6 *p = (struct sockaddr_in6 *) &ret_addr;
    len = sizeof *p;
    p->sin6_family = AF_INET6;
    assert(port);
    p->sin6_port = htons((uint16_t) atoi(port));
    if (inet_pton(AF_INET6, addr, &(p->sin6_addr)) <= 0)
        err_report_and_throw("inet_pton");
}

template<Domain s_d, Type s_t>
void Socket<s_d, s_t>
::bind(const char *local,
       const char *port) {
    assert_n_throw(m_status == Status::FREE);

    socklen_t slen;
    struct sockaddr_storage addr;
    set_sockaddr<s_d>(local, port, addr, slen);
    if (::bind(*m_pfd, (sockaddr *) &addr, slen))
        err_report_and_throw("bind");
    m_status = Status::BINDED;
}

template<Domain s_d, Type s_t>
void Socket<s_d, s_t>
::connect(const char *remote,
          const char *port) {
    assert_n_throw(m_status == Status::FREE || m_status == Status::BINDED);

    socklen_t slen;
    struct sockaddr_storage addr;
    set_sockaddr<s_d>(remote, port, addr, slen);
    if (::connect(*m_pfd, (sockaddr *) &addr, slen))
        err_report_and_throw("connect");
    m_status = Status::CONNECTED;
}

template<Domain s_d, Type s_t>
void Socket<s_d, s_t>
::listen(int backlog) {
    assert_n_throw(m_status == Status::BINDED && s_t == Type::STREAM);
    if (::listen(*m_pfd, backlog) == -1)
        err_report_and_throw("listen");
    m_status = Status::LISTENING;
}

template<Domain s_d, Type s_t>
Socket<s_d, s_t> Socket<s_d, s_t>
::accept() {
    assert_n_throw(m_status == Status::LISTENING && s_t == Type::STREAM);
    int fd = ::accept(*m_pfd, NULL, NULL);
    if (fd == -1)
        err_report_and_throw("accept");
    return Socket(fd, Status::CONNECTED);
}

template<Domain s_d, Type s_t>
Socket<s_d, s_t>
::Socket(int fd, Status ss) :
        m_pfd(new int(fd), deleter),
        m_status(ss) {}

template<Domain s_d, Type s_t>
void Socket<s_d, s_t>::set_opt(Options o,
                               time_t sec,
                               long nsec) {
    if (s_t != Type::STREAM && o == Options::KEEPALIVE) {
        err_report("KEEPALIVE only for connection based socket!");
        return;
    }
    if (s_t != Type::DGRAM && o == Options::DGRAM_BROADCAST) {
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

template<Domain D, Type T>
void BufferedSocket<D, T>::fsend(const char *format, ...) {
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
    if (::send(*this->m_pfd, m_buff, len + 1, 0) <= 0) {
        err_report_and_throw("send");
    }
}
template<Domain D, Type T>
void BufferedSocket<D, T>::send(const char *str) {
    int len = std::min((size_t) (BUFSZ), strlen(str));
    memcpy(m_buff, str, len);
    if (len >= BUFSZ) {
        err_report("send truncated.");
        len = BUFSZ - 1;
    }
    m_buff[len] = '\0';
    if (::send(*this->m_pfd, m_buff, len + 1, 0) <= 0) {
        err_report_and_throw("send");
    }
}
template<Domain D, Type T>
const char *BufferedSocket<D, T>::recv(int n) {
    assert_n_throw(this->m_status == Status::BINDED
                   || this->m_status == Status::CONNECTED);
    if (n < 0 || n > BUFSZ - 1) {
        err_report("n: out of range");
        return m_buff;
    }
    int len = ::recv(*this->m_pfd, m_buff, n, 0);
    if (len <= 0) {
        err_report_and_throw("recv");
    }
    m_buff[len] = '\0';
    return m_buff;
}

#define make_listen() do { \
this->set_opt(Options::REUSEADDR);\
this->set_opt(Options::REUSEPORT);\
this->bind(local, port);\
listen(backlog);} while(0)

ListenedSocket<Domain::IP4>::ListenedSocket(const char *port,
                                            const char *local,
                                            int backlog) {
    make_listen();
}

ListenedSocket<Domain::IP6>::ListenedSocket(const char *port,
                                            const char *local,
                                            int backlog) {
    make_listen();
}

ListenedSocket<Domain::UNIX>::ListenedSocket(const char *local, int backlog) {
    this->set_opt(Options::REUSEADDR);
    this->set_opt(Options::REUSEPORT);
    this->bind(local, "\0");
    listen(backlog);
}

//! Instantiating all possible classes
template
class Socket<Domain::IP4, Type::DGRAM>;
template
class Socket<Domain::IP6, Type::DGRAM>;
template
class Socket<Domain::UNIX, Type::DGRAM>;
template
class Socket<Domain::IP4, Type::STREAM>;
template
class Socket<Domain::IP6, Type::STREAM>;
template
class Socket<Domain::UNIX, Type::STREAM>;

template
class BufferedSocket<Domain::IP4, Type::DGRAM>;
template
class BufferedSocket<Domain::IP6, Type::DGRAM>;
template
class BufferedSocket<Domain::UNIX, Type::DGRAM>;
template
class BufferedSocket<Domain::IP4, Type::STREAM>;
template
class BufferedSocket<Domain::IP6, Type::STREAM>;
template
class BufferedSocket<Domain::UNIX, Type::STREAM>;

}
}//namespace bylSocket { namespace Tmpl {
