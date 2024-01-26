#ifndef MEMORY_MALLOC_H
#define MEMORY_MALLOC_H

#include "common/types/basic.h"
#include "common/lib/list.h"

/**
 * @brief alloc size virtual memory from heap,
 * it will align 8bit in 32bit cpu or 16bit in 64bit cpu
 * @param [in] size the size of we want memory, unit is byte
 * @return the virtual memory start address
 */
void *malloc(size_t size);

/**
 * @brief release the virtual memory start with ptr
 * memory size is in the start of the memory block
 * @param [in] ptr the memory start we want to release
 */
void free(void *ptr);

/**
 * @brief alloc size virtual memory from heap and set bits 0,  splite nmemb
 * elements, so very element size is size / nmemb, really it is a array
 * @param [in] nmemb elements count
 * @param [in] size the size of we want memory, uint is byte
 * @return the virtual memory start address
 */
void *calloc(size_t nmemb, size_t size);

/**
 * @brief alloc a new memory, it's size is @param size, and the data of ptr will
 * be copy into the new memory
 * @param [in] ptr old memory
 * @param [in] size new memory size
 * @return the virtual memory start address
 */
void *realloc(void *ptr, size_t size);

/**
 * @brief alloc size virtual memory, it will be align @param aligment bit
 * @param [in] aligment the align address
 * @param [in] size the size of we want memory, unit is byte
 * @return the virtual memory start address
*/
void *memalign(size_t aligment, size_t size);
/**
 * @brief implement by memalign with aligment is MEM_PAGE_SIZE(4K)
*/
void *valloc(size_t size);

/**
 * @brief alloc from kernel memory space
*/
void* vmalloc(size_t size);

void vfree(void *ptr);

#endif