/*
Patrick Punch
3/15/2025
CS 374
*/
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
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    int page_table_page = -1; // -1 for unavailable memory flag
    // Search for a free page in memory
    for (int i = 1; i < PAGE_COUNT; i++){ // Page 0 is reserved for system stuff
        if (mem[i] == 0){
            page_table_page = i;
            mem[i] = 1;
            break;
        }
    }
    if (page_table_page == -1){
        fprintf(stderr, "No memory available\n");
        return;
    }
    // Store the page table page number in the zero page
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num); // Get the ptp_addr
    mem[ptp_addr] = page_table_page;

    // Allocate pages for the data of the new process
    for (int i = 0; i < page_count; i++){
        int data_page = -1;
        // Search for a free page for the data
        for (int j = 1; j < PAGE_COUNT; j++){
            if (mem[j] == 0){
                data_page = j;
                mem[j] = 1;
                break;
            }
        }
        if (data_page == -1){
            fprintf(stderr, "No pages available\n");
            return;
        }
        int page_table_addr = get_address(page_table_page, i); // Compute addr in page table
        mem[page_table_addr] = data_page; // Store the page number in the page table
    }
}

//
// Add 'p' pages to the process 'n'
//
void add_pages(int proc_num, int p)
{
    int page_table_page = get_page_table(proc_num);
    int first_free_entry = -1;
    for (int i = 0; i < PAGE_COUNT; i++){
        int addr = get_address(page_table_page, i);
        if (mem[addr] == 0){
            first_free_entry = i;
            break;
        }
    }

    if (first_free_entry == -1){
        fprintf(stderr, "Page table full for process %d\n", proc_num);
        return;
    }
    int num_allocated = 0;
    for (int i = 0; i < p; i++){ // Allocate 'p' pages for the process
        int data_page = -1;
        for (int j = 1; j < PAGE_COUNT; j++){
            if (mem[j] == 0){ // Find a free page
                data_page = j;
                mem[j] = 1;
                break;
            }
        }

        if (data_page == -1){
            fprintf(stderr, "No physical memory available\n");
            return;
        }
        int page_table_addr = get_address(page_table_page, first_free_entry + i);
        mem[page_table_addr] = data_page; // Add the new physical page to the page table entry
        num_allocated++;
    }
}

//
// Kill a process by freeing its pages and its page table
//
void kill_process(int proc_num)
{
    int page_table_page = get_page_table(proc_num);

    for (int i = 0; i < PAGE_COUNT; i++){
        int addr = get_address(page_table_page, i);
        int page = mem[addr];
        if (page != 0){
            mem[page] = 0; // Free the page
        }
    }
    mem[page_table_page] = 0; // Free the page table itself

    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    mem[ptp_addr] = 0; // Free the page table entry in zero page
}

int get_physical_page(int proc_num, int vaddr)
{
    int virtual_page = vaddr >> PAGE_SHIFT;
    int page_table_page = get_page_table(proc_num);
    int addr = get_address(page_table_page, virtual_page); // Get the physical address from the page table
    int physical_page = mem[addr];
    if (physical_page == 0){ // Check for page fault
        printf("PAGE FAULT: proc %d, vaddr %d\n", proc_num, vaddr);
    }
    return physical_page;
}
//
// Store a value at a virtual address
//
void store_value(int proc_num, int vaddr, int val)
{
    int offset = vaddr & 255;
    int physical_page = get_physical_page(proc_num, vaddr);

    if (physical_page != 0){
        int phys_addr = get_address(physical_page, offset);
        mem[phys_addr] = val; // Calculate the physical address
        printf("Store proc %d: %d => %d, value=%d\n", proc_num, vaddr, phys_addr, val);
    }
}

//
// Load a value from a virtual address
//
void load_value(int proc_num, int vaddr)
{
    int offset = vaddr & 255;
    int physical_page = get_physical_page(proc_num, vaddr);

    if (physical_page != 0){
        // Calculate the physical address
        int phys_addr = get_address(physical_page, offset);
        int val = mem[phys_addr];
        printf("Load proc %d: %d => %d, value=%d\n", proc_num, vaddr, phys_addr, val);
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

    for (int i = 0; i < 64; i++) {
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
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0){
            int proc_num = atoi(argv[++i]);
            int num_data_pages = atoi(argv[++i]);
            new_process(proc_num, num_data_pages);
        }
        else if (strcmp(argv[i], "kp") == 0){
            int proc_num = atoi(argv[++i]);
            kill_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0){
            int proc_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            int val = atoi(argv[++i]);
            store_value(proc_num, vaddr, val);
        }
        else if (strcmp(argv[i], "lb") == 0){
            int proc_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            load_value(proc_num, vaddr);
        }
        else if (strcmp(argv[i], "ap") == 0){
            int proc_num = atoi(argv[++i]);
            int num_pages = atoi(argv[++i]);
            add_pages(proc_num, num_pages);
        }
    }
}
