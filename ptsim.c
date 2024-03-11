#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    // TODO
    int page_table_created = 0;
    //int page_table_address = 0;
    int app_page_table;
    //int page_address;

    //Allocate app page table
    for(int page_index = 1; page_index < PAGE_COUNT; page_index++){
        //page_address = get_address(0, page_index);
        if(mem[page_index] == 0){//if the page is free
            mem[page_index] = 1;//mark as used
            page_table_created = 1;//prove that a page was allocated
            //create page table link
            mem[get_address(0, proc_num + PTP_OFFSET)] = page_index;
            break;
        }
    }
    //OUT OF MEMORY ERROR
    if(page_table_created == 0){
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }
    app_page_table = get_page_table(proc_num);

    int page_sucess = 0;
    int current_count = 0;
    //Allocate other pages untill all have been created
    while(current_count < page_count){
        page_sucess = 0;
        for(int page_index = 0; page_index < PAGE_COUNT; page_index++){
            if(mem[page_index] == 0){
                mem[page_index] = 1;//mark page used
                mem[get_address(app_page_table, current_count)] = page_index; //link pages
                current_count++;
                page_sucess = 1;
                break;
            }
        }
        //OUT OF MEMORY ERROR
        if(page_sucess == 0){
            printf("OOM: proc %d: data page\n", proc_num);
            return;
        }
    }
}


//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < PAGE_COUNT; i++) {//MODIFICATION: 64 magic number to PAGE_COUNT
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }

    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            //printf("pfm");
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            //printf("ppt n:%d", proc_num);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            int proc_num = atoi(argv[++i]);
            int page_count = atoi(argv[++i]);
            //printf("np n:%d m:%d", proc_num, page_count);
            new_process(proc_num, page_count);
        }
    }
    fflush(stdout);
}
