//
// Created by mateusz on 21.05.2021.
//

#ifndef ZADANIE_2_POLLUTILS_H
#define ZADANIE_2_POLLUTILS_H

#include <sys/poll.h>
#include <sys/timerfd.h>
#include "err.h"
#include "Utils.h"

class PollUtils {
    static constexpr int N_DESC = 3;
public:
    static constexpr int MESSAGE_CLIENT = 0;
    static constexpr int TIMEOUT_CLIENT = 1;
    static constexpr int TIMEOUT_ROUND = 2;
private:
    static constexpr int TIMEOUT_MS = 2000;

    struct pollfd poll_data[N_DESC];

    PollUtils(int socket, int rounds_per_sec) {
        poll_data[MESSAGE_CLIENT].fd = socket;
        poll_data[MESSAGE_CLIENT].events = POLLIN;

        poll_data[TIMEOUT_CLIENT].fd = get_timeout_fd(Utils::TIMEOUT_CLIENTS_SEC / 2, 0);
        poll_data[TIMEOUT_CLIENT].events = POLLIN;

        long nano_timeout_rounds = 1e9 / rounds_per_sec;
        poll_data[TIMEOUT_ROUND].fd = get_timeout_fd(0, nano_timeout_rounds);
        poll_data[TIMEOUT_ROUND].events = POLLIN;
    }

    static int get_timeout_fd(__time_t tv_sec, long tv_nsec) {
        int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (fd < 0)
            syserr("timerfd_create");


        struct itimerspec interval;
        interval.it_interval.tv_sec = tv_sec;
        interval.it_interval.tv_nsec = tv_nsec;
        interval.it_value.tv_sec = tv_sec;
        interval.it_value.tv_nsec = tv_nsec;

        if (timerfd_settime(fd, 0, &interval, NULL) < 0)
            syserr("timerfd_settime");
        return fd;
    }

public:
    int do_poll() {
        return poll(poll_data, N_DESC, TIMEOUT_MS);
    }

    bool has_pollin_occurr(int index) {
        return poll_data[index].revents & POLLIN;
    }

    bool check_error() {
        for(int i = 0; i < N_DESC; i++) {
            if (poll_data[i].revents & POLLERR) {
                return true;
            }
        }
        return false;
    }

    int get_number_of_expirations(int index);
};


#endif //ZADANIE_2_POLLUTILS_H
