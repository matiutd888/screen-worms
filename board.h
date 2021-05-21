//
// Created by mateusz on 20.05.2021.
//

#ifndef ZADANIE_2_BOARD_H
#define ZADANIE_2_BOARD_H


#include <cstdint>

class Board {
    using board_dim_t = uint32_t;

    static constexpr board_dim_t DEFAULT_BOARD_WIDTH = 640;
    static constexpr board_dim_t DEFAULT_BOARD_HEIGHT = 480;

    board_dim_t width, height;

    int n_players;
};


#endif //ZADANIE_2_BOARD_H
