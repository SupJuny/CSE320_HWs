#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "debug.h"

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    // TO BE IMPLEMENTED
    if (global_options & 2)
        if(read_distance_data(stdin) == 0)
            if (build_taxonomy(stdout) == 0)
                if (emit_newick_format(stdout) == 0)
                    return EXIT_SUCCESS;

    if (global_options & 4)
        if(read_distance_data(stdin) == 0)
            if (build_taxonomy(stdout) == 0)
                if (emit_distance_matrix(stdout) == 0)
                    return EXIT_SUCCESS;

    if (global_options == 0)
        if (read_distance_data(stdin) == 0)
            if(build_taxonomy(stdout) == 0)
                return EXIT_SUCCESS;

    // printf("I failed :( \n");
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
