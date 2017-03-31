//
// Created by yulong on 3/31/17.
//

#ifndef BYLSOCKET_TMPL_SOCKET_H
#define BYLSOCKET_TMPL_SOCKET_H
#include "common.h"

namespace bylSocket {
namespace Tmpl {

//! template style alternative for Socket
/**
 * A socket has two ends : src/local and dest/remote
 *
 * bind : explicitly bind src to address/path/name
 * connect : implictly bound to free fd by os if not yet bound,
 *           then connect dest to specified addr
 * listen : make bound socket's dest open for all connection from
 *          different remotes/dests
 *          only necessary for stream
 * accept : return a socket whose source implicitly bound by os, and
 *          dest automatictily connected to client's address
 *          and return client's address info.
 *          only necessary for stream
 *
 * A passive socket (aka. server's socket, not matter dgram or stream),
 * must always bind it's source.
 *
 * A active socket (aka. client's socket),
 * recommending using connect its dest to implicitly bind its src.
 *
 * some rare cases: On the client side, you would only use bind if you
 * want to use a specific client-side port, which is rare. Usually on the
 * client, you specify the IP address and port of the server machine, and
 * the OS will pick which port you will use. Generally you don't care,
 * but in some cases, there may be a firewall on the client that only
 * allows outgoing connections on certain port. In that case, you will
 * need to bind to a specific port before the connection attempt will work.
 *
 * some FTP, NFS  and IRC clients' protocal
 *
 * N.B. for reliability and convenient resource release reasons,
 *      ONLY ABSTRACT unix domain sockets are implemented.
 *      reason: the named file on filesystem won't be unlinked if
 *      any system signals interrupt the program (the destructor won't
 *      be executed properly and thus remained)
 *
 */
template<Domain D, Type T>
class Socket {
public:
    Socket();
    void bind(const char *local, const char *port = "\0");
    void connect(const char *remote, const char *port = "\0");
    void listen(int backlog);
    Socket accept();
    void set_opt(Options o, time_t sec = 0, long int nsec = 0);

    Socket(const Socket &) = default;
    Socket(Socket &&) = default;
    Socket &operator=(const Socket &) = default;
    Socket &operator=(Socket &&) = default;
    virtual ~Socket() {}
protected:
    Socket(int fd, Status ss);
    std::shared_ptr<int> m_pfd;
    Status m_status;
};

template<Domain D, Type T>
class BufferedSocket : public Socket<D, T> {
public:
    BufferedSocket() : Socket<D, T>() {}
    BufferedSocket(const Socket<D, T> &o) : Socket<D, T>(o) {}
    BufferedSocket(Socket<D, T> &&o) : Socket<D, T>(std::move(o)) {}
    BufferedSocket(const BufferedSocket &o) : Socket<D, T>(o) {}
    BufferedSocket(BufferedSocket &&o) : Socket<D, T>(std::move(o)) {}
    BufferedSocket &operator=(const BufferedSocket &o) {
        Socket<D, T>::operator=(o);
        return *this;
    }
    BufferedSocket &operator=(BufferedSocket &&o) {
        Socket<D, T>::operator=(std::move(o));
        return *this;
    }

    virtual ~BufferedSocket() {}

    void fsend(const char *format, ...);
    void send(const char *str);
    const char *recv(int n = BUFSZ - 1);

protected:
    static const int BUFSZ = 512;
    char m_buff[BUFSZ];
};

template<Domain D>
class ListenedSocket { static_assert(true, "Not valid template resolution"); };

template<>
class ListenedSocket<Domain::IP4> : public Socket<Domain::IP4, Type::STREAM> {
public:
    explicit ListenedSocket(const char *port = "50000",
                            const char *local = "127.0.0.1",
                            int backlog = 50);

};
template<>
class ListenedSocket<Domain::IP6> : public Socket<Domain::IP6, Type::STREAM> {
public:
    explicit ListenedSocket(const char *port = "50000",
                            const char *local = "::1",
                            int backlog = 50);
};
template<>
class ListenedSocket<Domain::UNIX> : public Socket<Domain::UNIX, Type::STREAM> {
public:
    explicit ListenedSocket(const char *local, int backlog = 50);
};

}
}
#endif //BYLSOCKET_TMPL_SOCKET_H
