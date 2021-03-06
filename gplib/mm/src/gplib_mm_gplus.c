/*
* Description: A simple memory management module
*
* Author: Tristan Yang
*
* Date: 2008/02/12
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
*/
#include "gplib_mm_gplus.h"

#if (defined GPLIB_MEMORY_MANAGEMENT_EN) && (GPLIB_MEMORY_MANAGEMENT_EN == 1)

#ifndef C_MM_IRAM_LINK_NUM
	#define C_MM_IRAM_LINK_NUM		20
#endif

#ifndef C_MM_16BYTE_NUM
	#define C_MM_16BYTE_NUM			0
#endif
#ifndef C_MM_64BYTE_NUM
	#define C_MM_64BYTE_NUM			0
#endif
#ifndef C_MM_256BYTE_NUM
	#define C_MM_256BYTE_NUM		0
#endif
#ifndef C_MM_1024BYTE_NUM
	#define C_MM_1024BYTE_NUM		0
#endif
#ifndef C_MM_4096BYTE_NUM
	#define C_MM_4096BYTE_NUM		0
#endif

#define C_MM_DEBUG_MODE				2

typedef struct _mm_sdram_struct {
	struct _mm_sdram_struct *next;			// Point to next management entry of linking list
	INT32U *end;							// Point to the last word(4 bytes) of allocated memory
} MM_SDRAM_STRUCT, *P_MM_SDRAM_STRUCT;


#if C_MM_16BYTE_NUM
static INT8U mm_fix_16_flag[(C_MM_16BYTE_NUM+7)/8];
static INT32U mm_fix_16_addr[C_MM_16BYTE_NUM];
#endif
#if C_MM_64BYTE_NUM
static INT8U mm_fix_64_flag[(C_MM_64BYTE_NUM+7)/8];
static INT32U mm_fix_64_addr[C_MM_64BYTE_NUM];
#endif
#if C_MM_256BYTE_NUM
static INT8U mm_fix_256_flag[(C_MM_256BYTE_NUM+7)/8];
static INT32U mm_fix_256_addr[C_MM_256BYTE_NUM];
#endif
#if C_MM_1024BYTE_NUM
static INT8U mm_fix_1024_flag[(C_MM_1024BYTE_NUM+7)/8];
static INT32U mm_fix_1024_addr[C_MM_1024BYTE_NUM];
#endif
#if C_MM_4096BYTE_NUM
static INT8U mm_fix_4096_flag[(C_MM_4096BYTE_NUM+7)/8];
static INT32U mm_fix_4096_addr[C_MM_4096BYTE_NUM];
#endif

static MM_SDRAM_STRUCT *gp_mm_sdram_head;
static INT32U free_sdram_start;
static INT32U free_sdram_end;

#if C_MM_DEBUG_MODE > 1
INT32U fix_16_alloc_count = 0;
INT32U fix_64_alloc_count = 0;
INT32U fix_256_alloc_count = 0;
INT32U fix_1024_alloc_count = 0;
INT32U fix_4096_alloc_count = 0;
INT32U fix_16_alloc_count_max = 0;
INT32U fix_64_alloc_count_max = 0;
INT32U fix_256_alloc_count_max = 0;
INT32U fix_1024_alloc_count_max = 0;
INT32U fix_4096_alloc_count_max = 0;
#endif
#if C_MM_DEBUG_MODE > 0
INT32U sdram_alloc_count = 0;
INT32U sdram_alloc_count_max = 0;
#endif


#if _OPERATING_SYSTEM == 1	
static OS_EVENT		*sw_gplib_sem = NULL;
#endif

#if _OPERATING_SYSTEM == 1	
void sw_gplib_lock(void)
{
	INT8U err;

	OSSemPend(sw_gplib_sem, 0, &err);
}

void sw_gplib_unlock(void)
{
	OSSemPost(sw_gplib_sem);
}
#endif


void gp_mm_init(INT32U free_memory_start, INT32U free_memory_end)
{
	INT32U i = i, ptr;

	ptr = (free_memory_start + 3) & (~0x3);		// Align to 4-bytes boundary

	// Initate management structure for fixed size free SDRAM
  #if C_MM_16BYTE_NUM
	// Initiate 16-byte fixed size memory blocks
	for (i=0; i<C_MM_16BYTE_NUM; i++) {
		mm_fix_16_addr[i] = ptr;
		ptr += 16;
	}
	for (i=0; i<(C_MM_16BYTE_NUM+7)/8; i++) {
		mm_fix_16_flag[i] = 0x0;
	}
  #endif

  #if C_MM_64BYTE_NUM
	// Initiate 64-byte fixed size memory blocks
	for (i=0; i<C_MM_64BYTE_NUM; i++) {
		mm_fix_64_addr[i] = ptr;
		ptr += 64;
	}
	for (i=0; i<(C_MM_64BYTE_NUM+7)/8; i++) {
		mm_fix_64_flag[i] = 0x0;
	}
  #endif

  #if C_MM_256BYTE_NUM
	// Initiate 256-byte fixed size memory blocks
	for (i=0; i<C_MM_256BYTE_NUM; i++) {
		mm_fix_256_addr[i] = ptr;
		ptr += 256;
	}
	for (i=0; i<(C_MM_256BYTE_NUM+7)/8; i++) {
		mm_fix_256_flag[i] = 0x0;
	}
  #endif

  #if C_MM_1024BYTE_NUM
	// Initiate 1024-byte fixed size memory blocks
	for (i=0; i<C_MM_1024BYTE_NUM; i++) {
		mm_fix_1024_addr[i] = ptr;
		ptr += 1024;
	}
	for (i=0; i<(C_MM_1024BYTE_NUM+7)/8; i++) {
		mm_fix_1024_flag[i] = 0x0;
	}
  #endif

  #if C_MM_4096BYTE_NUM
	// Initiate 4096-byte fixed size memory blocks
	for (i=0; i<C_MM_4096BYTE_NUM; i++) {
		mm_fix_4096_addr[i] = ptr;
		ptr += 4096;
	}
	for (i=0; i<(C_MM_4096BYTE_NUM+7)/8; i++) {
		mm_fix_4096_flag[i] = 0x0;
	}
  #endif

	// Initate management structure for variable size free SDRAM
	free_sdram_start = ptr;
	free_sdram_end = free_memory_end & (~0x3);				// Align to 4-bytes boundary

    gp_mm_sdram_head = (MM_SDRAM_STRUCT *) free_sdram_start;
    gp_mm_sdram_head->end = (INT32U *) &gp_mm_sdram_head->end;
    gp_mm_sdram_head->next = (MM_SDRAM_STRUCT *) free_sdram_end - 1;
    gp_mm_sdram_head->next->next = NULL;

#if _OPERATING_SYSTEM == 1	
	if(sw_gplib_sem == NULL)
	{
		sw_gplib_sem = OSSemCreate(1);
	}
#endif    
    
}

#if C_MM_16BYTE_NUM || C_MM_64BYTE_NUM || C_MM_256BYTE_NUM || C_MM_1024BYTE_NUM || C_MM_4096BYTE_NUM
void * gp_fixed_size_malloc(INT32U size)
{
	INT16U i, j;

  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_lock();
  #endif
  
  #if C_MM_16BYTE_NUM
	if (size <= 16) {
		for (i=0; i<C_MM_16BYTE_NUM/8; i++) {
			if (mm_fix_16_flag[i] != 0xFF) {
				for (j=0; j<8; j++) {
					if (!(mm_fix_16_flag[i]&(1<<j))) {
						mm_fix_16_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_16_alloc_count++;
if (fix_16_alloc_count > fix_16_alloc_count_max) fix_16_alloc_count_max = fix_16_alloc_count;
#endif
					  #if _OPERATING_SYSTEM != _OS_NONE
					    sw_gplib_unlock();
					  #endif

						return (void *) mm_fix_16_addr[(i<<3) + j];
					}
				}
			}
		}
	  #if C_MM_16BYTE_NUM & 0x7
		i = (C_MM_16BYTE_NUM+7)/8 - 1;
		for (j=0; j<(C_MM_16BYTE_NUM & 0x7); j++) {
			if (!(mm_fix_16_flag[i]&(1<<j))) {
				mm_fix_16_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_16_alloc_count++;
if (fix_16_alloc_count > fix_16_alloc_count_max) fix_16_alloc_count_max = fix_16_alloc_count;
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

				return (void *) mm_fix_16_addr[(i<<3) + j];
			}
		}
	  #endif
	}
  #endif

  #if C_MM_64BYTE_NUM
	if (size <= 64) {
		for (i=0; i<C_MM_64BYTE_NUM/8; i++) {
			if (mm_fix_64_flag[i] != 0xFF) {
				for (j=0; j<8; j++) {
					if (!(mm_fix_64_flag[i]&(1<<j))) {
						mm_fix_64_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_64_alloc_count++;
if (fix_64_alloc_count > fix_64_alloc_count_max) fix_64_alloc_count_max = fix_64_alloc_count;
#endif
					  #if _OPERATING_SYSTEM != _OS_NONE
					    sw_gplib_unlock();
					  #endif

						return (void *) mm_fix_64_addr[(i<<3) + j];
					}
				}
			}
		}
	  #if C_MM_64BYTE_NUM & 0x7
		i = (C_MM_64BYTE_NUM+7)/8 - 1;
		for (j=0; j<(C_MM_64BYTE_NUM & 0x7); j++) {
			if (!(mm_fix_64_flag[i]&(1<<j))) {
				mm_fix_64_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_64_alloc_count++;
if (fix_64_alloc_count > fix_64_alloc_count_max) fix_64_alloc_count_max = fix_64_alloc_count;
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

				return (void *) mm_fix_64_addr[(i<<3) + j];
			}
		}
	  #endif
	}
  #endif

  #if C_MM_256BYTE_NUM
	if (size <= 256) {
		for (i=0; i<C_MM_256BYTE_NUM/8; i++) {
			if (mm_fix_256_flag[i] != 0xFF) {
				for (j=0; j<8; j++) {
					if (!(mm_fix_256_flag[i]&(1<<j))) {
						mm_fix_256_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_256_alloc_count++;
if (fix_256_alloc_count > fix_256_alloc_count_max) fix_256_alloc_count_max = fix_256_alloc_count;
#endif
					  #if _OPERATING_SYSTEM != _OS_NONE
					    sw_gplib_unlock();
					  #endif

						return (void *) mm_fix_256_addr[(i<<3) + j];
					}
				}
			}
		}
	  #if C_MM_256BYTE_NUM & 0x7
		i = (C_MM_256BYTE_NUM+7)/8 - 1;
		for (j=0; j<(C_MM_256BYTE_NUM & 0x7); j++) {
			if (!(mm_fix_256_flag[i]&(1<<j))) {
				mm_fix_256_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_256_alloc_count++;
if (fix_256_alloc_count > fix_256_alloc_count_max) fix_256_alloc_count_max = fix_256_alloc_count;
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

				return (void *) mm_fix_256_addr[(i<<3) + j];
			}
		}
	  #endif
	}
  #endif

  #if C_MM_1024BYTE_NUM
	if (size <= 1024) {
		for (i=0; i<C_MM_1024BYTE_NUM/8; i++) {
			if (mm_fix_1024_flag[i] != 0xFF) {
				for (j=0; j<8; j++) {
					if (!(mm_fix_1024_flag[i]&(1<<j))) {
						mm_fix_1024_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_1024_alloc_count++;
if (fix_1024_alloc_count > fix_1024_alloc_count_max) fix_1024_alloc_count_max = fix_1024_alloc_count;
#endif
					  #if _OPERATING_SYSTEM != _OS_NONE
					    sw_gplib_unlock();
					  #endif

						return (void *) mm_fix_1024_addr[(i<<3) + j];
					}
				}
			}
		}
	  #if C_MM_1024BYTE_NUM & 0x7
		i = (C_MM_1024BYTE_NUM+7)/8 - 1;
		for (j=0; j<(C_MM_1024BYTE_NUM & 0x7); j++) {
			if (!(mm_fix_1024_flag[i]&(1<<j))) {
				mm_fix_1024_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_1024_alloc_count++;
if (fix_1024_alloc_count > fix_1024_alloc_count_max) fix_1024_alloc_count_max = fix_1024_alloc_count;
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

				return (void *) mm_fix_1024_addr[(i<<3) + j];
			}
		}
	  #endif
	}
  #endif

  #if C_MM_4096BYTE_NUM
	if (size <= 4096) {
		for (i=0; i<C_MM_4096BYTE_NUM/8; i++) {
			if (mm_fix_4096_flag[i] != 0xFF) {
				for (j=0; j<8; j++) {
					if (!(mm_fix_4096_flag[i]&(1<<j))) {
						mm_fix_4096_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_4096_alloc_count++;
if (fix_4096_alloc_count > fix_4096_alloc_count_max) fix_4096_alloc_count_max = fix_4096_alloc_count;
#endif
					  #if _OPERATING_SYSTEM != _OS_NONE
					    sw_gplib_unlock();
					  #endif

						return (void *) mm_fix_4096_addr[(i<<3) + j];
					}
				}
			}
		}
	  #if C_MM_4096BYTE_NUM & 0x7
		i = (C_MM_4096BYTE_NUM+7)/8 - 1;
		for (j=0; j<(C_MM_4096BYTE_NUM & 0x7); j++) {
			if (!(mm_fix_4096_flag[i]&(1<<j))) {
				mm_fix_4096_flag[i] |= (1<<j);
#if C_MM_DEBUG_MODE > 1
fix_4096_alloc_count++;
if (fix_4096_alloc_count > fix_4096_alloc_count_max) fix_4096_alloc_count_max = fix_4096_alloc_count;
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

				return (void *) mm_fix_4096_addr[(i<<3) + j];
			}
		}
	  #endif
	}
  #endif

  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_unlock();
  #endif

	return NULL;
}

void gp_fixed_size_free(void *ptr)
{
	INT16U i;

  #if C_MM_16BYTE_NUM
  	if (((INT32U) ptr<=mm_fix_16_addr[C_MM_16BYTE_NUM-1]) && ((INT32U) ptr>=mm_fix_16_addr[0])) {
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_lock();
	  #endif
  		for (i=0; i<C_MM_16BYTE_NUM; i++) {
  			if ((INT32U) ptr == mm_fix_16_addr[i]) {
  				mm_fix_16_flag[i>>3] &= ~(1 << (i&0x7));
#if C_MM_DEBUG_MODE > 1
if (fix_16_alloc_count==0) {
	DBG_PRINT("MM fix16 is 0\r\n");
} else {
	fix_16_alloc_count--;
}
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

  				return;
  			}
  		}
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_unlock();
	  #endif

  		return;
  	}
  #endif

  #if C_MM_64BYTE_NUM
  	if (((INT32U) ptr<=mm_fix_64_addr[C_MM_64BYTE_NUM-1]) && ((INT32U) ptr>=mm_fix_64_addr[0])) {
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_lock();
	  #endif
  		for (i=0; i<C_MM_64BYTE_NUM; i++) {
  			if ((INT32U) ptr == mm_fix_64_addr[i]) {
  				mm_fix_64_flag[i>>3] &= ~(1 << (i&0x7));
#if C_MM_DEBUG_MODE > 1
if (fix_64_alloc_count==0) {
	DBG_PRINT("MM fix64 is 0\r\n");
} else {
	fix_64_alloc_count--;
}
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

  				return;
  			}
  		}
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_unlock();
	  #endif

  		return;
  	}
  #endif

  #if C_MM_256BYTE_NUM
  	if (((INT32U) ptr<=mm_fix_256_addr[C_MM_256BYTE_NUM-1]) && ((INT32U) ptr>=mm_fix_256_addr[0])) {
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_lock();
	  #endif
  		for (i=0; i<C_MM_256BYTE_NUM; i++) {
  			if ((INT32U) ptr == mm_fix_256_addr[i]) {
  				mm_fix_256_flag[i>>3] &= ~(1 << (i&0x7));
#if C_MM_DEBUG_MODE > 1
if (fix_256_alloc_count==0) {
	DBG_PRINT("MM fix256 is 0\r\n");
} else {
	fix_256_alloc_count--;
}
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

  				return;
  			}
  		}
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_unlock();
	  #endif

  		return;
  	}
  #endif

  #if C_MM_1024BYTE_NUM
  	if (((INT32U) ptr<=mm_fix_1024_addr[C_MM_1024BYTE_NUM-1]) && ((INT32U) ptr>=mm_fix_1024_addr[0])) {
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_lock();
	  #endif
  		for (i=0; i<C_MM_1024BYTE_NUM; i++) {
  			if ((INT32U) ptr == mm_fix_1024_addr[i]) {
  				mm_fix_1024_flag[i>>3] &= ~(1 << (i&0x7));
#if C_MM_DEBUG_MODE > 1
if (fix_1024_alloc_count==0) {
	DBG_PRINT("MM fix1024 is 0\r\n");
} else {
	fix_1024_alloc_count--;
}
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

  				return;
  			}
  		}
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_unlock();
	  #endif

  		return;
  	}
  #endif

  #if C_MM_4096BYTE_NUM
  	if (((INT32U) ptr<=mm_fix_4096_addr[C_MM_4096BYTE_NUM-1]) && ((INT32U) ptr>=mm_fix_4096_addr[0])) {
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_lock();
	  #endif
  		for (i=0; i<C_MM_4096BYTE_NUM; i++) {
  			if ((INT32U) ptr == mm_fix_4096_addr[i]) {
  				mm_fix_4096_flag[i>>3] &= ~(1 << (i&0x7));
#if C_MM_DEBUG_MODE > 1
if (fix_4096_alloc_count==0) {
	DBG_PRINT("MM fix4096 is 0\r\n");
} else {
	fix_4096_alloc_count--;
}
#endif
			  #if _OPERATING_SYSTEM != _OS_NONE
			    sw_gplib_unlock();
			  #endif

  				return;
  			}
  		}
	  #if _OPERATING_SYSTEM != _OS_NONE
	    sw_gplib_unlock();
	  #endif

  		return;
  	}
  #endif
#if C_MM_DEBUG_MODE > 1
DBG_PRINT("gp_fixed_size_free can not find allocate ptr=0x%x\r\n", ptr);
#endif
}
#endif

#if C_MM_DEBUG_MODE > 1
void mm_dump(void)
{
	MM_SDRAM_STRUCT *pmm;

	DBG_PRINT("MM Dump\r\n");
	DBG_PRINT("  Fix16 now=%d max=%d total=%d\r\n", fix_16_alloc_count, fix_16_alloc_count_max, C_MM_16BYTE_NUM);
	DBG_PRINT("  Fix64 now=%d max=%d total=%d\r\n", fix_64_alloc_count, fix_64_alloc_count_max, C_MM_64BYTE_NUM);
	DBG_PRINT("  Fix256 now=%d max=%d total=%d\r\n", fix_256_alloc_count, fix_256_alloc_count_max, C_MM_256BYTE_NUM);
	DBG_PRINT("  Fix1024 now=%d max=%d total=%d\r\n", fix_1024_alloc_count, fix_1024_alloc_count_max, C_MM_1024BYTE_NUM);
	DBG_PRINT("  Fix4096 now=%d max=%d total=%d\r\n", fix_4096_alloc_count, fix_4096_alloc_count_max, C_MM_4096BYTE_NUM);
	DBG_PRINT("  Free now=%d max=%d\r\n", sdram_alloc_count, sdram_alloc_count_max);

	pmm = gp_mm_sdram_head;
	while (pmm->next) {
    	if ((INT32U) (pmm->end) > (INT32U) (&pmm->end)) {
    		DBG_PRINT(" Used 0x%06X size=%d\r\n", pmm, (INT32U) (pmm->end) - (INT32U) (&pmm->end));
    	}
    	if ((INT32U) (pmm->next) != ((INT32U) (pmm->end)+4)) {
    		DBG_PRINT(" ____ 0x%06X size=%d\r\n", (INT32U) (pmm->end)+4, (INT32U) (pmm->next) - ((INT32U) (pmm->end)+4));
    	}
   		pmm=pmm->next;
   	}
}
#endif

void * gp_malloc_align(INT32U size, INT32U align)
{
    MM_SDRAM_STRUCT *pmm;
    INT32U new_head_addr;
    INT32U new_data_addr;

	if (!size) {
    	return (NULL);
    }

    pmm = gp_mm_sdram_head;
    size = (size+3) >> 2;
    if (align<4) align=4 ;
	if (align & (align-1)) {				// align value must be power of 2, eg: 2, 4, 8, 16, 32...
		align = 4;
	}

  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_lock();
  #endif
    while (pmm->next) {
    	new_data_addr = (((INT32U) (pmm->end+1))+sizeof(MM_SDRAM_STRUCT)+(align-1)) & ~(align-1);
        if (new_data_addr >= (INT32U) pmm->next) {
        	pmm=pmm->next;
        } else if (((INT32U *) pmm->next - (INT32U *) new_data_addr) >= size) {
        	new_head_addr = new_data_addr - sizeof(MM_SDRAM_STRUCT);
            ((MM_SDRAM_STRUCT *) new_head_addr)->next = pmm->next;
            ((MM_SDRAM_STRUCT *) new_head_addr)->end = (INT32U *) (new_data_addr + (size<<2)) - 1;
            pmm->next = (MM_SDRAM_STRUCT *) new_head_addr;
#if C_MM_DEBUG_MODE > 0
sdram_alloc_count++;
if (sdram_alloc_count > sdram_alloc_count_max) sdram_alloc_count_max = sdram_alloc_count;
#endif
		  #if _OPERATING_SYSTEM != _OS_NONE
            sw_gplib_unlock();
          #endif
            return ((void *) new_data_addr);
        } else {
            pmm=pmm->next;
        }
#if C_MM_DEBUG_MODE > 0
if ((INT32U) pmm<(INT32U) free_sdram_start || (INT32U) pmm>(INT32U) free_sdram_end) {
	DBG_PRINT("MM Error 1\r\n");
	mm_dump();
	return NULL;
}
#endif
    }
#if C_MM_DEBUG_MODE > 1
DBG_PRINT("MM fail in gp_malloc_align(size=%d align=%d)\r\n", size<<2, align);
mm_dump();
#endif
  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_unlock();
  #endif

    return (NULL);
}

void * gp_malloc(INT32U size)
{
	MM_SDRAM_STRUCT *pmm = gp_mm_sdram_head;

	if (!size) {
    	return (NULL);
    }
    

  #if C_MM_16BYTE_NUM || C_MM_64BYTE_NUM || C_MM_256BYTE_NUM || C_MM_1024BYTE_NUM || C_MM_4096BYTE_NUM
    if (size <= 4096) {
    	INT32U ptr;

    	ptr = (INT32U) gp_fixed_size_malloc(size);
    	if (ptr) {
    		return (void *) ptr;
    	}
#if C_MM_DEBUG_MODE > 1
DBG_PRINT("Fix number is not enough! size=%d\r\n", size);
#endif
    }
  #endif

    size = (size+3) >> 2;

  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_lock();
  #endif
    while (pmm->next) {
        if (((INT32U *) pmm->next - (INT32U *) pmm->end) > size+2) {
            ((MM_SDRAM_STRUCT *) (pmm->end+1))->next = pmm->next;
            pmm->next = (MM_SDRAM_STRUCT *) (pmm->end+1);
            pmm->next->end = (INT32U *) &pmm->next->end + size;
#if C_MM_DEBUG_MODE > 0
sdram_alloc_count++;
if (sdram_alloc_count > sdram_alloc_count_max) sdram_alloc_count_max = sdram_alloc_count;
#endif
		  #if _OPERATING_SYSTEM != _OS_NONE
            sw_gplib_unlock();
          #endif
            return ((INT32U *) &pmm->next->end + 1);
        }

        pmm=pmm->next;
#if C_MM_DEBUG_MODE > 0
if ((INT32U) pmm<(INT32U) free_sdram_start || (INT32U) pmm>(INT32U) free_sdram_end) {
	DBG_PRINT("MM Error 2\r\n");
	mm_dump();
	return NULL;
}
#endif
    }
#if C_MM_DEBUG_MODE > 1
DBG_PRINT("MM fail in gp_malloc(size=%d)\r\n", size<<2);
mm_dump();
#endif
  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_unlock();
  #endif

    return (NULL);
}

void gp_free(void *ptr)
{
    MM_SDRAM_STRUCT *pmm = gp_mm_sdram_head;

	if ((INT32U) ptr < free_sdram_start) {
      #if C_MM_16BYTE_NUM || C_MM_64BYTE_NUM || C_MM_256BYTE_NUM || C_MM_1024BYTE_NUM || C_MM_4096BYTE_NUM
		gp_fixed_size_free(ptr);
	  #endif

		return;
    } else if ((INT32U) ptr > free_sdram_end) {
    	return;
    }

  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_lock();
  #endif
    while (pmm->next) {
        if ((INT32U *) pmm->next+2 == ptr) {
            // Merge previous free memory with this one
#if C_MM_DEBUG_MODE > 0
if (sdram_alloc_count==0) {
	DBG_PRINT("MM variable is 0\r\n");
} else {
	sdram_alloc_count--;
}
if ((INT32U) pmm->next->next<(INT32U) free_sdram_start || (INT32U) pmm->next->next>(INT32U) free_sdram_end) {
	DBG_PRINT("MM Error 3\r\n");
	mm_dump();
}
#endif
            pmm->next = pmm->next->next;
          #if _OPERATING_SYSTEM != _OS_NONE
            sw_gplib_unlock();
          #endif
            return;
        }
        pmm = pmm->next;
#if C_MM_DEBUG_MODE > 0
if ((INT32U) pmm<(INT32U) free_sdram_start || (INT32U) pmm>(INT32U) free_sdram_end) {
	DBG_PRINT("MM Error 4\r\n");
	mm_dump();
	return;
}
#endif
    }
#if C_MM_DEBUG_MODE > 1
DBG_PRINT("ptr=0x%X is not found in gp_free\r\n", ptr);
#endif
    // Something must be wrong...
  #if _OPERATING_SYSTEM != _OS_NONE
    sw_gplib_unlock();
  #endif
}

#endif
