//
// Created by mateusz on 21.05.2021.
//

#ifndef ZADANIE_2_PACKET_H
#define ZADANIE_2_PACKET_H


#include <cstring>
#include "err.h"
#include "Utils.h"
#include <iostream>

class Packet {
protected:
    static constexpr int MAX_SIZE = 550;
    char buffer[MAX_SIZE];
public:
    class PacketToSmallException : std::exception {
        const char * what() const noexcept override {
            return "PACKET TO SMALL TO PERFORM READ / WRITE ";
        }
    };

    class FatalEncodingException : std::exception {
    public:
        const char *what() const noexcept override {
            return "Parse exception occured!";
        }
    };

    class FatalDecodingException : std::exception {
    public:
        const char *what() const noexcept override {
            return "Parse exception occured!";
        }
    };
    static size_t getMaxSize() {
        return MAX_SIZE;
    }
    const char *getBufferConst() const {
        return buffer;
    }

    const char *getBufferWithOffsetConst(size_t customOffset) const {
        return (buffer + customOffset);
    }
};

class WritePacket : public Packet {
    size_t offset;
    static inline std::string TAG = "WritePacket: ";
public:
    WritePacket() {
        memset(buffer, 0, MAX_SIZE);
        offset = 0;
    }

    void clearPacket() {
        memset(buffer, 0, MAX_SIZE);
        offset = 0;
    }

    [[nodiscard]] size_t getRemainingSize() const {
        return MAX_SIZE - offset;
    }

    [[nodiscard]] size_t getOffset() const {
        return offset;
    }
//
//    char *getBufferWithOffset(size_t customOffset) {
//        return (buffer + customOffset);
//    }

    // Throws @ParseException
    void write(const void *data, size_t dataSize) {
        if (offset + dataSize >= MAX_SIZE)
            throw Packet::FatalEncodingException();
//        std::cout << TAG << "Writing " << dataSize << " bytes to packet of size " << getRemainingSize() << " and offset " << offset << std::endl;

        memcpy(buffer + offset, data, dataSize);
        offset += dataSize;
    }

};

class ReadPacket : public Packet {
    bool isOK;
    size_t size;
    size_t offset;
public:
    ReadPacket() {
        isOK = true;
        offset = 0;
    }
    void errorOccured() {
        isOK = false;
    }

    char *getBuffer() {
        return buffer;
    }

    void setSize(size_t size) {
        if (size > MAX_SIZE)
            syserr("Setting size of packet bigger than maxSize");
        this->size = size;
    }

    [[nodiscard]] size_t getRemainingSize() const {
        return size - offset;
    }

    void readData(void *dst, size_t dataSize) {
//        std::cout << TAG << "Reading " << dataSize << " bytes from packet of size " << size << " and offset " << offset << std::endl;
        if (getRemainingSize() < dataSize)
            throw Packet::FatalDecodingException();
        memcpy(dst, buffer + offset, dataSize);
        offset += dataSize;
    }

    bool isok() {
        return isOK;
    }

    static inline std::string TAG = "ReadPacket: ";
};

#endif //ZADANIE_2_PACKET_H
