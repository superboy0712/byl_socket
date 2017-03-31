//
// Created by yulong on 3/31/17.
//

#ifndef BYLSOCKET_COMMON_H
#define BYLSOCKET_COMMON_H
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
    REUSEADDR = SO_REUSEADDR,
    REUSEPORT = SO_REUSEPORT,
    KEEPALIVE = SO_KEEPALIVE,
    RCVTIMEO = SO_RCVTIMEO,
    SNDTIMEO = SO_SNDTIMEO
};

}

#endif //BYLSOCKET_COMMON_H
