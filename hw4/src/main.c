#include <stdlib.h>
#include <setup.h>
#include <string.h>
#include <stdio.h>

#include "deet.h"

int main(int argc, char *argv[]) {
    // TO BE IMPLEMENTED
    // Remember: Do not put any functions other than main() in this file.
    int rm_deet = 0;
    if (argc == 2)
        if (!strcmp(argv[1], "-p")) {
            rm_deet = 1;
        }
    int i = set_up(rm_deet);

    return i;
    //if(*argv[1] == "help")
    // abort();
}
