//
// Created by mateusz on 19.05.2021.
//

#ifndef ZADANIE_2_Server_H
#define ZADANIE_2_Server_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <map>
#include <set>
#include "err.h"
#include "PollUtils.h"
#include "Game.h"
#include "Packet.h"
#include <iostream>
#include <netinet/tcp.h>

enum IPaddressType {
    NoIP = 0, IPv4 = 1, IPv6 = 2
};

class Socket {
protected:
    int socknum;
public:
    Socket() = default;

    [[nodiscard]] int getSocket() const {
        return socknum;
    }

    void doClose() const {
        if (close(socknum) != 0)
            syserr("Close error!");
    }
};

class UDPServerSocket : public Socket {
public:
    explicit UDPServerSocket(uint16_t portNum, const char *addrName = NULL);
};

class UDPClientSocket : public Socket {
    struct addrinfo addrInfo;
public:
    UDPClientSocket(uint16_t portNum, const char *addrName) : Socket() {
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo(addrName, std::to_string(portNum).c_str(), &hints, &servinfo)) != 0) {
            syserr("getaddrinfo");
        }
        debug_out_1 << "UDP: Trying to loop through IPS" << std::endl;
        // loop through all the results and make a socket
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("talker: socket");
                continue;
            }
            break;
        }

        if (p == NULL) {
            syserr("talker: failed to create socket");
        }
        addrInfo = *p;
        socknum = sockfd;
        freeaddrinfo(servinfo);
    }

    const struct addrinfo &getAddrInfo() {
        return addrInfo;
    };
};

class TCPClientSocket : public Socket {
public:
    explicit TCPClientSocket(uint16_t portNum, const char *addrName = NULL);
};

// get sockaddr, IPv4 or IPv6:
inline void *getInAddr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

class Client {
    uint16_t portNum;
    std::string ipAddress;
    IPaddressType ipType;
    struct sockaddr_storage sockaddrStorage{};

public:
    // We do not compare session ID.
    bool operator<(const Client &rhs) const;

    bool operator==(const Client &rhs) const {
        return ipType == rhs.ipType && ipAddress == rhs.ipAddress && portNum == rhs.portNum;
    }

    [[nodiscard]] const sockaddr_storage &getSockaddrStorage() const {
        return sockaddrStorage;
    }

    friend std::ostream &operator<<(std::ostream &os, const Client &client) {
        os << " ipAddress: " << client.ipAddress << "portNum: " << client.portNum;
        return os;
    }

    Client() {
        ipType = IPaddressType::NoIP;
    };

    explicit Client(const struct sockaddr_storage &clientAddress);
};

#endif //ZADANIE_2_Server_H
