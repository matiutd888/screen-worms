//
// Created by mateusz on 20.05.2021.
//

#ifndef ZADANIE_2_UTILS_H
#define ZADANIE_2_UTILS_H


#include <cstdint>
#include <string>

class Utils {
    using port_num_t = uint16_t;
public:
    static constexpr port_num_t DEFAULT_PORT_NUM = 2021;
    static inline const std::string DEFAULT_GUI_SERVER_NAME = "localhost";
    static inline const std::string optstring = "p:s:t:v:w:h:";

    static constexpr int TIMEOUT_CLIENTS_SEC = 2;
    static constexpr int DEFAULT_ROUNDS_PER_SEC = 50;

};


#endif //ZADANIE_2_UTILS_H
