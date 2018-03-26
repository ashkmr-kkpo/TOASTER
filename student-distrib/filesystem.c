#include "filesystem.h"

/* initialize_file_system
 *    INPUT: file_system_start_address - The address of the boot block which is at the start of the file system.
 * FUNCTION: Save the address of the boot block locally.
 */
void initialize_file_system(void * file_system_start_address)
{
	boot_block = (file_system_statistics_t *)file_system_start_address;
}

/* read_dentry_by_name
 *    INPUT: filename - The name of the rtc, file, or directory to read.
 			 dentry - A pointer to memory to store the directory that we read.
 * FUNCTION: Search the file system for a directory entry with the name 'filename' and store it back in the 'dentry'.
 */
int32_t read_dentry_by_name(const uint8_t* filename, dentry_t* dentry)
{
	if(dentry == NULL)
		return -1;
	uint32_t dentry_index = 0;
    int len_input = strlen((int8_t *)filename);
    int len_file, length;
    dentry_t * searched_dentry = (dentry_t *)(boot_block);
    searched_dentry += FIRST_FILE_OR_FOLDER_OFFSET;

    for(dentry_index = 0; dentry_index < boot_block->num_dentries; dentry_index++) {
    	len_file = strlen((int8_t*)searched_dentry->file_name);
    	length = (len_file > len_input) ? len_file : len_input;
    	length = (length > MAX_FILE_LENGTH) ? MAX_FILE_LENGTH : length;
    	if(strncmp((int8_t*)searched_dentry->file_name, (int8_t *)filename, length) == 0) {
    		memcpy(dentry, searched_dentry, sizeof(dentry_t));
    		return 0;
    	}
    	searched_dentry += 1;
    }
	return -1;
}

/* read_dentry_by_index
 *    INPUT: index - The index into the directory entries in the boot block.
 			 dentry - A pointer to the memory to store the directory that we read.
 * FUNCTION: Fetch the directory entry that is at index 'index' and store it in 'dentry'.
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
	if(index < 0 || index >= boot_block->num_inodes || dentry == NULL)
	{
		return -1;
	}
	dentry_t * searched_dentry = (dentry_t *)(boot_block);
	searched_dentry += FIRST_FILE_OR_FOLDER_OFFSET;
	uint32_t inode_index = 0;
	for(inode_index = 0; inode_index < boot_block->num_inodes; inode_index++)
	{
		if(searched_dentry->inode_number == index)
		{
			memcpy(dentry, searched_dentry, sizeof(dentry_t));
			return 0;
		}
		searched_dentry++;
	}
	return -1;
}

/* read_data
 *    INPUT: inode - The inode of the directory that we wish to read.
 			 offset - This is the offset in bytes into the file that we wish to read.
 			 buf - The buffer to store the read data into.
 			 length - The length in bytes of the file that we wish to read.
 * FUNCTION: Read 'length' raw bytes starting at 'offset' into the 'inode' and store it into the buffer 'buf'.
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	if(inode < 0 || inode >= boot_block->num_inodes || buf == NULL || length <= 0)
		return -1;

	//GO TO INODE
	uint8_t * find_inode = (uint8_t *)(boot_block);
	find_inode += BLOCK_SIZE; //jump past the boot block
	find_inode += inode * BLOCK_SIZE; //select the correct inode
	inode_t * selected_inode = (inode_t *)find_inode;
	//GO TO INODE

	if(selected_inode->length_in_bytes <= 0)
	{
		return 0;
	}

	//CALCULATE INODE DATA BLOCK OFFSET
	uint32_t inode_dblock_offset = offset / BLOCK_SIZE;
	uint32_t dblock_offset = offset % BLOCK_SIZE;
	uint32_t curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
	//CALCULATE INODE DATA BLOCK OFFSET

	if(curr_dblock < 0)
		return -1;

	//FIND DATA BLOCK ZERO
	dblock_t * dblock_zero = (dblock_t *)boot_block;
	dblock_zero += INODE_OFFSET_IN_BLOCKS;
	dblock_zero += boot_block->num_inodes;
	//FIND DATA BLOCK ZERO

	//GET POINTER TO FIRST DATA BLOCK WITH OFFSET
	uint8_t * curr_dblock_ptr = (uint8_t *)dblock_zero;
	curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
	curr_dblock_ptr += dblock_offset;
	//GET POINTER TO FIRST DATA BLOCK WITH OFFSET

	//LOOP THROUGH THE INODE DATA BLOCKS COPYING MEMORY INTO BUF
	uint32_t data_byte_index = 0;
	uint32_t bytes_copied = 0;
	uint32_t remaining_length = 0;
	if(length > selected_inode->length_in_bytes - offset)
		remaining_length = selected_inode->length_in_bytes - offset;
	else
		remaining_length = length;

	if(offset > selected_inode->length_in_bytes)
		return 0; //EOF CONDITION OR ERROR CONDITION?

	for(data_byte_index = 0; data_byte_index < remaining_length; data_byte_index++) {
		if((offset+data_byte_index) % BLOCK_SIZE == 0 && data_byte_index !=0) {
			inode_dblock_offset++;
			if(inode_dblock_offset >= NUM_DBLOCK_REFS)
				return -1;
			curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
			curr_dblock_ptr = (uint8_t *)dblock_zero;
			curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
		}
		memcpy(buf, curr_dblock_ptr, 1);
		curr_dblock_ptr++;
		buf++;
		bytes_copied++;
	}
	return bytes_copied;
	//LOOP THROUGH THE INODE DATA BLOCKS COPYING MEMORY INTO BUF
}

/* read_data_corr_sig
 *    INPUT: fd - The file descriptor of the file we want to read.			 
 			 buf - The buffer to store the read data into.
 			 length - The length in bytes of the file that we wish to read.
 * FUNCTION: Read 'length' raw bytes from 'fd' and store it into the buffer 'buf'.
 */
int32_t read_data_corr_sig(uint32_t fd, uint8_t* buf, uint32_t length)
{
	PCB* pcb = get_pcb_ptr();
	if(pcb == NULL)
		return -1;
	uint32_t inode = pcb->fd[fd].inode_num;
	uint32_t offset = pcb->fd[fd].file_pos;
	if(inode < 0 || inode >= boot_block->num_inodes || buf == NULL || length <= 0)
		return -1;

	//GO TO INODE
	uint8_t * find_inode = (uint8_t *)(boot_block);
	find_inode += BLOCK_SIZE; //jump past the boot block
	find_inode += inode * BLOCK_SIZE; //select the correct inode
	inode_t * selected_inode = (inode_t *)find_inode;
	//

	if(selected_inode->length_in_bytes <= 0)
		return 0;

	//CALCULATE INODE DATA BLOCK OFFSET
	uint32_t inode_dblock_offset = offset / BLOCK_SIZE;
	uint32_t dblock_offset = offset % BLOCK_SIZE;
	uint32_t curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
	//

	if(curr_dblock < 0)
		return -1;

	//FIND DATA BLOCK ZERO
	dblock_t * dblock_zero = (dblock_t *)boot_block;
	dblock_zero += INODE_OFFSET_IN_BLOCKS;
	dblock_zero += boot_block->num_inodes;
	//

	//GET POINTER TO FIRST DATA BLOCK WITH OFFSET
	uint8_t * curr_dblock_ptr = (uint8_t *)dblock_zero;
	curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
	curr_dblock_ptr += dblock_offset;
	//

	//LOOP THROUGH THE INODE DATA BLOCKS COPYING MEMORY INTO BUF
	uint32_t data_byte_index = 0;
	uint32_t bytes_copied = 0;
	uint32_t remaining_length = 0;
	if(length >= selected_inode->length_in_bytes - offset)
		remaining_length = selected_inode->length_in_bytes - offset;
	else
		remaining_length = length;

	if(offset >= selected_inode->length_in_bytes)
		return 0; //EOF CONDITION OR ERROR CONDITION?
	for(data_byte_index = 0; data_byte_index < remaining_length; data_byte_index++) {
		if((offset+data_byte_index) % BLOCK_SIZE == 0 && data_byte_index !=0) {
			inode_dblock_offset++;
			if(inode_dblock_offset >= NUM_DBLOCK_REFS)
				return -1;
			curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
			curr_dblock_ptr = (uint8_t *)dblock_zero;
			curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
		}
		if(curr_dblock_ptr==NULL || buf ==NULL)
		{
			return bytes_copied;
		}
		memcpy(buf, curr_dblock_ptr, 1);
		curr_dblock_ptr++;
		buf++;
		bytes_copied++;
		pcb->fd[fd].file_pos++;
	}
	return bytes_copied;
	//
}

/* file_write
 *    INPUT: fd - The file descriptor of the file we want to write.			 
 			 buf - The buffer to write the data to.
 			 length - The length in bytes of the file that we wish to write.
 * FUNCTION: does nothing, files are read-only
 */
uint32_t file_write (int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/* directory_write
 *    INPUT: fd - The file descriptor of the file we want to write.			 
 			 buf - The buffer to write the data to.
 			 length - The length in bytes of the file that we wish to write.
 * FUNCTION: does nothing, directories are read-only
 */
uint32_t directory_write (int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/* file_close
 *    INPUT:
 * FUNCTION:
 */
uint32_t file_close (int32_t fd)
{
	return 0;
}

/* directory_close
 *    INPUT:
 * FUNCTION:
 */
uint32_t directory_close (int32_t fd)
{
	return 0;
}

/* read_directory
 *    INPUT:
 * FUNCTION:
 */
uint32_t read_directory (int32_t fd, void* buf, int32_t nbytes)
{
	// get pcb to access fd
	PCB* pcb = get_pcb_ptr();
	uint32_t offset = pcb->fd[fd].file_pos;
	// check if offset reading blocks without file name
	if(boot_block->dentries[offset].file_name[0] == '\0')
		return 0;
	uint32_t length = 0;
	// as much as fits or all 32 bytes
	length = (nbytes > MAX_FILE_LENGTH) ? MAX_FILE_LENGTH : nbytes;
	// copy to buffer
	strncpy((int8_t*) buf, (int8_t*)boot_block->dentries[offset].file_name, length);
	// update position for subsequent reads
	pcb->fd[fd].file_pos++;
	return length;
}

/* open_file
 *    INPUT:
 * FUNCTION:
 */
uint32_t open_file(const uint8_t * filename, dentry_t * dentry)
{
	return read_dentry_by_name(filename, dentry);
}

/* open_directory
 *    INPUT:
 * FUNCTION:
 */
uint32_t open_directory(const uint8_t * filename, dentry_t * dentry)
{
	return read_dentry_by_name(filename, dentry);
}
