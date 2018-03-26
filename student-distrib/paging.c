#include "paging.h"

/* paging_init
 * 	   INPUT: None
 *	FUNCTION: Enables paging and fills the page directories and page tables
 */
void paging_init(void)
{
	uint32_t i;
	// initialize all page directories/tables to not present
 	for(i = 0; i < NUM_PAGE_DIRECTORY_ENTRIES; i++) {
 		page_directory[i].val = 0;
 		page_directory[i].read_write = 1; // all pages read/write enabled
 	}
	 	
	for(i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		page_table[i].val = 0;
		user_video_page_table[i].val = 0;
		page_table[i].read_write = 1;	// all pages read/write enabled
		user_video_page_table[i].read_write = 1;
		page_table[i].address = i;	// assign address to page table
	}
	// assign 1 page to display (80 width * 25 height * 2 byte < 4096)
	// assign 3 pages to inactive terminals (1 per terminal)
	page_table[VIDEO_MEM_LOCATION + 0].present = 1;
	page_table[VIDEO_MEM_LOCATION + 1].present = 1;
	page_table[VIDEO_MEM_LOCATION + 2].present = 1;
	page_table[VIDEO_MEM_LOCATION + 3].present = 1;
	// assign table for 1st entry of directory
	page_directory[0].address = ((int)page_table) >> 12;	// shift by 12 to remove non-address bits
	page_directory[0].present = 1; 
	// assign kernel page
	page_directory[1].address = KERNEL_ADDRESS >> 12; // shift by 12 to remove non-address bits
	page_directory[1].global = 1;
	page_directory[1].size = 1;
	page_directory[1].cache_disabled = 1;
	page_directory[1].present = 1;
	
	// enable paging
	asm (
		"movl	%%eax, %%cr3;"	// load cr3 with address of page directory
		"movl	%%cr4, %%eax;"	// enable 4 MB pages by turning PSE on in cr4 (bit 4, PSE)
		"orl	$0x00000010, %%eax;"
		"movl	%%eax, %%cr4;"
		"movl	%%cr0, %%eax;"	// enable paging by turning PG on in cr0 (bit 31) 
		"orl	$0x80000000, %%eax;"
		"movl	%%eax, %%cr0;"
		: // no outputs
		: "a"(page_directory)
		: "cc"
		);
}

/* user_mapping
 *    INPUT: none
 * FUNCTION: creates paging to allow user to access video memory
 */
void user_mapping(void)
{
	int parent_terminal = process_term();
	int offset = (parent_terminal + 1) * KiB4;
	// assign user video memory page
	user_video_page_table[0].address = (VIDEO_MEM_ADDRESS + offset) >> 12; // shift by 12 to remove non-address bits
	user_video_page_table[0].user_supervisor = 1;
	// user_video_page_table[0].read_write = 1;
	user_video_page_table[0].present = 1;
	page_directory[USER_VIDEO_LOCATION].address = ((int)user_video_page_table) >> 12; // shift by 12 to remove non-address bits
	page_directory[USER_VIDEO_LOCATION].user_supervisor = 1;
	page_directory[USER_VIDEO_LOCATION].read_write = 1;
	page_directory[USER_VIDEO_LOCATION].present = 1;
}

/* program_paging
 * 	   INPUT: physical_address : location of program to map to in physical memory
 *	FUNCTION: setup paging for user program to map from physical address to virtual address, also flushses TLB
 */
void program_paging(uint32_t physical_address)
{
	page_directory[FIRST_PROGRAM_LOCATION].address = physical_address >> 12; // shift by 12 to remove non-address bits
	page_directory[FIRST_PROGRAM_LOCATION].size = 1;
	page_directory[FIRST_PROGRAM_LOCATION].user_supervisor = 1;
	page_directory[FIRST_PROGRAM_LOCATION].read_write = 1;
	page_directory[FIRST_PROGRAM_LOCATION].present = 1;

	// flush TLB
	asm("movl %%cr3, %%eax;"
		"movl %%eax, %%cr3;"
		: // no outputs
		: // no inputs
		: "memory", "cc", "eax"
		);
}

/* swap_terminal_mapping
 * 	   INPUT: new_terminal : terminal number being swapped to
 *	FUNCTION: copies data from the inactive page table to the active page table
 */
void swap_terminal_mapping(int new_terminal)
{
	// check if terminal number is valid (1-3)
	if(new_terminal < 1 || new_terminal > 3)
		return;
	memcpy((uint32_t*)VIDEO_MEM_ADDRESS, (uint32_t*)(VIDEO_MEM_ADDRESS + (new_terminal * KiB4)), VID_MEM_BYTES);
}
