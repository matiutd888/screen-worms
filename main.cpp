//
// Created by mateusz on 19.05.2021.
//

#include "Connection.h"
#include "err.h"
int main() {
    Server connectionManager(2138, 3);
    connectionManager.start();
}