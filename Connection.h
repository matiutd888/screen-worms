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
    IPv4 = 0, IPv6 = 1
};

class Socket {
protected:
    int socknum;
public:                                                 // SOCK_DGRAM | SOCK_STREAM
    Socket() = default;

    [[nodiscard]] int getSocket() const {
        return socknum;
    }

    void doClose() const {
        if (close(socknum) != 0)
            syserr("Close error!");
    }

    ~Socket() {
        // doClose(); TODO chyba będę to robił dwa razy.
    }
};

class UDPServerSocket : public Socket {
public:
    UDPServerSocket(uint16_t portNum, const char *addrName = NULL) {
        int sockfd;
        struct addrinfo hints, *servinfo, *p = NULL;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
//        hints.ai_flags = AI_PASSIVE; // use my IP

        if (getaddrinfo(addrName, std::to_string(portNum).c_str(), &hints, &servinfo) != 0) {
            syserr("getaddrinfo");
        }

        // loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("listener: socket");
                continue;
            }
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("listener: bind");
                continue;
            }
            break;
        }

        if (p == NULL)
            syserr("listener: failed to bind socket\n");

        socknum = sockfd;

        freeaddrinfo(servinfo);
    }
};

class UDPClientSocket : public Socket {
    struct addrinfo addrInfo;
public:
    UDPClientSocket(uint16_t portNum, const char *addrName) {
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
        for(p = servinfo; p != NULL; p = p->ai_next) {
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
    TCPClientSocket(uint16_t portNum, const char *addrName = NULL) {
        int sockfd;
        struct addrinfo hints, *servinfo, *p = NULL;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_STREAM;
//        hints.ai_flags = AI_PASSIVE; // use my IP

        if (getaddrinfo(addrName, std::to_string(portNum).c_str(), &hints, &servinfo) != 0) {
            syserr("getaddrinfo");
        }
        debug_out_1 << "TCP: Trying to loop through IPS" << std::endl;
        // loop through all the results and connect to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }

//            // https://stackoverflow.com/questions/31997648/where-do-i-set-tcp-nodelay-in-this-c-tcp-client
//            int yes = 1;
//            int result = setsockopt(sockfd,
//                                    IPPROTO_TCP,
//                                    TCP_NODELAY,
//                                    (char *) &yes,
//                                    sizeof(int));
//            // 1 - on, 0 - off
//            if (result < 0) {
//                perror("client: set sock opt");
//                continue;
//
//            }
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                perror("client: connect tcp");
                close(sockfd);
                continue;
            }

            break;
        }
        if (p == NULL)
            syserr("listener: failed to bind socket\n");

        socknum = sockfd;

        freeaddrinfo(servinfo);
    }
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
    struct sockaddr_storage sockaddrStorage;
public:
    // We do not compare session ID.
    bool operator<(const Client &rhs) const {
        if (ipType != rhs.ipType)
            return ipType < rhs.ipType;

        if (ipAddress != rhs.ipAddress)
            return ipAddress < rhs.ipAddress;

        if (portNum != rhs.portNum)
            return portNum < rhs.portNum;

        return false;
    }

    bool operator==(const Client &rhs) const {
        return ipType == rhs.ipType && ipAddress == rhs.ipAddress && portNum == rhs.portNum;
    }
//
//    uint64_t getSessionID() const {
//        return sessionID;
//    }
//
//    void setSessionId(uint64_t sessionId) {
//        sessionID = sessionId;
//    }

    [[nodiscard]] const sockaddr_storage &getSockaddrStorage() const {
        return sockaddrStorage;
    }

    friend std::ostream &operator<<(std::ostream &os, const Client &client) {
        os << " ipAddress: " << client.ipAddress  << "portNum: " << client.portNum;
        return os;
    }

    Client() = default;

    explicit Client(const struct sockaddr_storage &clientAddress) {
        sockaddrStorage = clientAddress;
        auto *s = reinterpret_cast<sockaddr *>(&sockaddrStorage);
        if (s->sa_family == AF_INET) {
            ipType = IPaddressType::IPv4;
            portNum = ntohs(((struct sockaddr_in *) s)->sin_port);
//            debug_out_1 << "IP TYPE = IPV4" << std::endl;
        } else if (s->sa_family == AF_INET6) {
            ipType = IPaddressType::IPv6;
            portNum = ntohs(((struct sockaddr_in6 *) s)->sin6_port);
//            debug_out_1 << "IP TYPE = IPV6" << std::endl;
        } else {
            syserr("Client: Incorrect sa_family!");
        }

        char buffIP[INET6_ADDRSTRLEN];
        if (inet_ntop(s->sa_family, getInAddr(s), buffIP, sizeof(buffIP)) == NULL) {
            syserr("Client: Error during inet_ntop!");
        }

        ipAddress = std::string(buffIP);
    }
};

#endif //ZADANIE_2_Server_H
