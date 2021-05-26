//
// Created by mateusz on 21.05.2021.
//

#ifndef ZADANIE_2_POLLUTILS_H
#define ZADANIE_2_POLLUTILS_H

#include <sys/poll.h>
#include <sys/timerfd.h>
#include "err.h"
#include "Utils.h"
#include <unistd.h>

class PollUtils {
protected:
    static constexpr int MAX_DESC = 10;
    struct pollfd pollData[MAX_DESC];

    static int getTimeoutFd(time_t tv_sec, long tv_nsec);

private:
    int numberOfDescriptors;
    static constexpr int TIMEOUT_MS = 2000;
public:
    int doPoll() {
        return poll(pollData, numberOfDescriptors, TIMEOUT_MS);
    }

    bool hasPollinOccurred(int index) const;

    bool hasPolloutOccurred(int index) const;


    [[maybe_unused]] bool checkError() const {
        for (int i = 0; i < numberOfDescriptors; i++) {
            if (pollData[i].revents & POLLERR) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] int getDescriptor(int index) const;

    void closeAllDescriptors() {
        for (auto &i : pollData) {
            if (close(i.fd)) {
                syserr("Poll data close!");
            }
        }
    }

protected:
    explicit PollUtils(int numberOfDescriptors) : numberOfDescriptors(numberOfDescriptors) {}
};

class PollServer : public PollUtils {
    static constexpr int N_DESC = 3;
public:
    static constexpr int MESSAGE_CLIENT = 0;
    static constexpr int TIMEOUT_CLIENT = 1;
    static constexpr int TIMEOUT_ROUND = 2;

    PollServer() : PollUtils(N_DESC) {}

    PollServer(int socket, int roundsPerSec);

    void addPolloutToEvents(int index);

    void removePolloutFromEvents(int index);
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
    static long milisecToNano(long millisec) {
        return millisec * 1e6;
    }
};


#endif //ZADANIE_2_POLLUTILS_H
