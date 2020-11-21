#include "my_vm.h"

int myerrno = 0;

int *physBitmap;
int *virtBitmap;

pde_t *pageDir = NULL; // pointer to page directory (array of page directory entries)

void *physMem = NULL; // void pointer to point to the start of allocated physical memory

size_t numVirtPages = 0; // stores total number of virtual pages
size_t numPhysPages = 0; // stores total number of physical pages

int numOffsetBits = 0; // stores the number of bits used to calculate the offset in virtual addresses (ex. bits 11:0 for PGSIZE = 4096)
int numOuterIndexBits = 0; // stores the number of bits used to calculate the outer index (index of page dir) in virtual addresses (ex. bits 21:12 for PGSIZE = 4096)
int numInnerIndexBits = 0; // stores the number of bits used to calculate the inner index (index of page table) in virtual addresses (ex. bits 31:22 for PGSIZE = 4096)
int numPageDirEntries = 0; // stores the number of page dir entries for easy traversal through array

int initialized = 0; // stores value representing whether or not physical memory has been initialized yet

unsigned int num_tlb_checks = 0; // stores the number of times the TLB is checked for an address mapping
unsigned int num_tlb_misses = 0; // stores the number of times the TLB checks encounter a miss

tlb *tlb_head = NULL;
tlb *tlb_tail = NULL;
int tlb_count = 0;
/*
int main()
{
    void *va, *pa;
    numOffsetBits = numOuterIndexBits = numInnerIndexBits = 4;

    va = (void*)0x440;
    pa = (void*)0x440;
    pageDir = malloc(sizeof(pde_t)*(int)pow(2, numOuterIndexBits));
    
    PageMap(pageDir, va, pa);
    if(DEBUG) printf("HELLO WORLD!\n");
}
*/
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
    if(DEBUG) printf("PHYSICAL MEMORY STARTS AT ADDRESS: %lx\n", (long unsigned int)physMem);
    // store appropriate number of virtual pages
    numVirtPages = MAX_MEMSIZE / PGSIZE;

    if(DEBUG) printf("numvirtpages: %d\n", numVirtPages);

    // store appropriate number of physical pages
    numPhysPages = MEMSIZE / PGSIZE;

    // set number of offset bits to log-base-2 of PGSIZE (ex. numOffsetBits = 12 for PGSIZE = 4096)
    numOffsetBits = log2(PGSIZE);

    // set number of outer index bits to half of bitspace remaining for use in the virtual address (ex. 10 for PGSIZE = 4096)
    numOuterIndexBits = (ADDRESS_BIT_LENGTH - numOffsetBits) / 2;

    // set number of outer index bits to the number of bits remaining for use in the virtual address (ex. 10 for PGSIZE = 4096)
    numInnerIndexBits = ADDRESS_BIT_LENGTH - (numOffsetBits + numOuterIndexBits);

    // store number of page dir entries based on number of outer index bits
    numPageDirEntries = pow(2, numOuterIndexBits);

    // determine number of elements needed in virtual bitmap (bit array)
    int bitmapLength = numVirtPages / sizeof(int);
    if(numVirtPages % ADDRESS_BIT_LENGTH != 0){
        bitmapLength++;
    }

    if(DEBUG) printf("virtual bitmap length: %d\n", bitmapLength);

    // allocate and initialize virtual bitmap
    //int temp[bitmapLength];
    virtBitmap = malloc(sizeof(int)*bitmapLength);
    int i;
    for(i = 0; i < bitmapLength; i++){
        virtBitmap[i] = 0;
    }
    SetBit(virtBitmap, 0);

    // determine number of elements needed in physical bitmap (bit array)
    bitmapLength = numPhysPages / ADDRESS_BIT_LENGTH;
    if(numPhysPages % ADDRESS_BIT_LENGTH != 0){
        bitmapLength++;
    }

    if(DEBUG) printf("physical bitmap length: %d\n", bitmapLength);

    // allocate and initialize physical bitmap
    //int temp2[bitmapLength];
    physBitmap = malloc(sizeof(int)*bitmapLength);
    for(i = 0; i < bitmapLength; i++){
        physBitmap[i] = 0;
    }
    physBitmap[0] = 1 << (sizeof(int)*8 - 1);

    // allocate and initialize page directory

    pde_t temp3[numPageDirEntries];
    pageDir = temp3;
    memset(pageDir, 0, numPageDirEntries*sizeof(pde_t));

    if(DEBUG) printf("number of page directory entries: %d\n", numPageDirEntries);

    //if(DEBUG) printf("outer bits: %d, inner bits: %d, offset bits: %d\n", numOuterIndexBits, numInnerIndexBits, numOffsetBits);

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
    //! Alec
    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    // Initialize new tlb entry with va and pa
    tlb *newEntry = malloc(sizeof(tlb));
    memset(newEntry, 0, sizeof(tlb));
    newEntry->entry = malloc(sizeof(tlb_entry));
    newEntry->entry->va = va;
    newEntry->entry->pa = pa;
    
    // in edge case where tlb size is 0
    if(TLB_SIZE == 0)
        return 0;

    // there are no entries
    if(tlb_count++ == 0)
    {
        tlb_head = tlb_tail = newEntry;
        return 0;
    }
    
    newEntry->next = tlb_head;
    tlb_head->prev = newEntry;
    tlb_head = newEntry;
    if(tlb_count > TLB_SIZE)
    {
        tlb_tail->prev->next = NULL;
        tlb_count--;
        tlb *temp = tlb_tail;
        tlb_tail = tlb_tail->prev;

        free(temp->entry);
        free(temp);
    }

    return -1;
}

int remove_TLB(void *va)
{
    //! Alec
    tlb *ptr = tlb_head;
    while(ptr != NULL)
    {
        if(ptr->entry->va == va)
        {
            if(ptr->prev != NULL)
                ptr->prev->next = ptr->next;
            if(ptr->next != NULL)
                ptr->next->prev = ptr->prev;
            free(ptr->entry);
            free(ptr);
            tlb_count--;
            return 0;
        }
        ptr = ptr->next;
    }
    return 1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {
    //! Alec
    /* Part 2: TLB lookup code here */
    num_tlb_checks++;

    tlb *ptr = tlb_head;
    while(ptr != NULL)
    {
        if(ptr->entry->va == va)
        {
            if(DEBUG) printf("***FOUND ADDRESS %lx from TLB\n", (unsigned long)ptr->entry->pa);
            return (pte_t *)(&ptr->entry->pa);
        }
        ptr = ptr->next;
    }
    num_tlb_misses++;
    return NULL;

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    //! Mark
    double miss_rate = 0;

    printf("misses: %d, checks: %d\n", num_tlb_misses, num_tlb_checks);

    /*Part 2 Code here to calculate and print the TLB miss rate*/

    miss_rate = (double)num_tlb_misses / (double)num_tlb_checks;


    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}


/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t * Translate(pde_t *pgdir, void *va) {

    //! Mark

    //HINT: Get the Page directory index (1st level) Then get the
    //2nd-level-page table index using the virtual address.  Using the page
    //directory index and page table index get the physical address

    //* first check TLB for virtual -> physical mapping. if not found, then continue on and find in page table
    if(DEBUG) puts("about to check tlb");
    pte_t *tlb_check = check_TLB((void*)((unsigned long)va & ~((1 << numOffsetBits) - 1)));
    if(DEBUG) puts("here");
    if(tlb_check != NULL){
        return tlb_check;
    }
    if(DEBUG) puts("checked tlb");

    unsigned long pDirMask = 0;
    int i;
    for(i = numOffsetBits + numInnerIndexBits; i < ADDRESS_BIT_LENGTH; i++){
        pDirMask |= (0x1 << i);
    }

    unsigned long pTableMask = 0;
    for(i = numOffsetBits; i < ADDRESS_BIT_LENGTH - numOuterIndexBits; i++){
        pTableMask |= (0x1 << i);
    }
    ////if(DEBUG) printf("pTableMask: %x\n", pTableMask);
    
    // grab outer index bits in address using pDirMask and store as page_directory_index
    unsigned long pdir_index = ((unsigned long)va & pDirMask) >> (numOffsetBits + numInnerIndexBits);

    // grab inner index bits in address using pTableMask and store as page_table_index
    unsigned long ptable_index = ((unsigned long)va & pTableMask) >> numOffsetBits;
    
    return &pgdir[pdir_index][ptable_index];
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added.
Returns:
    0 - Success
    -1 - Failure, set myerrno appropriately
*/
//? What's the format of pa? As in, should I be worried about handling offset bits?
int PageMap(pde_t *pgdir, void *va, void *pa)
{
    //! Alec
    /*HINT: Similar to Translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    // init all variables
    unsigned long virtAddress, pageTableMask, pageTableIndex, pageDirIndex;

    // cast the virtual address and remove the offset bits, since we are only accessing the page, not its contents
    virtAddress = ((unsigned long)va) >> numOffsetBits;

    // get the last numInnerIndexBits bits in the virtual address, since offset bits have been truncated
    pageTableMask = (unsigned long)pow(2, numInnerIndexBits) - 1; // 10 1s
    pageTableIndex = virtAddress & pageTableMask;

    // truncate the page table bits and set the remaining bits as pageDirIndex
    pageDirIndex = virtAddress >> numInnerIndexBits;

    if(pgdir[pageDirIndex] == NULL)
    {
        // allocates memory for a page table array of size 2^numInnerIndexBits
        unsigned long numEntriesInPageTable = (unsigned long)pow(2, numInnerIndexBits);
        unsigned long sizeOfPageTable = sizeof(pte_t) * numEntriesInPageTable;
        pgdir[pageDirIndex] = malloc(sizeOfPageTable);
        memset(pgdir[pageDirIndex], 0, sizeOfPageTable);
    }

    // Checks if the virtual address has already been mapped/not cleared
    if(pgdir[pageDirIndex][pageTableIndex] != 0)
    {
        myerrno = 1;
        return -1;
    }

    // Assign it
    pgdir[pageDirIndex][pageTableIndex] = (unsigned long)pa;

    // Change bitmap values
    SetBit(virtBitmap, (pageDirIndex << numInnerIndexBits) + pageTableIndex);
    SetBit(physBitmap, ((unsigned long)(pa - physMem) >> numOffsetBits));

    return 0;
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
Function that gets the next available virtual page(s) (if multiple pages, will return address of the first one)
*/
void *get_next_avail_virt(int num_pages_to_find) {

    //! Mark

    //Use virtual address bitmap to find the next free *virtual* pages

    int i;
    for(i = 0; i < numVirtPages; i++){
        if(TestBit(virtBitmap, i) == 0){ // found a 0
            if(DEBUG) printf("found a 0 at %d\n", i);
            // now make sure there are enough contiguous 0's
            int j;
            int foundEnoughZeroes = 1;
            for(j = 0; j < num_pages_to_find-1; j++){
                i++;
                if(i > numVirtPages || TestBit(virtBitmap, i) != 0){ // not enough contiguous 0's or reached end of virtual bitmap
                if(DEBUG) printf("did NOT find a 0 at %d\n", j);
                    foundEnoughZeroes = 0;
                    break;
                }
                if(DEBUG) printf("found a 0 at %d\n", j);
            }
            if(foundEnoughZeroes){
                if(DEBUG) puts("found enough 0's");
                // set bits to 1
                int k;
                for(k = 0; k <= j; k++){
                    SetBit(virtBitmap, i-k);
                }
                // *build appropriate virtual address and return it*
                unsigned long ret = ((i-j) / (int)pow(2, numInnerIndexBits)) << numInnerIndexBits;
                ret += ((i-j) % (int)pow(2, numInnerIndexBits)) << numOffsetBits;
                return (void*)ret;
            }

        }
    }
    return NULL;
}

/*
Function that gets the next available physical page (returns array of unsigned long when successful; returns NULL when unsuccessful)
*/
//! returns address based on physical bitmap - the return value must be added to the address
//! of the start of physical memory (void *physMem) in order to get real physical memory address
unsigned long *get_next_avail_phys(int num_pages_to_find) {

    //! Mark

    //Use virtual address bitmap to find the next free *physical* page

    unsigned long *physPages = malloc(num_pages_to_find * sizeof(unsigned long));

    int numPagesFound = 0;
    int i;
    for(i = 0; i < numPhysPages && numPagesFound < num_pages_to_find; i++){
        if(TestBit(physBitmap, i) == 0){
            physPages[numPagesFound] = ((unsigned long)i)*PGSIZE;
            numPagesFound++;
        }
    }
    if(numPagesFound != num_pages_to_find){ // not enough physical pages left -> return NULL
        return NULL;
    }
    // set the bits to 1
    int j;
    for(j = 0; j < num_pages_to_find; j++){
        SetBit(physBitmap, physPages[j]/PGSIZE);
    }
    return physPages;
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *myalloc(unsigned int num_bytes) {

    //! Mark

    //HINT: If the physical memory is not yet initialized, then allocate and initialize.

    /* HINT: If the page directory is not initialized, then initialize the
    page directory. Next, using get_next_avail(), check if there are free pages. If
    free pages are available, set the bitmaps and map a new page. Note, you will
    have to mark which physical pages are used. */


    // initialize physical memory using SetPhysicalMem() if this is first user call to myalloc()
    if(initialized == 0){
        if(DEBUG) puts("INITIALIZING");
        SetPhysicalMem();
        initialized = 1;
    }
    if(DEBUG) puts("CALLED MYALLOC");

    // calculate number of pages that need to be allocated
    unsigned long numPagesToAllocate = ceil((double)num_bytes / PGSIZE);
    if(DEBUG) printf("num pages to allocate: %lu \n", numPagesToAllocate);

    // find next available virtual pages, and check for failure. (bits in bitmap will be set by get_next_avail_virt function, so no need to worry about that)
    void *firstVirtPagePtr = get_next_avail_virt(numPagesToAllocate);
    if(firstVirtPagePtr == NULL){
        puts("myalloc failed - no virtual space left");
        return NULL;
    }

    // find next available physical pages, and check for failure. (bits in bitmap will be set by get_next_avail_phys function, so no need to worry about that)
    unsigned long *physPages = get_next_avail_phys(numPagesToAllocate);
    if(physPages == NULL){
        puts("myalloc failed - no physical space left");
        return NULL;
    }

    int h;
    for(h = 0; h < numPagesToAllocate; h++){
        if(DEBUG) printf("****************phys address: %lx\t", physPages[h]);
    }
    if(DEBUG) puts("");

    // now insert virtual -> physical mapping(s) into page table
    int i;
    for(i = 0; i < numPagesToAllocate; i++){

        void *virtAddress = firstVirtPagePtr;
        virtAddress += (i << numOffsetBits);
        void *physAddress = (void*)((unsigned long)physMem + physPages[i]);

        //! add new virtual -> physical mapping to TLB
        if(DEBUG) printf("****INSERTING INTO TLB -> phys address: %x, at virt address: %x\n", (unsigned int)physAddress, (unsigned int)virtAddress);
        add_TLB(virtAddress, physAddress);

        //! add new virtual -> physical mapping to page table
        PageMap(pageDir, virtAddress, physAddress);


        if(DEBUG) printf("****INSERTING INTO PAGE TABLE -> phys address: %x, at virt address: %x\n", (unsigned int)physAddress, (unsigned int)virtAddress);
        if(DEBUG) printf("\n-----------------\nPAGE DIR LOOKS LIKE:\n");
        int y;
        for(y = 0; y < 5; y++){
            if(DEBUG) printf("index: %d phys address: %x\n", y, (unsigned int)pageDir[0][y]);
        }

    }

    return firstVirtPagePtr;

}

/**
 * Responsible for releasing one or more memory pages using virtual address (va)
 * Procedure:
 *  Find the number of pages to free by max(1, ceil(size/pagesize))
 *  "Page index" is the first numOuterIndexBits + numInnerIndexBits bits of va
 *  Set virtbitmap and corresponding physbitmap values to 0 for all necessary pages
*/
void myfree(void *va, int size) {
    //!Alec

    // Free the page table entries starting from this virtual address (va)
    // Also mark the pages free in the bitmap
    // Only free if the memory from "va" to va+size is valid
    
    int numPages;
    unsigned long pageIndex, *pageDirIndexArray, *pageTableIndexArray;
    // Finds the number of pages we need to free
    //!numPages = max(1, ceil((double)size/numPages)); <- possible issue - size/numPages but numPages is not initialized... instead size/PGSIZE ?
    //numPages = max(1, ceil((double)size/numPages));
    numPages = max(1, ceil((double)size/PGSIZE));
    if(DEBUG) printf("NUM PAGES IN FREE FUNCTION: %d\n", numPages);
    pageDirIndexArray = malloc(numPages * sizeof(unsigned long));
    pageTableIndexArray = malloc(numPages * sizeof(unsigned long));

    // Truncate offset bits of the virtual address
    pageIndex = (unsigned long)va >> numOffsetBits;

    for(int i = 0; i < numPages; i++)
    {
        //init local variables
        unsigned long pageDirBits, pageTableBits, pageTableMask;

        // get the rightmost numInnerIndexBits Bits from the pageIndex
        pageTableMask = (unsigned long)pow(2, numInnerIndexBits) - 1;
        pageTableBits = (pageIndex + i) & pageTableMask;

        // gets the remaining bits
        pageDirBits = (pageIndex + i) >> numInnerIndexBits;

        if(pageDir[pageDirBits] == NULL || pageDir[pageDirBits][pageTableBits] == 0)
        {
            if(DEBUG) printf("SEGMENTATION FAULT: cannot free memory which has not been allocated first\n");
            free(pageDirIndexArray);
            free(pageTableIndexArray);
            return;
        }

        // We need to validate that all the memory locations are valid before we free,
        // so we store the indices in another array for later.
        // Later we actually perform the freeing actions
        pageDirIndexArray  [i] = pageDirBits;
        pageTableIndexArray[i] = pageTableBits;
    }

    for(int i = 0; i < numPages; i++)
    {
        unsigned long virtBitmapIndex, physBitmapIndex, physAddr;

        // restores the virtual address index
        virtBitmapIndex = (pageDirIndexArray[i] << numInnerIndexBits) + pageTableIndexArray[i];

        // retrieves the physical address from the page directory
        physAddr = pageDir[pageDirIndexArray[i]][pageTableIndexArray[i]];

        // truncates offset bits to get the bitmap index;
        physBitmapIndex = physAddr >> numOffsetBits - (unsigned long)physMem;

        // Frees the memory
        ClearBit(virtBitmap, virtBitmapIndex);
        ClearBit(physBitmap, physBitmapIndex);
        pageDir[pageDirIndexArray[i]][pageTableIndexArray[i]] = 0;
    }
    free(pageDirIndexArray);
    free(pageTableIndexArray);
}


/**
 * The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
*/
void PutVal(void *va, void *val, int size) {
    //! Alec
    /* HINT: Using the virtual address and Translate(), find the physical page. Copy
       the contents of "val" to a physical page. NOTE: The "size" value can be larger
       than one page. Therefore, you may have to find multiple pages using Translate()
       function.*/

    int remainingBits = size;
    char *data = (char*)val;
    if(DEBUG) printf("DOING PUTVAL() ON %d\n", *data);


    for(int i = 0; remainingBits > 0; i++)
    {
        // Get a pointer to the physical memory. We dereference the pointer given to get 
        // the unsigned long, then cast that to a char* so we can reference the physical memory
        char *physAddr = (char*)(*Translate(pageDir, va + (i << numOffsetBits)));

        // Determine how many bits we are copying
        int bitsToCopy = min(remainingBits, PGSIZE);
        remainingBits -= bitsToCopy;

        //! add offset bits to phys address
        unsigned long offsetMask = pow(2, numOffsetBits) - 1;
        unsigned long numOffsetBytes = (unsigned long)va & offsetMask;
        char* physAddrWithOffset = (char*)((unsigned long)physAddr + numOffsetBytes);

        if(DEBUG) printf("DOING PUTVAL AT PHYS ADDRESS %x\n", (unsigned int)physAddrWithOffset);

        // copy the bits over to memory
        strncpy(physAddrWithOffset, data, bitsToCopy);

        // increment data so we copy new data next time
        data += bitsToCopy;
    }
    
}


/*Given a virtual address, this function copies the contents of the page to val*/
void GetVal(void *va, void *val, int size) {

    //! Alec
    /* HINT: put the values pointed to by "va" inside the physical memory at given
    "val" address. Assume you can access "val" directly by derefencing them.
    If you are implementing TLB,  always check first the presence of translation
    in TLB before proceeding forward */
    char *data = (char*)val;
    int remainingBits = size;
    for(int i = 0; remainingBits > 0; i++)
    {
        // Get a pointer to the physical memory. We dereference the pointer given to get 
        // the unsigned long, then cast that to a char* so we can reference the physical memory
        char *physAddr = (char*)(*Translate(pageDir, va + (i << numOffsetBits)));

        // Determine how many bits we are copying
        int bitsToCopy = min(remainingBits, PGSIZE);
        remainingBits -= bitsToCopy;

        //! add offset bits to phys address
        unsigned long offsetMask = pow(2, numOffsetBits) - 1;
        unsigned long numOffsetBytes = (unsigned long)va & offsetMask;
        char* physAddrWithOffset = (char*)((unsigned long)physAddr + numOffsetBytes);

        // copy the bits over from memory
        strncpy(data, physAddrWithOffset, bitsToCopy);

        // increment data so we copy new data next time
        data += bitsToCopy;
    }

}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void MatMult(void *mat1, void *mat2, int size, void *answer) {
    //! Alec
    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
    matrix accessed. Similar to the code in test.c, you will use GetVal() to
    load each element and perform multiplication. Take a look at test.c! In addition to
    getting the values from two matrices, you will perform multiplication and
    store the result to the "answer array"*/

    int i, j, k;
    for(i = 0; i < size; i++)
    {
        for(j = 0; j < size; j++)
        {
            int sum = 0;
            for(k = 0; k < size; k++)
            {
                int mat1Value, mat2Value;
                GetVal(mat1 + sizeof(int) * (i * size + k), &mat1Value, sizeof(int));
                GetVal(mat2 + sizeof(int) * (k * size + j), &mat2Value, sizeof(int));
                sum += mat1Value * mat2Value;
            }
            PutVal(answer + sizeof(int)*(i*size + j), &sum, sizeof(int));
        }
    }
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
set the bit at the k-th position in A[]
*/
void SetBit(int A[], int k){
    A[k/32] |= 1 << (k%32); 
}

/*
clear the bit at the k-th position in A[]
*/
void ClearBit(int A[], int k){
    A[k/32] &= ~(1 << (k%32));
}

/*
return value of bit at the k-th position in A[]
*/
int TestBit(int A[],  int k){
    return ( (A[k/32] & (1 << (k%32) )) != 0);
}

/*
End bitmap functions
*/