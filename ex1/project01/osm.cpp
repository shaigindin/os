#include "osm.h"
#include <iostream>
#include <sys/time.h>

// constants
#define UNROLLING_FAC 10


int empty_func() {
    return 0;
}

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations){
    int var = 1;
    if (iterations == 0){
        return -1;
    }
    struct timeval time_before_op;
    struct timeval time_after_op;
    gettimeofday(&time_before_op , NULL);
    for (unsigned int i = 0; i < iterations / UNROLLING_FAC; i += UNROLLING_FAC)
    {
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
        var += 20; // operation
    }
    gettimeofday(&time_after_op , NULL);
    return (((1e9 * (double) (time_after_op.tv_sec - time_before_op.tv_sec)) +
    1000 * (double) (time_after_op.tv_usec - time_before_op.tv_usec)) / iterations);
}


/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval time_before_op;
    struct timeval time_after_op;
    gettimeofday(&time_before_op , NULL);
    for (unsigned int i = 0; i < iterations / UNROLLING_FAC; i += UNROLLING_FAC)
    {
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
        empty_func(); // operation
    }
    gettimeofday(&time_after_op , NULL);
    return (double) (((1e9 * (double) (time_after_op.tv_sec - time_before_op.tv_sec)) +             
    1000 * (double) (time_after_op.tv_usec - time_before_op.tv_usec)) / iterations);
}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval time_before_op;
    struct timeval time_after_op;
    gettimeofday(&time_before_op , NULL);
    for (unsigned int i = 0; i < iterations / UNROLLING_FAC; i += UNROLLING_FAC)
    {
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
        OSM_NULLSYSCALL; // operation
    }
    gettimeofday(&time_after_op , NULL);
    return (double) (((1e9 * (double) (time_after_op.tv_sec - time_before_op.tv_sec)) +             
    1000 * (double) (time_after_op.tv_usec - time_before_op.tv_usec)) / iterations);
}
