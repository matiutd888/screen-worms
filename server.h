//
// Created by mateusz on 19.05.2021.
//

#ifndef ZADANIE_2_SERVER_H
#define ZADANIE_2_SERVER_H
namespace server_constants {
    using port_num_t = uint16_t;
    constexpr port_num_t DEFAULT_PORT_NUM = 2021;
    const char *DEFAULT_GUI_SERVER_NAME = "localhost";
    using board_dim_t = uint32_t;
    constexpr board_dim_t DEFAULT_BOARD_WIDTH = 640;
    constexpr board_dim_t DEFAULT_BOARD_HEIGHT = 480;
    const char *optstring = "p:s:t:v:w:h:";
}
class Server {

};

#endif //ZADANIE_2_SERVER_H
