//
// Created by mateusz on 22.05.2021.
//

#ifndef ZADANIE_2_RANDOM_H
#define ZADANIE_2_RANDOM_H

#endif //ZADANIE_2_RANDOM_H

class Random {
    uint32_t r;
    static constexpr uint32_t MULTIPLY_CONST = 279410273;
    static constexpr uint32_t MODULO_CONST = 4294967291;
public:
    Random(uint32_t seed) {
        r = seed;
    };

    uint32_t generate();
};