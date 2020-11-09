#include "my_vm.h"

bitmap_t pdeBitmap;    // Bitmap with bits set to retrieve page directory
bitmap_t pteBitmap;    // Bitmap with bits set to retrieve page table from directory
bitmap_t offsetBitmap; // Bitmap with bits set to retrieve page offset

int *physBitmap;
int *virtBitmap;

pde_t *pageDir; // pointer to page directory (array of page directory entries)

void *physMem = NULL; // void pointer to point to the start of allocated physical memory

size_t numVirtPages = 0; // stores total number of virtual pages
size_t numPhysPages = 0; // stores total number of physical pages

int numOffsetBits = 0; // stores the number of bits used to calculate the offset in virtual addresses (ex. bits 11:0 for PGSIZE = 4096)
int numOuterIndexBits = 0; // stores the number of bits used to calculate the outer index (index of page dir) in virtual addresses (ex. bits 21:12 for PGSIZE = 4096)
int numInnerIndexBits = 0; // stores the number of bits used to calculate the inner index (index of page table) in virtual addresses (ex. bits 31:22 for PGSIZE = 4096)
int numPageDirEntries = 0; // stores the number of page dir entries for easy traversal through array

int initialized = 0; // stores value to help determine if physical memory has been initialized yet

pde_t *PageDirectories;


/*
Function responsible for allocating and setting your physical memory
*/
void SetPhysicalMem() {
    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

    // allocate physical memory and set the void pointer physMem to the start of the physical memory
    physMem = malloc(MEMSIZE);
    // store appropriate number of virtual pages
    numVirtPages = MAX_MEMSIZE / PGSIZE;

    // store appropriate number of physical pages
    numPhysPages = MEMSIZE / PGSIZE;

    // set number of offset bits to log-base-2 of PGSIZE (ex. 12 for PGSIZE = 4096)
    numOffsetBits = log(PGSIZE) / log(2);

    // set number of outer index bits to half of bitspace remaining for use in the virtual address (ex. 10 for PGSIZE = 4096)
    numOuterIndexBits = (32 - numOffsetBits) / 2;

    // set number of outer index bits to the number of bits remaining for use in the virtual address (ex. 10 for PGSIZE = 4096)
    numInnerIndexBits = 32 - (numOffsetBits + numOuterIndexBits);

    // store number of page dir entries based on number of outer index bits
    numPageDirEntries = 2^numOuterIndexBits;

    // determine number of elements needed in virtual bitmap (bit array)
    int bitmapLength = numVirtPages / 32;
    if(numVirtPages % 32 != 0){
        bitmapLength++;
    }

    // allocate and initialize virtual bitmap
    int temp[bitmapLength];
    virtBitmap = temp;
    int i;
    for(i = 0; i < bitmapLength; i++){
        virtBitmap[i] = 0;
    }

    // determine number of elements needed in physical bitmap (bit array)
    bitmapLength = numPhysPages / 32;
    if(numPhysPages % 32 != 0){
        bitmapLength++;
    }

    // allocate and initialize physical bitmap
    int temp2[bitmapLength];
    physBitmap = temp2;
    for(i = 0; i < bitmapLength; i++){
        physBitmap[i] = 0;
    }

    // allocate and initialize page directory

    pde_t temp3[numPageDirEntries];
    pageDir = temp3;
    for(i = 0; i < numPageDirEntries; i++){
        pageDir[i] = 0;
    }

    //printf("outer bits: %d, inner bits: %d, offset bits: %d\n", numOuterIndexBits, numInnerIndexBits, numOffsetBits);

    /* bitmap debugging/testing
    SetBit(physBitmap+1, 0);
    SetBit(physBitmap+1, 1);
    SetBit(physBitmap+1, 2);
    SetBit(physBitmap+1, 3);
    printf("%x\n", physBitmap[1] & 0xf);

    /*
    printf("physical bitarray length (should be 8192): %d\n", bitmapLength);
    printf("first elements in physBitmap: %x\n", physBitmap[1]);
    */

    

    // set initialized flag to 1
    initialized = 1;
}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;

    /*Part 2 Code here to calculate and print the TLB miss rate*/




    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}


/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {
    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

    /*
    int pdir_index = -1;
    int i;
    for(i = 0; i < numPageDirEntries; i++){ // loop through page directory until you find desired entry (pgdir)
        if(pageDir[i] == pgdir){
            pdir_index = i;
            break;
        }
    }
    if(pdir_index < 0 ){ // pde not found, so return null
        return NULL;
    }
    // otherwise, page directory was found so continue on
    */ //i dont think the above is right, misread the instructions ^^^^^^^^

    int pDirMask = 0;
    int i;
    for(i = numOffsetBits + numInnerIndexBits; i < 32; i++){
        pDirMask |= (0x1 << i);
    }

    int pTableMask = 0;
    for(i = numOffsetBits; i < 32 - numOuterIndexBits; i++){
        pTableMask |= (0x1 << i);
    }
    
    int pdir_index = ((int)va & pDirMask) >> (numOffsetBits + numInnerIndexBits); // grab outer index bits in address using pDirMask and store as index of pde
    int ptable_index = ((int)va & pTableMask) >> numOffsetBits; // grab inner index bits in address using pTableMask and store as index of pte
    int offset = (int)va & (int)((2^numOffsetBits) - 1); // grab offset bits and store in offset variable

    // *** now, get and return physical address from pagedir[pdir_index] ---> pagetable[ptable_index] ---> offset

    //If translation not successfull
    return NULL;
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
PageMap(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to Translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    return -1;
}


/*Function that gets the next available page
*/
/*
void *get_next_avail(int num_pages) {

    //Use virtual address bitmap to find the next free page
}

**** FUNCTION ABOVE SPLIT INTO THE TWO FUNCTIONS BELOW, AS SUGGESTED ****
*/

/*
Function that gets the next available virtual page
*/
void *get_next_avail_virt(int num_pages) {

    //Use virtual address bitmap to find the next free *virtual* page
}

/*
Function that gets the next available physical page
*/
void *get_next_avail_phys(int num_pages) {

    //Use virtual address bitmap to find the next free *physical* page
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *myalloc(unsigned int num_bytes) {

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

    /* HINT: If the page directory is not initialized, then initialize the
    page directory. Next, using get_next_avail(), check if there are free pages. If
    free pages are available, set the bitmaps and map a new page. Note, you will
    have to mark which physical pages are used. */


    // initialize physical memory using SetPhysicalMem() if this is first user call to myalloc()
    if(initialized == 0){
        SetPhysicalMem();
        initialized = 1;
    }

    return NULL;

}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void myfree(void *va, int size) {

    //Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    //Only free if the memory from "va" to va+size is valid
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {

    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/


}

/*
Bitmap functions below
*/

// Sets bit range [a, b) to 1
void SetBitRange(int A[], int a, int b){
    int i;
    for(i = a; i < b; i++)
        SetBit(A, i);
}

/*
set the bit at the k-th position in A[i]
*/
void SetBit(int A[], int k){
    A[k/32] |= 1 << (k%32); 
}

/*
clear the bit at the k-th position in A[i]
*/
void ClearBit(int A[], int k){
    A[k/32] &= ~(1 << (k%32));
}

/*
return value of bit at the k-th position in A[i]
*/
int TestBit(int A[],  int k){
    return ( (A[k/32] & (1 << (k%32) )) != 0);
}

/*
End bitmap functions
*/