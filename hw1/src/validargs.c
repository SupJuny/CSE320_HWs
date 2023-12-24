#include <stdlib.h>

#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv)
{
    // failure
    if (argc < 1) {
        return -1;
    }

    else if (argc == 1)
        return 0;

    // success
    else {
        if (*(*(argv + 1)) == 45) {  // -

            // -h
            if (*(*(argv + 1)+1) == 104 && *(*(argv + 1)+2) == 0) {
                global_options = HELP_OPTION;
                return 0;
            }

            // -m
            else if (*(*(argv + 1)+1) == 109 && *(*(argv + 1)+2) == 0) {

                // something after -m
                if (argc > 2) {
                    return -1;
                }

                // nothing after -m
                else {
                    global_options = MATRIX_OPTION;
                    return 0;
                }
            }

            // -n
            else if (*(*(argv + 1)+1) == 110 && *(*(argv + 1)+2) == 0) {

                // -n 'none'
                if (argc == 2) {
                    global_options = NEWICK_OPTION;
                    return 0;
                }

                // -n -o
                else if (*(*(argv + 2)) == 45 && *(*(argv + 2)+1) == 111 && *(*(argv + 2)+2) == 0){

                    // -n -o 'none'
                    if (argc == 3) {
                        return -1;
                    }

                    // -n -o something
                    else if (argc == 4) {
                        global_options = NEWICK_OPTION;
                        outlier_name = *(argv + 3);
                        return 0;
                    }

                    else
                        return -1;
                }

                // -n -z
                else {
                    return -1;
                }
            }

            else
                return -1;
        }
    }
    return 0;
}
