#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;

    printf("%f\n", *ptr);

    sf_free(ptr);


    // void *x = sf_malloc(sizeof(long double) * 9);
    // sf_show_heap();
    // void *y = sf_malloc(100);
    // sf_show_heap();
    // void *z = sf_realloc(x, PAGE_SZ);
    // double frag = sf_fragmentation();
    // printf("\n frag %f \n", frag);
    // sf_show_heap();

    // sf_free(y);
    // sf_show_heap();
    // sf_free(z);
    // sf_show_heap();


    // double util = sf_utilization();
    // printf("\n util %f \n", util);

    return EXIT_SUCCESS;
}
