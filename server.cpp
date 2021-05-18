#include <iostream>
#include <bits/getopt_posix.h>




int main(int argc, char **argv) {
    int *p = (int*)malloc(10te);
    char c;
    while ((c = getopt(argc, argv, optstring)) != -1) {
        switch (c) {
            default:
//                printf("dupa\n");
               printf("%s\n", optarg);
        }
    }
    return 0;
}
