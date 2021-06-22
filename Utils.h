//
// Created by mateusz on 20.05.2021.
//

#ifndef ZADANIE_2_UTILS_H
#define ZADANIE_2_UTILS_H


#include <cstdint>
#include <string>
#include <iostream>

#define debug_out_1 if(Utils::debug_1)std::cout
#define debug_out_0 if(Utils::debug_0)std::cout

namespace Utils {
    const static int debug_1 = false;
    const static int debug_0 = false;
    static constexpr uint16_t DEFAULT_SERVER_PORT_NUM = 2021;
    static inline const std::string DEFAULT_GUI_SERVER_NAME = "localhost";
    static inline const char *const optstring = "p:s:t:v:w:h:";

    static constexpr int TIMEOUT_CLIENTS_SEC = 2;
    static constexpr int DEFAULT_ROUNDS_PER_SEC = 50;
    static constexpr uint32_t DEFAULT_BOARD_WIDTH = 640;
    static constexpr uint32_t DEFAULT_BOARD_HEIGHT = 480;
    static constexpr int DEFAULT_TURINING_SPEED = 6;
    static constexpr int INTERVAL_CLIENT_MESSAGE_MS = 30;
    static constexpr int NUMBER_OF_TICKS = 2;
    static constexpr long SEC_TO_NANOSEC = 1e9;
    static constexpr uint16_t DEFAULT_GUI_PORT_NUM = 20210;
    static constexpr int GAME_CLIENTS_LIMIT = 25;
    static constexpr int TURN_MAX_VALUE = 2;
};


#endif //ZADANIE_2_UTILS_H
