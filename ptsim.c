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

//virtual to physical address
int virt_to_phys(int proc_num, int virtual_address){
    int proc_table = get_page_table(proc_num);

    //address virtual page half to physical page
    int virt_page_num = virtual_address >> PAGE_SHIFT;
    int phys_page = mem[get_address(proc_table, virt_page_num)];

    int phys_offset = virtual_address & 255;//this should probably be PAGE_SIZE, but its 256? PAGE_SIZE - 1?

    int phys_addr = (phys_page << PAGE_SHIFT) | phys_offset;
    return phys_addr;
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
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

void kill_process(int proc_num){
    //no deleting the OS page
    if(proc_num == 0){
        return;
    }

    int process_table = get_page_table(proc_num);

    //remove page table page
    int table_page = mem[get_address(0, proc_num + PTP_OFFSET)];
    mem[table_page] = 0;
    //actual table in memory is retained cause i don't feel like re accessing this wild memory address, keep track of this

    //remove pages
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(process_table, i);
        int page = mem[addr];
        if (page != 0) {
            //printf("%02x -> %02x\n", i, page);
            mem[page] = 0;
        }
    }
}

void store_value(int proc_num, int vaddr, int val){
    int addr = virt_to_phys(proc_num, vaddr);

    mem[addr] = val;

    printf("Store proc %d: %d => %d, value=%d\n", proc_num, vaddr, addr, val);
}

int get_value(int proc_num, int vaddr){
    int addr = virt_to_phys(proc_num, vaddr);

    int val = mem[addr];

    printf("Load proc %d: %d => %d, value=%d\n",
    proc_num, vaddr, addr, val);

    return val;
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
        else if (strcmp(argv[i], "kp") == 0) {
            int proc_num = atoi(argv[++i]);
            //printf("kp n:%d", proc_num);
            kill_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0) {
            int proc_num = atoi(argv[++i]);
            int addr_num = atoi(argv[++i]);
            int value_num = atoi(argv[++i]);
            store_value(proc_num, addr_num, value_num);
            //printf("sb n:%d a:%d b:%d", proc_num, addr_num, value_num);
        }
        else if (strcmp(argv[i], "lb") == 0) {
            int proc_num = atoi(argv[++i]);
            int addr_num = atoi(argv[++i]);
            //printf("lb n:%d a:%d", proc_num, addr_num);
            get_value(proc_num, addr_num);
        }
    }
    fflush(stdout);
}
