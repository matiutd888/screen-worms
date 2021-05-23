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
    static constexpr int TIMEOUT_MS = 2000;
    struct pollfd pollData[N_DESC];
public:
    static constexpr int MESSAGE_CLIENT = 0;
    static constexpr int TIMEOUT_CLIENT = 1;
    static constexpr int TIMEOUT_ROUND = 2;

    PollUtils() = default; // A bad practise of sorts

    PollUtils(int socket, int roundsPerSec) {
        pollData[MESSAGE_CLIENT].fd = socket;
        pollData[MESSAGE_CLIENT].events = POLLIN;

        if (Utils::NUMBER_OF_TICKS > Utils::TIMEOUT_CLIENTS_SEC) {
            long nanoTimeoutClients = Utils::TIMEOUT_CLIENTS_SEC * 1e9 / Utils::NUMBER_OF_TICKS;
            pollData[TIMEOUT_CLIENT].fd = getTimeoutFd(0, nanoTimeoutClients);
        } else {
            pollData[TIMEOUT_CLIENT].fd = getTimeoutFd(Utils::TIMEOUT_CLIENTS_SEC / Utils::NUMBER_OF_TICKS, 0);
        }

        pollData[TIMEOUT_CLIENT].events = POLLIN;

        if (roundsPerSec > 1) {
            long nanoTimeoutRounds = 1e9 / roundsPerSec;
            pollData[TIMEOUT_ROUND].fd = getTimeoutFd(0, nanoTimeoutRounds);
        } else {
            pollData[TIMEOUT_ROUND].fd = getTimeoutFd(1, 0);
        }
        pollData[TIMEOUT_ROUND].events = POLLIN;
    }
private:
    static int getTimeoutFd(time_t tv_sec, long tv_nsec) {
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
    int doPoll() {
        return poll(pollData, N_DESC, TIMEOUT_MS);
    }

    bool hasPollinOccurred(int index) {
        return pollData[index].revents & POLLIN;
    }

    bool checkError() {
        for(int i = 0; i < N_DESC; i++) {
            if (pollData[i].revents & POLLERR) {
                return true;
            }
        }
        return false;
    }

    int getNumberOfExpirations(int index) {
        return 0;
    }

    int getDescriptor(int index) {
        return pollData[index].fd;
    }

    ~PollUtils() {
        //TODO
//        for (auto & i : pollData) {
//            if (close(i.fd)) {
//                syserr("Poll data close!");
//            }
//        }
    }
};


#endif //ZADANIE_2_POLLUTILS_H
