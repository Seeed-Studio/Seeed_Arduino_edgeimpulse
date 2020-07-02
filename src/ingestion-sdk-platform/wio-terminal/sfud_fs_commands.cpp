/* Edge Impulse ingestion SDK
 * Copyright (c) 2020 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include ----------------------------------------------------------------- */
#include "sfud_fs_commands.h"

#include "sfud.h"
#include "Arduino.h"
//#include "setup.h"

/* Private types & constants ---------------------------------------------- */

/**
 * File system config struct.
 * @details Holds all the info needed for config file and sample data.<br>
 * - The config file is stored in the last available sector<br>
 * - The sample data starts at the end of the program data and ends before the
 * config file
 */
typedef struct
{
	uint32_t sector_size;					/*!< Erase sector size 			 */
	uint32_t page_size;						/*!< Minimum page write size 	 */
	uint32_t config_file_address;			/*!< Start address of config file*/
	uint32_t sample_start_address;			/*!< Start of sample storage mem */
	bool     fs_init;						/*!< FS is successfully init  	 */
    const sfud_flash *flash;
	
}ei_sfud_fs_t;

/** 32-bit align write buffer size */
#define WORD_ALIGN(a)	((a & 0x3) ? (a & ~0x3) + 0x4 : a)
/** Align addres to given sector size */
#define SECTOR_ALIGN(a, sec_size)	((a & (sec_size-1)) ? (a & ~(sec_size-1)) + sec_size : a)

/* Private variables ------------------------------------------------------- */
static ei_sfud_fs_t sfud_fs = {0};

/* Public functions -------------------------------------------------------- */

/**
 * @brief      Init Flash pheripheral for reading & writing and set all 
 * 			   parameters for the file system.
 * @return     true if succesful else false
 */
bool ei_sfud_fs_init(void)
{
    char ret;
    ret = sfud_init();
    
    #ifdef SFUD_USING_QSPI
    sfud_qspi_fast_read_enable(sfud_get_device(SFUD_W25Q32_DEVICE_INDEX), 2);
    #endif 

    sfud_fs.flash  = sfud_get_device_table() + 0;
    sfud_fs.fs_init = true;
	sfud_fs.sector_size = ei_sfud_fs_get_block_size();
    sfud_fs.config_file_address = 0;
    sfud_fs.sample_start_address  =  1 * ei_sfud_fs_get_block_size();
	return ret == SFUD_FS_CMD_OK;
}

/**
 * @brief      Copy configuration data to config pointer
 *
 * @param      config       Destination pointer for config
 * @param[in]  config_size  Size of configuration in bytes
 *
 * @return     ei_sfud_ret_t enum
 */
int ei_sfud_fs_load_config(uint32_t *config, uint32_t config_size)
{

	char ret;

	if(config == NULL) {
		ret = SFUD_FS_CMD_NULL_POINTER;
	}

	else if(sfud_fs.fs_init == true) {
        ret = (sfud_read(sfud_fs.flash, sfud_fs.config_file_address, config_size, (uint8_t *)config) == SFUD_SUCCESS)		
            ? SFUD_FS_CMD_OK
			: SFUD_FS_CMD_READ_ERROR;
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}
	return (int)ret;	
}

/**
 * @brief      Write config to Flash
 *
 * @param[in]  config       Pointer to configuration data
 * @param[in]  config_size  Size of configuration in bytes
 *
 * @return     ei_sfud_ret_t enum
 */
int ei_sfud_fs_save_config(const uint32_t *config, uint32_t config_size)
{
	char ret;
	
	if(config == NULL) {
		ret = SFUD_FS_CMD_NULL_POINTER;
	}
    if(config_size > sfud_fs.sample_start_address){
        ret = SFUD_FS_CMD_WRITE_ERROR;
    }
	else if(sfud_fs.fs_init == true) {
        ret = (sfud_erase(sfud_fs.flash, sfud_fs.config_file_address, config_size )== SFUD_SUCCESS)
			? SFUD_FS_CMD_OK
			: SFUD_FS_CMD_ERASE_ERROR;

		if(ret == SFUD_FS_CMD_OK) {
            ret  = (sfud_write(sfud_fs.flash,sfud_fs.config_file_address,config_size ,(const uint8_t* )config) == SFUD_SUCCESS)
				? SFUD_FS_CMD_OK
				: SFUD_FS_CMD_WRITE_ERROR;
		}
		else {
			ret = SFUD_FS_CMD_ERASE_ERROR;
		}
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}

	return (int)ret;
}

int ei_sfud_fs_prepare_sampling(void)
{
	char ret;
	if(sfud_fs.fs_init == true) {
        ret = (sfud_erase(sfud_fs.flash, sfud_fs.sample_start_address, sfud_fs.flash->chip.capacity - sfud_fs.sample_start_address)== SFUD_SUCCESS)
			? SFUD_FS_CMD_OK
			: SFUD_FS_CMD_ERASE_ERROR;
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}

	return ret;
}

int ei_sfud_fs_erase_sampledata(uint32_t start_block, uint32_t length)
{
	char ret;
	if(sfud_fs.fs_init == true) {
		uint32_t start_addr = sfud_fs.sample_start_address + start_block * ei_sfud_fs_get_block_size();
        ret = (sfud_erase(sfud_fs.flash, start_addr, length)== SFUD_SUCCESS)
			? SFUD_FS_CMD_OK
			: SFUD_FS_CMD_ERASE_ERROR;
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}

	return ret;	
}
#if 0
int ei_sfud_fs_write_sample_block(const void *sample_buffer, uint32_t address_offset)
{
	char ret;

	if(sample_buffer == NULL) {
		ret = SFUD_FS_CMD_NULL_POINTER;
	}
	else if (sfud_fs.fs_init == true) {
        // ret  = (sfud_write(sfud_fs.flash,sfud_fs.sample_start_address, address_offset ,(const uint8_t* )sample_buffer) == SFUD_SUCCESS)
		// 		? SFUD_FS_CMD_OK
		// 		: SFUD_FS_CMD_WRITE_ERROR;
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}

	return ret;
}
#endif 

int ei_sfud_fs_write_samples(const void *sample_buffer, uint32_t address_offset, uint32_t n_samples)
{
	int ret;

	if(sample_buffer == NULL) {
		ret = SFUD_FS_CMD_NULL_POINTER;
	}
	else if (sfud_fs.fs_init == true) {
        ret  = (sfud_write(sfud_fs.flash,sfud_fs.sample_start_address + address_offset, n_samples ,(const uint8_t* )sample_buffer) == SFUD_SUCCESS)
				? SFUD_FS_CMD_OK
				: SFUD_FS_CMD_WRITE_ERROR;
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}
    return ret;
}

int ei_sfud_fs_read_sample_data(void *sample_buffer, uint32_t address_offset, uint32_t n_read_bytes)
{
	char ret;
	if(sample_buffer == NULL) {
		ret = SFUD_FS_CMD_NULL_POINTER;
	}
	else if(sfud_fs.fs_init == true) {
        ret = (sfud_read(sfud_fs.flash, sfud_fs.sample_start_address + address_offset, n_read_bytes, (uint8_t *)sample_buffer) == SFUD_SUCCESS)	
			? SFUD_FS_CMD_OK
			: SFUD_FS_CMD_READ_ERROR;
	}
	else {
		ret = SFUD_FS_CMD_NOT_INIT;
	}	
	return ret;
}

uint32_t ei_sfud_fs_get_n_available_sample_blocks(void)
{
	return sfud_fs.flash->chip.capacity ;
}

uint32_t ei_sfud_fs_get_block_size(void){
    return 4*1024;
}