//
// Created by mateusz on 19.05.2021.
//

#include "Utils.h"
#include "DataBuilders.h"
#include "Server.h"

//./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]
//
//* `-p n` – numer portu (domyślnie `2021`)
//* `-s n` – ziarno generatora liczb losowych (opisanego poniżej, domyślnie
//wartość uzyskana przez wywołanie `time(NULL)`)
//* `-t n` – liczba całkowita wyznaczająca szybkość skrętu
//(parametr `TURNING_SPEED`, domyślnie `6`)
//* `-v n` – liczba całkowita wyznaczająca szybkość gry
//(parametr `ROUNDS_PER_SEC` w opisie protokołu, domyślnie `50`)
//* `-w n` – szerokość planszy w pikselach (domyślnie `640`)
//* `-h n` – wysokość planszy w pikselach (domyślnie `480`)
int main(int argc, char **argv) {
    ServerDataBuilder builder;
    builder.parse(argc, argv);
    auto data = builder.build();
    std::cout << data << std::endl;
    Server server(data);
    server.start();
}