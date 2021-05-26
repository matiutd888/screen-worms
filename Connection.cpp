#include <iostream>
#include <bits/getopt_posix.h>


#include "Connection.h"

bool Client::operator<(const Client &rhs) const {
    if (ipType != rhs.ipType)
        return ipType < rhs.ipType;

    if (ipAddress != rhs.ipAddress)
        return ipAddress < rhs.ipAddress;

    if (portNum != rhs.portNum)
        return portNum < rhs.portNum;

    return false;
}

Client::Client(const sockaddr_storage &clientAddress) {
    sockaddrStorage = clientAddress;
    auto *s = reinterpret_cast<sockaddr *>(&sockaddrStorage);
    if (s->sa_family == AF_INET) {
        ipType = IPaddressType::IPv4;
        portNum = ntohs(((struct sockaddr_in *) s)->sin_port);
    } else if (s->sa_family == AF_INET6) {
        ipType = IPaddressType::IPv6;
        portNum = ntohs(((struct sockaddr_in6 *) s)->sin6_port);
    } else {
        syserr("Client: Incorrect sa_family!");
    }
    char buffIP[INET6_ADDRSTRLEN];
    if (inet_ntop(s->sa_family, getInAddr(s), buffIP, sizeof(buffIP)) == NULL) {
        syserr("Client: Error during inet_ntop!");
    }
    ipAddress = std::string(buffIP);
}

TCPClientSocket::TCPClientSocket(uint16_t portNum, const char *addrName) : Socket() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p = NULL;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM;

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

//            https://stackoverflow.com/questions/31997648/where-do-i-set-tcp-nodelay-in-this-c-tcp-client
        int yes = 1;
        int result = setsockopt(sockfd,
                                IPPROTO_TCP,
                                TCP_NODELAY,
                                (char *) &yes,
                                sizeof(int));
        // 1 - on, 0 - off
        if (result < 0) {
            perror("client: set sock opt");
            continue;
        }

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

UDPServerSocket::UDPServerSocket(uint16_t portNum, const char *addrName) : Socket() {
    int sockfd;
    sockaddr_in6 server_address;
    server_address.sin6_family = AF_INET6;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(portNum);

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
        syserr("Error calling socket");

    if (bind(sockfd, reinterpret_cast<const sockaddr *>(&server_address), sizeof(server_address)) < 0)
        syserr("bind!");
//    struct addrinfo hints, *servinfo, *p = NULL;
//    memset(&hints, 0, sizeof hints);
//    hints.ai_family = AF_UNSPEC; // set to AF_INET to use IPv4
//    hints.ai_socktype = SOCK_DGRAM;
////    hints.ai_family.
//    if (getaddrinfo(addrName, std::to_string(portNum).c_str(), &hints, &servinfo) != 0) {
//        syserr("getaddrinfo");
//    }
//
//    // loop through all the results and bind to the first we can
//    for (p = servinfo; p != NULL; p = p->ai_next) {
//        if ((sockfd = socket(p->ai_family, p->ai_socktype,
//                             p->ai_protocol)) == -1) {
//            perror("listener: socket");
//            continue;
//        }
//        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
//            close(sockfd);
//            perror("listener: bind");
//            continue;
//        }
//        break;
//    }
//
//    if (p == NULL)
//        syserr("listener: failed to bind socket\n");

    socknum = sockfd;
}
