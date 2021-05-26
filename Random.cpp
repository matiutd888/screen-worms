//
// Created by mateusz on 22.05.2021.
//
#include <cstdint>
#include "Random.h"

uint32_t Random::generate() {
    uint32_t ret = r;
    uint64_t calc = r * MULTIPLY_CONST;
    r = calc % MODULO_CONST;
    return ret;
}
