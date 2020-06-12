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
	return true;
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
	return (int)SFUD_FS_CMD_OK;	
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
	return (int)SFUD_FS_CMD_OK;	
}

int ei_sfud_fs_prepare_sampling(void)
{
	return (int)SFUD_FS_CMD_OK;	
}

int ei_sfud_fs_erase_sampledata(uint32_t start_block, uint32_t end_address)
{
	return (int)SFUD_FS_CMD_OK;	
}

int ei_sfud_fs_write_sample_block(const void *sample_buffer, uint32_t address_offset)
{
	return (int)SFUD_FS_CMD_OK;	
}

int ei_sfud_fs_write_samples(const void *sample_buffer, uint32_t address_offset, uint32_t n_samples)
{
	return (int)SFUD_FS_CMD_OK;	
}

int ei_sfud_fs_read_sample_data(void *sample_buffer, uint32_t address_offset, uint32_t n_read_bytes)
{
	return 512;
}

uint32_t ei_sfud_fs_get_n_available_sample_blocks(void)
{
	return 256;	
}

