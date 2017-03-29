/*
 * bylSocket.hpp
 *
 *  Created on: Feb 22, 2017
 *      Author: yulong
 */

#ifndef BYLSOCKET_HPP_
#define BYLSOCKET_HPP_

#include "util.h"
#include <memory>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>


namespace bylSocket {

enum class Domain { UNIX = AF_UNIX, IP4 = AF_INET, IP6 = AF_INET6 };
enum class Type { STREAM = SOCK_STREAM, DGRAM = SOCK_DGRAM };
enum class Status { UNINITIALIZED, FREE, BINDED, LISTENING, CONNECTED };
enum class Options {
    DGRAM_BROADCAST = SO_BROADCAST,
    REUSEADDR       = SO_REUSEADDR,
    REUSEPORT       = SO_REUSEPORT,
    KEEPALIVE       = SO_KEEPALIVE,
    RCVTIMEO        = SO_RCVTIMEO,
    SNDTIMEO        = SO_SNDTIMEO
};
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
class Socket {
public:
    Socket(Domain d, Type t);
    void bind(const char *local, const char *port = "\0");
    void connect(const char *remote, const char *port = "\0");
    void listen(int backlog);
    Socket accept();

    Socket(const Socket &) = default;
    Socket(Socket &&) = default;
    Socket &operator=(const Socket &) = default;
    Socket &operator=(Socket &&) = default;

    virtual ~Socket() {}

    void set_opt(Options o, time_t sec = 0, long int nsec = 0);
protected:
    Socket(int fd, Domain d, Type t, Status ss);
    std::shared_ptr<int> m_pfd;
    Domain               m_domain;
    Type                 m_type;
    Status               m_status;

};

// template style alternative of Socket
template<Domain s_d, Type s_t>
class MagicSocket {
public:
    MagicSocket();
    void bind(const char *local, const char *port = "\0");
    void connect(const char *remote, const char *port = "\0");
    void listen(int backlog);
    MagicSocket accept();
protected:
    MagicSocket(int fd, Status ss);
private:
    std::shared_ptr<int> m_pfd;
    Status               m_status;
};

template
class MagicSocket<Domain::IP4, Type::DGRAM>;
template
class MagicSocket<Domain::IP6, Type::DGRAM>;
template
class MagicSocket<Domain::UNIX, Type::DGRAM>;
template
class MagicSocket<Domain::IP4, Type::STREAM>;
template
class MagicSocket<Domain::IP6, Type::STREAM>;
template
class MagicSocket<Domain::UNIX, Type::STREAM>;


class BufferedSocket : public bylSocket::Socket {
public:

    BufferedSocket(Domain d, Type t);
    BufferedSocket(const Socket o);
    BufferedSocket(const BufferedSocket &o);
    BufferedSocket &operator=(const BufferedSocket &o);
    BufferedSocket(BufferedSocket &&o);
    BufferedSocket &operator=(BufferedSocket &&o);
    virtual ~BufferedSocket() {}

    void fsend(const char *format, ...);
    void send(const char *str);
    const char *recv(int n = BUFSZ - 1);

protected:
    static const int BUFSZ = 512;
    char m_buff[BUFSZ];
};

/**
 * Convenient Listened Socket wrapper
 */
class ListenedSocket : public Socket {
public:
    ListenedSocket(Domain d = Domain::IP4,
                 const char *port = "50000",
                 const char *local = "127.0.0.1",
                 int backlog = 50);


    // for Unix type
    explicit ListenedSocket(const char *local, int backlog = 50);

};

}// bylSocket

#endif /* BYLSOCKET_HPP_ */
