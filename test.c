#include <stdio.h>
#include <string.h>
#include "my_vm.h"

#define size 2

void printMyMatrix(void *mat)
{
    for(int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            int printVal;
            GetVal(mat + sizeof(int) * (i * size + j), &printVal, sizeof(int));
            printf("%d\t", printVal);
        }
        printf("\n");
    }
}


int main()
{
    int matsize = size*size;
    int mat1[] = {1, 2, 3, 4};
    int mat2[] = {4, 5, 6 ,7};
    void *answer = myalloc(matsize*sizeof(int));

    void *mymat1 = myalloc(matsize*sizeof(int));
    void *mymat2 = myalloc(matsize*sizeof(int));

    for(int i = 0; i < matsize; i++)
    {
        PutVal(mymat1 + i*sizeof(int), &mat1[i], sizeof(int));
        PutVal(mymat2 + i*sizeof(int), &mat2[i], sizeof(int));
    }
    printMyMatrix(mymat1);    
    printf("\n");
    printMyMatrix(mymat2);    
    printf("\n");
    MatMult(mymat1, mymat2, size, answer);
    printMyMatrix(answer);


    

    return 0;
}