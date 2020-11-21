#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//Assume the address space is 32 bits, so the max memory size is 4GB

//Page size is 4KB
#define DEBUG 0

//Add any important includes here which you may need

#define PGSIZE 4096 // 2^12

// Maximum size of your memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024 //4GB

#define MEMSIZE 1024*1024*1024

#define ADDRESS_BIT_LENGTH 32

#define min(X,Y) ((X) < (Y) ? (X) : (Y))
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef pte_t *pde_t;


#define TLB_SIZE 5

typedef struct tlb_entry {
    void *va; // virtual address
    void *pa; // physical address
}tlb_entry;

//Structure to represents TLB
typedef struct tlb {

    //Assume your TLB is a direct mapped TLB of TBL_SIZE (entries)
    // You must also define wth TBL_SIZE in this file.
    //Assume each bucket to be 4 bytes
    
    struct tlb *next;
    struct tlb *prev;

    struct tlb_entry *entry;

} tlb;


void SetPhysicalMem();
pte_t* Translate(pde_t *pgdir, void *va);
int PageMap(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *myalloc(unsigned int num_bytes);
void myfree(void *va, int size);
void PutVal(void *va, void *val, int size);
void GetVal(void *va, void *val, int size);
void MatMult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();

/*
Our functions
*/
/*
char *exceptions[] = {
    "SUCCESS",
    "VIRTUAL ADDRESS IS ALREADY MAPPED"
    };
*/

void SetBitRange(int A[], int a, int b);

void SetBit(int A[], int k); // set the bit at the k-th position in A[i]
void ClearBit(int A[], int k); // clear the bit at the k-th position in A[i]
int TestBit(int A[],  int k); // return value of bit at the k-th position in A[i]
/*
End our functions
*/



#endif
