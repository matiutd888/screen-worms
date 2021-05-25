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
protected:
    static constexpr int MAX_DESC = 10;
    struct pollfd pollData[MAX_DESC];

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

private:
    int numberOfDescriptors;
    static constexpr int TIMEOUT_MS = 2000;
public:
    int doPoll() {
        return poll(pollData, numberOfDescriptors, TIMEOUT_MS);
    }

    bool hasPollinOccurred(int index) {
        if (index >= numberOfDescriptors) {
            syserr("invalid Poll index!");
        }
        return pollData[index].revents & POLLIN;
    }

    bool hasPolloutOccurred(int index) {
        if (index >= numberOfDescriptors) {
            syserr("invalid Poll index!");
        }
        return pollData[index].revents & POLLOUT;
    }


    bool checkError() {
        for (int i = 0; i < numberOfDescriptors; i++) {
            if (pollData[i].revents & POLLERR) {
                return true;
            }
        }
        return false;
    }

    int getDescriptor(int index) const {
        if (index >= numberOfDescriptors) {
            syserr("invalid Poll index!");
        }
        return pollData[index].fd;
    }

    int getNumberOfExpirations(int index) {
        return 0;
    }

    ~PollUtils() {
        //TODO
//        for (auto & i : pollData) {
//            if (close(i.fd)) {
//                syserr("Poll data close!");
//            }
//        }
    }

protected:
    PollUtils(int numberOfDescriptors) : numberOfDescriptors(numberOfDescriptors) {}
};

class PollServer : public PollUtils {
    static constexpr int N_DESC = 3;
public:
    static constexpr int MESSAGE_CLIENT = 0;
    static constexpr int TIMEOUT_CLIENT = 1;
    static constexpr int TIMEOUT_ROUND = 2;

    PollServer() : PollUtils(N_DESC) {} // A bad practise of sorts

    PollServer(int socket, int roundsPerSec) : PollUtils(N_DESC) {
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
            long nanoTimeoutRounds = Utils::SEC_TO_NANOSEC / roundsPerSec;
            pollData[TIMEOUT_ROUND].fd = getTimeoutFd(0, nanoTimeoutRounds);
        } else {
            pollData[TIMEOUT_ROUND].fd = getTimeoutFd(1, 0);
        }
        pollData[TIMEOUT_ROUND].events = POLLIN;
    }

    void addPolloutToEvents(int index) {
        pollData[index].events |= POLLOUT;
    }

    void removePolloutFromEvents(int index) {
        pollData[index].events = POLLIN;
    }
};

class PollClient : public PollUtils {
    static constexpr int N_DESC = 3;
public:
    static constexpr int MESSAGE_SERVER = 0;
    static constexpr int INTERVAL_SENDMESSAGE = 1;
    static constexpr int MESSAGE_GUI = 2;

    PollClient() : PollUtils(N_DESC) {};

    PollClient(int serverfd, int guifd) : PollUtils(N_DESC) {
        pollData[MESSAGE_SERVER].fd = serverfd;
        pollData[MESSAGE_SERVER].events = POLLIN;
        pollData[MESSAGE_GUI].fd = guifd;
        pollData[MESSAGE_GUI].events = POLLIN;

        long nano = milisecToNano(Utils::INTERVAL_CLIENT_MESSAGE_MS);
        if (nano >= Utils::SEC_TO_NANOSEC) {
            syserr("PollClient: Sending to slow!");
        }
        pollData[INTERVAL_SENDMESSAGE].fd = getTimeoutFd(0, nano);
        pollData[INTERVAL_SENDMESSAGE].events = POLLIN;
    }



private:
    long milisecToNano(long milisec) {
        return milisec * 1e6;
    }

};


#endif //ZADANIE_2_POLLUTILS_H
