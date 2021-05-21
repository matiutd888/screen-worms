//
// Created by mateusz on 19.05.2021.
//

#ifndef ZADANIE_2_SERVER_H
#define ZADANIE_2_SERVER_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include "err.h"

class Connection {

    int client_sock;


    Connection(uint16_t port_num) {
        int sockfd;
        struct addrinfo hints, *servinfo, *p = NULL;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if (getaddrinfo(NULL, std::to_string(port_num).c_str(), &hints, &servinfo) != 0) {
            syserr("getaddrinfo");
        }

        // loop through all the results and bind to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
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

        client_sock = sockfd;

        freeaddrinfo(servinfo);

    }


    void do_close() {
        close(client_sock);
    }

    // Non-static const




};

#endif //ZADANIE_2_SERVER_H
