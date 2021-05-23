//
// Created by mateusz on 21.05.2021.
//

#ifndef ZADANIE_2_PACKET_H
#define ZADANIE_2_PACKET_H


#include <cstring>

class Packet {
    static constexpr int MAX_SIZE = 550;
    char buffer[MAX_SIZE];
    size_t offset;
    bool isOK;
public:
    Packet() {
        memset(buffer, 0, MAX_SIZE);
        offset = 0;
        isOK = true;
    }

    void clearPacket() {
        memset(buffer, 0, MAX_SIZE);
        offset = 0;
    }

    size_t getSize() {
        return MAX_SIZE;
    }

    size_t getRemainingSize() {
        return MAX_SIZE - offset;
    }

    size_t getOffset() {
        return offset;
    }

    char *getBufferWithOffset(size_t customOffset) {
        return (buffer + customOffset);
    }

    char *getBuffer() {
        return buffer;
    }

    void setOffset(size_t offset) {
        this->offset = offset;
    }

    bool write(const void *data, size_t dataSize) {
        if (offset + dataSize >= MAX_SIZE)
            return false;

        memcpy(buffer + offset, data, dataSize);

        return true;
    }

    void errorOccured() {
        isOK = false;
    }
};

#endif //ZADANIE_2_PACKET_H
