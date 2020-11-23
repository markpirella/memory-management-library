#include <stdio.h>
#include <string.h>
#include "my_vm.h"
#include <sys/time.h>
#include <sys/types.h>

#define size 5000
#define TEST_ITER 100000

int main()
{

    void *mat = myalloc(sizeof(int)*size);
    for(int i = 0; i < size; i++)
    {
        PutVal(mat, &i, sizeof(int));
    }

    struct timeval before;
    struct timeval after;
    double totalTime = 0;

    gettimeofday(&before, NULL);
    srand(before.tv_usec);
    for(int i = 0; i < TEST_ITER; i++)
    {
        gettimeofday(&before, NULL);
        int store;
        int index = rand() % size;
        GetVal(mat + index*sizeof(int), &store, sizeof(int));
        gettimeofday(&after, NULL);

        double timeOneIter = (after.tv_sec*1e6 + after.tv_usec) - (before.tv_sec*1e6 + before.tv_usec);
        totalTime += timeOneIter;
    }

    printf("TLB_SIZE: %d\tNumber of Accesses: %d\tAverage access time: %f usec\n", TLB_SIZE, TEST_ITER, (totalTime)/TEST_ITER);

    return 0;
}