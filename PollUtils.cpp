//
// Created by mateusz on 21.05.2021.
//

#include "PollUtils.h"

int PollUtils::getTimeoutFd(time_t tv_sec, long tv_nsec) {
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

bool PollUtils::hasPollinOccurred(int index) const {
    if (index >= numberOfDescriptors) {
        syserr("invalid Poll index!");
    }
    return pollData[index].revents & POLLIN;
}

int PollUtils::getDescriptor(int index) const {
    if (index >= numberOfDescriptors) {
        syserr("invalid Poll index!");
    }
    return pollData[index].fd;
}

bool PollUtils::hasPolloutOccurred(int index) const {
    if (index >= numberOfDescriptors) {
        syserr("invalid Poll index!");
    }
    return pollData[index].revents & POLLOUT;
}

PollServer::PollServer(int socket, int roundsPerSec) : PollUtils(N_DESC) {
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

void PollServer::addPolloutToEvents(int index) {
    pollData[index].events |= POLLOUT;
}

void PollServer::removePolloutFromEvents(int index) {
    pollData[index].events = POLLIN;
}
