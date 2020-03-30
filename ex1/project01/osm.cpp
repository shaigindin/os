#include "osm.h"
#include <iostream>
#include <sys/time.h>


int empty_func() {
    return 0;
}

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval time_before_op{};
    struct timeval time_after_op{};
    gettimeofday(&time_before_op , NULL);
    unsigned  int x=15;
    for (unsigned int i = 0; i <= iterations; ++i)
    {
        x += x + 69; // operation
    }
    gettimeofday(&time_after_op , NULL);
    double returnval = (double)(( (unsigned long)time_after_op.tv_usec - (unsigned long)time_before_op.tv_usec) * 1000)
                       / (double)iterations ;
    return returnval;
}


/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval time_before_op{};
    struct timeval time_after_op{};
    gettimeofday(&time_before_op , NULL);
    for (unsigned  int i = 0; i <= iterations; ++i)
    {
        empty_func(); // operation
    }
    gettimeofday(&time_after_op , NULL);
    double returnval = (double)(( (unsigned long)time_after_op.tv_usec - (unsigned long)time_before_op.tv_usec) * 1000)
                       / (double)iterations ;
    return returnval;
}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations){
    if (iterations == 0){
        return -1;
    }
    struct timeval time_before_op{};
    struct timeval time_after_op{};
    gettimeofday(&time_before_op , NULL);
    for (unsigned  int i = 0; i <= iterations; ++i)
    {
        OSM_NULLSYSCALL; // operation
    }
    gettimeofday(&time_after_op , NULL);
    double returnval = (double)(( (unsigned long)time_after_op.tv_usec - (unsigned long)time_before_op.tv_usec) * 1000)
            / (double)iterations ;
    return returnval;
}

