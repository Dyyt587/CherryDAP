/*
 * @Author: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @Date: 2025-10-19 02:11:30
 * @LastEditors: Dyyt587 67887002+Dyyt587@users.noreply.github.com
 * @LastEditTime: 2025-10-19 02:13:16
 * @FilePath: \CherryDAP\projects\bl616\vfs\target\flash_blob.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef FLASH_BLOB_H
#define FLASH_BLOB_H
// #include "flash_algo.h"

#ifndef RAM_SIZE
#define RAM_SIZE 0x20000
#endif

typedef struct  {
    uint32_t breakpoint;
    uint32_t static_base;
    uint32_t stack_pointer;
} program_syscall_t;

typedef struct  {
     uint32_t  init;
     uint32_t  uninit;
     uint32_t  erase_chip;
     uint32_t  erase_sector;
     uint32_t  program_page;
     uint32_t  read;
     uint32_t  verify;
     program_syscall_t sys_call_s;
     uint32_t  program_buffer;
     uint32_t  algo_start;
     uint32_t  algo_size;
     uint32_t *algo_blob;
     uint32_t  program_buffer_size;
     uint32_t  algo_flags;         
} program_target_t;

typedef struct  {
    program_target_t target;
    uint32_t sector_info_length;
    uint32_t prog_blob[4096 * 5];
}flash_blob_t;

extern flash_blob_t tFlashBlob;

uint32_t target_flash_blob(char *chFileName,int wFlashAddr,int wRamBase);

#endif



