#define CREAT_DRIVERLAYER_STRUCT

#include "fsystem.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined SD_EN) && (SD_EN == 1)                               //
//================================================================//
#define CACHE_FAT_TABLE_SYNC		0x80
#define CACHE_FAT_TABLE_HIT			0x40
#define CACHE_FAT_TABLE_INIT		0x20
#define CACHE_FAT_TABLE_MISS		0x10

#define CACHE_FAT_TABLE_CNT	5

typedef struct {
	INT32U  cache_sector_idx;
	INT32U  cache_addrs;
	INT32U	cache_time;
	INT8U  	dirty_flag;
	INT8U  	is_used_flag;
}cache_info;

cache_info cache_fat_table[CACHE_FAT_TABLE_CNT]={0};

#define SDC_CACHE_START_ADDR    0xF800B000
#define SDC_CACHE_SIZE          (32*1024)
#define SDC_CACHE_SECTOR_NUMS   (SDC_CACHE_SIZE/512)

INT8U   cache_init_flag = 0;// cache has initialized, its value is set to 0xA8
INT8U   fat_cache_L1_en;	// if cache is intialized sucessfully, it will be set to 1
INT32U  fat_cluster_size;	// unit: sectors
INT32U  fat_tbl_size;		// unit: sectors
INT32U  fat_start_id[2];
static INT8U fat_type;		//FAT16 if its value is 16, or FAT32 if its value is 32
static INT32U fs_info_start;//sector no of FSINFO

INT32U  BPB_Start_id;		//the first sector no of DOS partition

#define SD_RW_RETRY_COUNT       3

void fat_l1_cache_reinit(void);
void fat_l1_cache_uninit(void);

static INT32S fat_l1_cache_init(void);

static INT32S SD_Initial(void);
static INT32S SD_Uninitial(void);
static void SD_GetDrvInfo(struct DrvInfo* info);
static INT32S SD_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf);
static INT32S SD_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf);
static INT32S SD_Flush(void);
static INT32S SD_WriteStart(INT32U blkno);
static INT32S SD_WriteContinue(INT32U blkcnt, INT32U buf);
static INT32S SD_WriteStop(void);

//--------------------------------------------------
struct Drv_FileSystem FS_SD_driver = {
	"SD",
	DEVICE_READ_ALLOW|DEVICE_WRITE_ALLOW,
	SD_Initial,
	SD_Uninitial,
	SD_GetDrvInfo,
	SD_ReadSector,
	SD_WriteSector,
	SD_Flush,
	SD_WriteStart,
	SD_WriteContinue,
	SD_WriteStop,
};

INT8U find_cache_fat_table_for_read(INT32U sectorIdx)
{
	INT8U i;
	INT32U max_time_range = OSTimeGet();
	INT8U select_idx = 0;

	for(i=0; i<CACHE_FAT_TABLE_CNT; i++)
	{
		// Return unused cache fat table
		if(cache_fat_table[i].is_used_flag == 0)
		{
			return (i|CACHE_FAT_TABLE_INIT);
		}

		// When all table is full, find the oldest cache time of cache fat table to will replace it later     
		if(max_time_range >= cache_fat_table[i].cache_time)
		{
			max_time_range = cache_fat_table[i].cache_time;
			select_idx = i;
		}

		// BIGO! Return cache fat table	index
		if((sectorIdx >=cache_fat_table[i].cache_sector_idx) &&
		   (sectorIdx < (cache_fat_table[i].cache_sector_idx+SDC_CACHE_SECTOR_NUMS))
		)
		{
			return (i|CACHE_FAT_TABLE_HIT);
		}
	}
	
	return (select_idx|CACHE_FAT_TABLE_MISS);
}

INT8U find_cache_fat_table_for_write(INT32U sectorIdx)
{
	INT8U i;
	INT32U max_time_range = OSTimeGet();
	INT8U select_idx = 0;

	for(i=0; i<CACHE_FAT_TABLE_CNT; i++)
	{
		if(cache_fat_table[i].is_used_flag == 0)
		{
			return (i|CACHE_FAT_TABLE_INIT);
		}
		
		if(max_time_range >= cache_fat_table[i].cache_time)
		{
			max_time_range = cache_fat_table[i].cache_time;
			select_idx = i;
		}
		
		if((sectorIdx >=cache_fat_table[i].cache_sector_idx) &&
		   (sectorIdx < (cache_fat_table[i].cache_sector_idx+SDC_CACHE_SECTOR_NUMS))
		)
		{
			return (i|CACHE_FAT_TABLE_HIT);
		}
	}
	
	return (select_idx|CACHE_FAT_TABLE_SYNC);
}


void fat_l1_cache_uninit(void)
{
    fat_cache_L1_en = 0;
}

void fat_l1_cache_reinit(void)
{
    cache_init_flag = 0;
    fat_l1_cache_init();
}


INT32S fat_l1_cache_init(void)
{
    INT32S ret;
    INT8U *byte_buf;
    INT16U *short_buf;
    INT32U *word_buf;
    INT32U Next_Free = 0xFFFFFFFF;
    INT32U FS_info_tag = 0x00000000;
    INT32U Free_Count = 0xFFFFFFFF;
  ///#if FS_INFO_CACHE == 1
    ///INT32U *fi_word_buf = (INT32U *) &sd_fs_info_ram[0];
    INT32U SD_Buf_Start_addr;
  ///#endif

    if (cache_init_flag != 0xA8)
    {
      ///#if FS_CACHE_DBG_EN == 1
      ///  DBG_PRINT ("SD FatL1 cache Init...");
      ///#endif
        SD_Buf_Start_addr = SDC_CACHE_START_ADDR;
        fs_info_start = 0xFFFFFFFF;

        BPB_Start_id = 0;  // NO MBR, BPB always is 0

        ///short_buf = (INT16U *) &sd_cache_ram[0];
        ///word_buf = (INT32U *) &sd_cache_ram[0];
        ///byte_buf = (INT8U *) &sd_cache_ram[0];
		short_buf = (INT16U *) SD_Buf_Start_addr;
        word_buf = (INT32U *) SD_Buf_Start_addr;
        byte_buf = (INT8U *) SD_Buf_Start_addr;


		///#if FS_FLUSH_CAHCE_EN
		for(ret=0; ret<CACHE_FAT_TABLE_CNT; ret++)
		{
			cache_fat_table[ret].cache_sector_idx = 0;
			cache_fat_table[ret].dirty_flag = 0;
			cache_fat_table[ret].cache_time = 0;
			cache_fat_table[ret].is_used_flag = 0;
			cache_fat_table[ret].cache_addrs = (INT32U)(SD_Buf_Start_addr+(ret*SDC_CACHE_SIZE));
		}
		///#endif

        ///ret = drvl2_sd_read(BPB_Start_id, (INT32U *) &sd_cache_ram[0], 1);
        ret = drvl2_sd_read(BPB_Start_id, (INT32U *) SD_Buf_Start_addr, 1);
        if(ret==0 && short_buf[0x1fE/2] == 0xAA55 && byte_buf[0] != 0xEB) 
		{
            BPB_Start_id = ((short_buf[0x1C8/2]<<16) | short_buf[0x1C6/2]);
            DBG_PRINT ("MBR Find, first part offset :0x%x\r\n",BPB_Start_id);
            ///ret = drvl2_sd_read(BPB_Start_id, (INT32U *) &sd_cache_ram[0], 1);
            ret = drvl2_sd_read(BPB_Start_id, (INT32U *) SD_Buf_Start_addr, 1);
        }

        if(ret != 0) 
		{
            fat_cache_L1_en = 0;

			DBG_PRINT("SD read fail 1\r\n");
            return -1;

		} 
		else if(ret == 0) 
		{
            fat_cache_L1_en = 1;

            fat_cluster_size = byte_buf[13];  // 8 sectors
            fat_start_id[0] = short_buf[7] + BPB_Start_id;   //0x24*0x200=0x4800 

            fat_tbl_size = short_buf[22/2];	//this value must be set to 0 in FAT32
            if(fat_tbl_size == 0) 
			{
                //find FAT32
                fat_type = 32;
                DBG_PRINT ("FAT32\r\n");

                fat_tbl_size = word_buf[9];  //0x1d72*0x200=0x3AE400
                fs_info_start = short_buf[48/2] + BPB_Start_id;

              /*#if FS_INFO_CACHE == 1
                if ( drvl2_sd_read(fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1)!=0 ) 
				{
					fsinfo_loaded_flag = 0;
					DBG_PRINT("SD read fail 2\r\n");
				} 
				else 
				{
					fsinfo_loaded_flag = 1;
				}
				fsinfo_dirty_flag = 0;

                FS_info_tag = fi_word_buf[0];
                Free_Count = fi_word_buf[488/4] + BPB_Start_id;  //should not add BPB_Start_id, wwj comment@20140929
                Next_Free = fi_word_buf[492/4] + BPB_Start_id;
              #else*/
                ///if (drvl2_sd_read(fs_info_start, (INT32U *) &sd_cache_ram[0], 1) != 0) 
                if (drvl2_sd_read(fs_info_start, (INT32U *) SD_Buf_Start_addr, 1) != 0) 
                {
					DBG_PRINT("SD read fail 2\r\n");
				}
                FS_info_tag = word_buf[0];
                Free_Count = word_buf[488/4] + BPB_Start_id;  //should not add BPB_Start_id, wwj comment@20140929
                Next_Free = word_buf[492/4] + BPB_Start_id;
              ///#endif

                DBG_PRINT ("Fs info Tag: 0x%x (SectorId:%d)\r\n", FS_info_tag, fs_info_start);

              /*#if FAT_WRITE_TBL_ID == 1  //FAT2->FAT1
                if (FS_info_tag == 0x41615252) {
                    if ((FSI_Free_Count == Free_Count) && (FSI_Next_Free==Next_Free)) 
                    {
                        mirror_en=0;
                    }
                }
              #endif*/

            }
			else 
			{
                fat_type = 16;
                DBG_PRINT ("FAT16\r\n");
            }
            fat_start_id[1] = fat_start_id[0] + fat_tbl_size; // 0x3AE400+0x4800=0x3B2C00
        }

      /*#if FAT_WRITE_TBL_ID==1
        if (mirror_en) {
            SD_tbl_mirror_init(); // Mirror initial FAT1->FAT2
        } else {
            DBG_PRINT ("Tbl Mirror initial avoid\r\n");
        }
      #endif*/
        ///mirror_start_id = 0x3FFFFFFF;
        ///mirror_end_id = 0;  // default mirror start

      /*#if FS_CACHE_DBG_EN == 1
        DBG_PRINT ("\r\nfat_cluster_size:%d\r\n",fat_cluster_size);
        DBG_PRINT ("fat_start_id[0]:%d\r\n",fat_start_id[0]);
        DBG_PRINT ("fat_start_id[1]:%d\r\n",fat_start_id[1]);
        DBG_PRINT ("fat_tbl_size:%d\r\n",fat_tbl_size);
      #endif*/
        cache_init_flag = 0xA8;
    }
    return 0;
}

INT32S SD_fill_fat_cache(INT32U start_clus, INT32U num_clus)
{
  ///#if FS_FLUSH_CAHCE_EN==1
	INT8U retIdx, i;
	INT32U blkno;
	INT32S ret;
	INT32U addr1, addr2;
	INT32U *ptr, *ptr_end;

    if(fat_cache_L1_en && num_clus)
    {
	    while(1) 
		{
			blkno = fat_start_id[0] + (start_clus * 4)/512;
			retIdx = find_cache_fat_table_for_write(blkno);
			if((retIdx & CACHE_FAT_TABLE_SYNC) || (retIdx & CACHE_FAT_TABLE_INIT))
			{
				// Write the replaced data from the cache fat table to the SDC
				if(retIdx & CACHE_FAT_TABLE_SYNC) 
				{
					retIdx &= ~(CACHE_FAT_TABLE_SYNC);
					if(cache_fat_table[retIdx].dirty_flag) 
					{
				      ///#if FS_CACHE_DBG_EN == 1
				      ///  DBG_PRINT ("w");
				      ///#endif
						cache_fat_table[retIdx].dirty_flag = 0;
						for(i=0; i<SD_RW_RETRY_COUNT; i++) 
						{
							ret = drvl2_sd_write(cache_fat_table[retIdx].cache_sector_idx, (INT32U *)cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
							if(ret == 0) 
							{
								break;
							}
						}
					}
				} 
				else 
				{

			      ///#if FS_CACHE_DBG_EN == 1
			      ///  DBG_PRINT ("i");
			      ///#endif
					retIdx &= ~(CACHE_FAT_TABLE_INIT);
				}

				// Read data from SDC and store it into cache fat table
				cache_fat_table[retIdx].cache_sector_idx = ((blkno/SDC_CACHE_SECTOR_NUMS)*SDC_CACHE_SECTOR_NUMS);
				
				for(i=0; i<SD_RW_RETRY_COUNT; i++) 
				{
					ret = drvl2_sd_read(cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
					if(ret == 0) 
					{
						break;
					}
				}
				
				cache_fat_table[retIdx].cache_time = OSTimeGet();
				cache_fat_table[retIdx].is_used_flag = 1;
				cache_fat_table[retIdx].dirty_flag = 1;

			}
			else if(retIdx & CACHE_FAT_TABLE_HIT)
			{

		      ///#if FS_CACHE_DBG_EN == 1
		        //DBG_PRINT ("h");
		      ///#endif
				retIdx &= ~(CACHE_FAT_TABLE_HIT);

				cache_fat_table[retIdx].dirty_flag = 1;
			}

			addr1 = fat_start_id[0] * 512 + (start_clus * 4) - cache_fat_table[retIdx].cache_sector_idx * 512 + cache_fat_table[retIdx].cache_addrs;
			ptr = (INT32U *)addr1;
			addr2 = cache_fat_table[retIdx].cache_addrs + SDC_CACHE_SIZE;
			ptr_end = (INT32U *)addr2;
			while(ptr < ptr_end) 
			{
				*ptr++ = start_clus + 1;
				start_clus++;
				num_clus--;
				if(num_clus == 0) 
				{
					ptr--;
					*ptr = LONG_LAST_CLUSTER;
					break;
				}
			}
			if(num_clus == 0) return STATUS_OK;
		}
    }
  ///#endif 

	return STATUS_FAIL;
}

INT32S SD_Initial(void)
{
    INT32S ret;

	ret = drvl2_sd_init();
	if (ret == 0)
	{
	    fat_l1_cache_init();
	}
	else
	{
		DBG_PRINT("FS drvl2_sd_init fail\r\n");
	}
	fs_sd_ms_plug_out_flag_reset();
	return ret; 
}

INT32S SD_Uninitial(void)
{
     drvl2_sd_card_remove();
	 return 0;
}

void SD_GetDrvInfo(struct DrvInfo* info)
{
	info->nSectors = drvl2_sd_sector_number_get();
	info->nBytesPerSector = 512;
}

INT32S SD_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}

	for(i=0; i<SD_RW_RETRY_COUNT; i++) {
		ret = drvl2_sd_read(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0) {
			break;
		} else {
			DBG_PRINT("SD read fail 8\r\n");
		}
	}

#if SUPPORT_STG_SUPER_PLUGOUT == 1
    if(ret != 0) {
        //if(drvl2_sd_read(0, (INT32U *) buf, 1) != 0)
        {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
#endif

	return ret;
}


INT32S SD_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    if(fs_sd_ms_plug_out_flag_get() == 1) {return 0xFFFFFFFF;}

	for(i=0; i<SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd_write(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0) {
			break;
		} else {
			DBG_PRINT("SD write fail 5\r\n");
		}
	}

#if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) {
        if (drvl2_sd_read(0, (INT32U *) buf, 1) != 0) {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
#endif

	return ret;
}

INT32S SD_Flush(void)
{

    //DBG_PRINT ("+++++++SD1_Flush\r\n");
	return 0;
}


INT32S SD_WriteStart(INT32U blkno)
{
	if(drvl2_sd_write_start(blkno) != 0) {
		return -1;
	}
	return 0;
}


INT32S SD_WriteContinue(INT32U blkcnt, INT32U buf)
{
	if(drvl2_sd_write_sector((INT32U *)buf, blkcnt, 1) != 0) {
		return -1;
	}
	return 0;
}


INT32S SD_WriteStop(void)
{
	if(drvl2_sd_write_stop() != 0) {
		return -1;
	}
	return 0;
}


//=== SD 1 setting ===//
#if (SD_DUAL_SUPPORT==1)

INT32S SD1_Initial(void)
{
	return drvl2_sd1_init();
}

INT32S SD1_Uninitial(void)
{
     drvl2_sd1_card_remove();
	 return 0;
}

void SD1_GetDrvInfo(struct DrvInfo* info)
{
	info->nSectors = drvl2_sd1_sector_number_get();
	info->nBytesPerSector = 512;
}

//read/write speed test function
INT32S SD1_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}
	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd1_read(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
	}
  #if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) 
    {
        //if (drvl2_sd_read(0, (INT32U *) buf, 1)!=0)
        {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
  #endif
	return ret;
}

INT32S SD1_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;

    if (fs_sd_ms_plug_out_flag_get()==1) {return 0xFFFFFFFF;}
	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd1_write(blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
	}
  #if SUPPORT_STG_SUPER_PLUGOUT == 1
    if (ret!=0) 
    {
        if (drvl2_sd1_read(0, (INT32U *) buf, 1)!=0)
        {
            fs_sd_ms_plug_out_flag_en();
            DBG_PRINT ("============>SUPER PLUG OUT DETECTED<===========\r\n");
        }
    }
  #endif
	return ret;
}

INT32S SD1_Flush(void)
{

    //DBG_PRINT ("+++++++SD1_Flush\r\n");
	return 0;
}

struct Drv_FileSystem FS_SD1_driver = {
	"SD1",
	DEVICE_READ_ALLOW|DEVICE_WRITE_ALLOW,
	SD1_Initial,
	SD1_Uninitial,
	SD1_GetDrvInfo,
	SD1_ReadSector,
	SD1_WriteSector,
	SD1_Flush,
};
#endif

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(defined SD_EN) && (SD_EN == 1)                          //
//================================================================//

