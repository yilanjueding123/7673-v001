#include "ap_storage_service.h"
#include "ap_state_config.h"
#include "ap_state_handling.h"
#include "fs_driver.h"
#include "avi_encoder_app.h"


#define BASE_SIZE              0x03200000  //50mb        
#define AVI_REC_MAX_BYTE_SIZE  0x77500000// 0x1f400000//500mb//0x77500000// 0x70000000  //1879048192 Bytes//78643200//4B00000//0x77500000//1909mb
#define AP_STG_MAX_FILE_NUMS    625
#define BKGROUND_DETECT_INTERVAL  (20*128)  // 20 second
#define BKGROUND_DEL_INTERVAL     (1*128)   // 1 second

extern INT8U screen_saver_enable;
static INT32S g_jpeg_index;
//extern STOR_SERV_PLAYINFO play_info;
static INT32S g_avi_index;
static INT32S g_wav_index;
static INT32S g_file_index;
static INT16S g_play_index;
static INT16U g_file_num;
static INT16U g_err_cnt;
static INT16U g_same_index_num;
static INT16S g_latest_avi_file_index;
static INT32U g_avi_file_time;
static INT32U g_jpg_file_time;
static INT8U g_avi_index_9999_exist;
static INT32U g_avi_file_oldest_time;
static INT16S g_oldest_avi_file_index=0;
static CHAR g_file_path[24];
//static CHAR g_next_file_path[24];
static INT8U curr_storage_id;
static INT8U storage_mount_timerid;
static INT8U first_storage_check_flag=0;
//static INT8U err_handle_timerid;
#if C_AUTO_DEL_FILE == CUSTOM_ON
	static INT8U storage_freesize_timerid;
#endif

INT8U usbd_storage_exit;
INT8U device_plug_phase=0;
INT32U gSelect_Size;
INT32U gOutOfFreeSpace;

static INT16U avi_file_table[625];
static INT16U jpg_file_table[625];
static INT16U wav_file_table[625];
static INT32U g_wav_file_time;
static INT32U BkDelThreadMB;
static INT8U sd_upgrade_file_flag = 0;

/*static*/ INT32U avi_file_size ;///= 150;  //default 150MB
////*static*/ INT32U threshold_size;

st_storage_file_node_info FNodeInfo[MAX_SLOT_NUMS];
#if 0//C_AUTO_DEL_FILE == CUSTOM_ON
/*static*/ INT8U ap_step_work_start=0;
static INT32S retry_del_idx=-1;
static INT8U retry_del_counts=0;
static INT8S bkground_del_disble=0;
static INT32U BkDelThreadMB;
#endif
static INT8U card_first_dete=1;
extern INT8U card_space_less_flag;
//	prototypes
INT32S get_file_final_wav_index(INT8U count_total_num_enable);
INT32S get_file_final_avi_index(INT8U count_total_num_enable);
INT32S get_file_final_jpeg_index(INT8U count_total_num_enable);
INT16U get_deleted_file_number(void);
INT16U get_same_index_file_number(void);

void ap_storage_service_init(void)
{
#if C_AUTO_DEL_FILE == CUSTOM_ON
	storage_freesize_timerid = 0xFF;
#endif
	g_play_index = -1;
	storage_mount_timerid = STORAGE_SERVICE_MOUNT_TIMER_ID;
	sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, storage_mount_timerid, STORAGE_TIME_INTERVAL_MOUNT);
}

INT8U storage_sd_upgrade_file_flag_get(void)
{
	return sd_upgrade_file_flag;
}

#if 0//C_AUTO_DEL_FILE == CUSTOM_ON
void bkground_del_disable(INT32U disable1_enable0)
{
    bkground_del_disble=disable1_enable0;
}

INT8S bkground_del_disable_status_get(void)
{
    return bkground_del_disble;
}

///#if C_AUTO_DEL_FILE == CUSTOM_ON
void ap_storage_service_freesize_check_switch(INT8U type)
{
	if (type == TRUE) {
		if (storage_freesize_timerid == 0xFF) {
			storage_freesize_timerid = STORAGE_SERVICE_FREESIZE_TIMER_ID;
			sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, STORAGE_TIME_FIRST_CHECK);//STORAGE_TIME_INTERVAL_FREESIZE
			first_storage_check_flag=0;
		}
	} else {
		if (storage_freesize_timerid != 0xFF) {
			sys_kill_timer(storage_freesize_timerid);
			storage_freesize_timerid = 0xFF;
		}
	}
}

void ap_storage_service_del_thread_mb_set(void)
{
	INT8U	temp[6] = {0, 1, 2, 3, 5, 10};
    INT32U	SDC_MB_Size;

	if(ap_state_config_video_resolution_get() >= 2) { //720P and below
		BkDelThreadMB = temp[ap_state_config_record_time_get()]*100;	//100MB per minute
		SDC_MB_Size = drvl2_sd_sector_number_get()/2048;

		if (BkDelThreadMB>(SDC_MB_Size/2) || (BkDelThreadMB==0)) {
			BkDelThreadMB=100;
		}
	} else {
		BkDelThreadMB = temp[ap_state_config_record_time_get()]*200;	//200MB per minute
		SDC_MB_Size = drvl2_sd_sector_number_get()/2048;

		if (BkDelThreadMB>(SDC_MB_Size/2) || (BkDelThreadMB==0)) {
			BkDelThreadMB=200;
		}
	}
}

INT32U bkground_del_thread_size_get(void)
{
	if (bkground_del_disble != 1) {
		return BkDelThreadMB;
	} else {
		return 0;
	}
}

INT32S ap_storage_service_freesize_check_and_del(void)
{
	CHAR f_name[24];
	INT32U i, j;
	INT32S del_index, ret;
	INT64U  disk_free_size;
    INT32S  step_ret;   
    INT16S del_ret;
	unsigned long file_size;
    struct stat_t buf_tmp;

    ret = STATUS_OK;

    if(ap_step_work_start==0)
    {
		disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
		ap_storage_service_del_thread_mb_set();
RE_DEL:
        DBG_PRINT("\r\n[Bkgnd Del Detect (DskFree: %d MB)]\r\n",disk_free_size);

        if (bkground_del_disble == 1)
			{
			if(first_storage_check_flag == 0)
				{
			     sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, STORAGE_TIME_INTERVAL_FREESIZE);//
			     first_storage_check_flag=1;
				}
            if (disk_free_size <= 50) 
				{
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128);
                // if sdc redundant size less than CARD_FULL_SIZE_RECORD, STOP recording now.
                if(disk_free_size < CARD_FULL_SIZE_RECORD)
					{
                     AviPacker_Break_Set(pAviEncPara->AviPackerCur->avi_workmem, 1);
                     msgQSend(ApQ, MSG_APQ_VDO_REC_STOP, NULL, NULL, MSG_PRI_NORMAL);
                     sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DETECT_INTERVAL);
                    }
               } 
		    else //if (current_movie_Byte_size_get(pAviEncPara->AviPackerCur->avi_workmem)>AVI_REC_MAX_BYTE_SIZE)
            {
                file_size=current_movie_Byte_size_get(pAviEncPara->AviPackerCur->avi_workmem);
				if(file_size >AVI_REC_MAX_BYTE_SIZE )
					{
                     DBG_PRINT ("AVI Max Size:%d MB Attend\r\n",current_movie_Byte_size_get(pAviEncPara->AviPackerCur->avi_workmem)>>20);
                     msgQSend(ApQ, MSG_APQ_VDO_REC_RESTART, NULL, NULL, MSG_PRI_NORMAL);
					}
				else if(file_size > (AVI_REC_MAX_BYTE_SIZE-BASE_SIZE))
					{
					 DBG_PRINT("close_to_file_max\r\n");
                     sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
					}
            }
            return 0;
        }
	#if 0
    	if (disk_free_size <= BkDelThreadMB) {
            if(g_avi_index_9999_exist) {
			    del_index = g_oldest_avi_file_index;
			    sprintf((char *)f_name, (const char *)"C:\\VIDEO\\MOVI%04d.avi", g_oldest_avi_file_index);
		    } else {
    		    del_index = -1;
    		    for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
        			if (avi_file_table[i]) {
        				for (j=0 ; j<16 ; j++) {
        					if (avi_file_table[i] & (1<<j)) {
        						del_index = (i << 4) + j;
        						sprintf((char *)f_name, (const char *)"C:\\VIDEO\\MOVI%04d.avi", del_index);
        						stat(f_name, &buf_tmp);
        						if(buf_tmp.st_mode == D_RDONLY) {
        							del_index = -1;
        						} else {
	        						break;
	        					}
        					}
        				}
        				if (del_index != -1) {
        					break;
        				}
        			}
    		    }
            }
  
            if (del_index == -1 || gp_strcmp((INT8S*)f_name,(INT8S*)g_file_path) == 0) {
        		if (disk_free_size <= 50) {
        		    msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, NULL, NULL, MSG_PRI_NORMAL);
        			bkground_del_disble = 1;
        		} else if(disk_free_size < 200) {
	                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128);
        		}
                return STATUS_FAIL;
            }
    		DBG_PRINT("\r\nDel <%s>\r\n", f_name);

            unlink_step_start();
            del_ret = unlink(f_name);
    		if (del_ret< 0) {
                if (retry_del_idx<0) {
                    retry_del_idx = del_index;
                } else {
                    retry_del_counts++;
                }

                if (retry_del_counts > 2) {
                    retry_del_idx = -1;  // reset retry index
                    retry_del_counts = 0;  // reset retry counts
    			    avi_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
    			    //g_file_num--;
                    DBG_PRINT("Del Fail, avoid\r\n");
                } else {
                    DBG_PRINT("Del Fail, retry\r\n");
                }

    			ret = STATUS_FAIL;
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128);
    		} else {
    		    retry_del_idx = -1;  // reset retry index
    		    retry_del_counts = 0;  // reset retry counts
    		    ap_step_work_start=1;
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
    			avi_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
    			//g_file_num--;
    			DBG_PRINT("Step Del Init OK\r\n");

                if(g_avi_index_9999_exist) {
			        get_file_final_avi_index(0);
			        get_file_final_jpeg_index(0);
			        get_file_final_wav_index(0);
                }
		    }

            if (disk_free_size <= 70ULL) {
                unlink_step_flush();
                ap_step_work_start = 0;
                if ((vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20) <70ULL) {
                    DBG_PRINT ("Re-Delete\r\n");
                    goto RE_DEL;
                }
            }
            ret = STATUS_OK;
        }
		#endif
    } else {
        step_ret = unlink_step_work();
        if(step_ret != 0) {
            sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
            //DBG_PRINT ("StepDel Continue\r\n");
        } else {
            ap_step_work_start = 0;
            DBG_PRINT ("StepDel Done\r\n");
            disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
            DBG_PRINT("\r\n[Dominant (DskFree: %d MB)]\r\n", disk_free_size);
            if (disk_free_size > BkDelThreadMB) {
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DETECT_INTERVAL);
            } else {
                sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 128);
            }
        }
    }
	return ret;
}

#endif

void ap_storage_service_file_del(INT32U idx)
{
	INT32S ret, i;
	INT32U max_temp;
	struct stat_t buf_tmp;
	INT64U  disk_free_size;

	if (idx == 0xFFFFFFFF) {
#if C_AUTO_DEL_FILE == CUSTOM_ON	
		ret = ap_storage_service_freesize_check_and_del();
#endif		
	} else {
		//close(play_info.file_handle);

		stat(g_file_path, &buf_tmp);
		if(buf_tmp.st_mode & D_RDONLY){
			ret = 0x55;
			msgQSend(ApQ, MSG_APQ_SELECT_FILE_DEL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
			return;
		}
		if (unlink(g_file_path) < 0) {
			DBG_PRINT("Delete file fail.\r\n");
			ret = STATUS_FAIL;
		} else {
			DBG_PRINT("Delete file OK.\r\n");
			g_jpeg_index = 0;
            g_avi_index = 0;
            g_file_index = 0;
            g_file_num = 0;
            g_same_index_num = 0;
			g_wav_index = 0;
			//g_play_index = -1;	//Daniel marked for returning to previous one after deleting the current one
			for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
				avi_file_table[i] = 0;
				jpg_file_table[i] = 0;
				wav_file_table[i] = 0;
			}
			get_file_final_avi_index(1);
			get_file_final_jpeg_index(1);
			get_file_final_wav_index(1);

			if(g_avi_index_9999_exist)
			{
                if (g_avi_index == 10000) {
                    g_file_index = 0;
                } else if (g_jpeg_index == 10000) {
                    g_file_index = 0;
                } else if (g_wav_index == 10000) {
                    g_file_index = 0;
                } else {
					if(g_avi_file_time > g_jpg_file_time)
					{
                        max_temp = g_avi_index;
						if (g_avi_file_time > g_wav_file_time) {
                        	g_file_index = max_temp;
						}else{
                        	g_file_index = g_wav_index;
                    	}
					} else {
                        max_temp = g_jpeg_index;
						if (g_jpg_file_time > g_wav_file_time) {
                        	g_file_index = max_temp;
						}else{
                        	g_file_index = g_wav_index;
                    	}                        
					}
				}
			} else {
                if (g_avi_index > g_jpeg_index) {
                    max_temp = g_avi_index;
				} else {
                    max_temp = g_jpeg_index;
                }

                if (max_temp > g_wav_index) {
                    g_file_index = max_temp;
				} else {
                    g_file_index = g_wav_index;
				}
			}

			if(!g_avi_index_9999_exist) {
				g_oldest_avi_file_index = 0;
			}
			ret = STATUS_OK;
		}

		g_same_index_num = get_same_index_file_number();
		msgQSend(ApQ, MSG_APQ_SELECT_FILE_DEL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
	}

	disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
}

void ap_storage_service_file_delete_all(void)
{
	struct f_info file_info;
	//struct stat_t buf_tmp;
	INT8U	locked_files_exist = 0;
	INT32S nRet, i, ret;
	INT32U max_temp;
	INT64U  disk_free_size;

//check if there is any locked video file
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		locked_files_exist = 0;
	}
	else
	{
		while (1) 
		{
			//stat((CHAR *) file_info.f_name, &buf_tmp);
			//if(buf_tmp.st_mode & D_RDONLY) {		//skip if it's locked
			if(file_info.f_attrib & _A_RDONLY) {	//modified by wwj, stat() spent too long time
				locked_files_exist = 1;
				break;
			} 
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				locked_files_exist = 0;
				break;
			}
			continue;
		}
	}

    g_file_index = 0;
    g_file_num = 0;
    g_wav_index = 0;
	for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
		avi_file_table[i] = 0;
		jpg_file_table[i] = 0;
		wav_file_table[i] = 0;
	}
	g_play_index = -1;
	g_avi_index_9999_exist = g_avi_file_time = g_avi_file_oldest_time = 0;
//AVI	
    g_avi_index = -1;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		g_avi_index++;
	}
	else
	{
		g_avi_index = 0;
		while (1) 
		{
			//stat((CHAR *) file_info.f_name, &buf_tmp);
			//if(buf_tmp.st_mode & D_RDONLY) {		//skip if it's locked
			if(file_info.f_attrib & _A_RDONLY) {	//modified by wwj, stat() spent too long time
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}

			unlink((CHAR *) file_info.f_name);
			//if (unlink((CHAR *) file_info.f_name) == 0) 
			//{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				//continue;
			//}
		}
	}
//JPEG	
	g_jpeg_index = -1;
	g_jpg_file_time = 0;
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
	if (nRet < 0) {
		g_jpeg_index++;
	}
	else
	{
		g_jpeg_index = 0;
		while (1) 
		{
			unlink((CHAR *) file_info.f_name);
			//if (unlink((CHAR *) file_info.f_name)==0) 
			//{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				//continue;
			//}
		}
	}

//WAV
 	g_wav_index = -1;
	g_wav_file_time = 0;

	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if (nRet < 0) {
		g_wav_index++;
	}
	else
	{
		g_wav_index = 0;
		while (1) 
		{
			unlink((CHAR *) file_info.f_name);
			//if (unlink((CHAR *) file_info.f_name)==0) 
			//{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				//continue;
			//}
		}
	}
//Scan again if locked_files_exist
	if(locked_files_exist)
	{
		get_file_final_avi_index(1);
		get_file_final_jpeg_index(1);
		get_file_final_wav_index(1);
        if(g_avi_index_9999_exist)
        {
			if(g_avi_index == 10000) {
				g_file_index = 0;
            } else if (g_jpeg_index == 10000) {
                g_file_index = 0;
            } else if (g_wav_index == 10000) {
                g_file_index = 0;
            }
            else 
            {
				if(g_avi_file_time > g_jpg_file_time)
				{
					max_temp = g_avi_index;
					if (g_avi_file_time > g_wav_file_time) {
						g_file_index = max_temp;
					} else {
						g_file_index = g_wav_index;
					}
				}
				else
				{
                    max_temp = g_jpeg_index;
					if (g_jpg_file_time > g_wav_file_time) {
						g_file_index = max_temp;
					} else {
						g_file_index = g_wav_index;
					}                        
				}
			}
		}
        else
        {
			if (g_avi_index > g_jpeg_index) {
				max_temp = g_avi_index;
			} else {
				max_temp = g_jpeg_index;
			}

			if (max_temp>g_wav_index) {
				g_file_index = max_temp;
			} else {
				g_file_index = g_wav_index;
			}
		}	
	}	

	if(!g_avi_index_9999_exist) {
		g_oldest_avi_file_index = 0;
	}

	ret = STATUS_OK;
	g_same_index_num = 0;
	msgQSend(ApQ, MSG_APQ_FILE_DEL_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);

	disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
}

void ap_storage_service_file_lock_one(void)
{
	INT32S ret;
	if ( (avi_file_table[g_play_index >> 4] & (1 << (g_play_index & 0xF)))) {	//only lock video files
		_setfattr(g_file_path, D_RDONLY);
	}

	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_LOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);	
}

void ap_storage_service_file_lock_all(void)
{
	struct f_info file_info;
	INT32S nRet, ret;

	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		ret = STATUS_FAIL;
	}
	else
	{
		while (1) 
		{
			if (_setfattr((CHAR *) file_info.f_name, D_RDONLY) == 0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}

	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_LOCK_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_unlock_one(void)
{
	INT32S ret;
	if ( (avi_file_table[g_play_index >> 4] & (1 << (g_play_index & 0xF)))){	//only unlock video files
		_setfattr(g_file_path, D_NORMAL);
	}

	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_UNLOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_unlock_all(void)
{
	struct f_info file_info;
	INT32S nRet, ret;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		ret = STATUS_FAIL;
	}
	else
	{
		while (1) 
		{
			if (_setfattr((CHAR *) file_info.f_name, D_NORMAL) == 0) 
			{
				nRet = _findnext(&file_info);
				if (nRet < 0) {
					break;
				}
				continue;
			}
		}
	}

	ret = STATUS_OK;
	msgQSend(ApQ, MSG_APQ_FILE_UNLOCK_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_timer_start(void)
{
	if (storage_mount_timerid == 0xff) {
		storage_mount_timerid = STORAGE_SERVICE_MOUNT_TIMER_ID;
		sys_set_timer((void*)msgQSend, (void*)StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, storage_mount_timerid, STORAGE_TIME_INTERVAL_MOUNT);
	}
}

void ap_storage_service_timer_stop(void)
{
	if (storage_mount_timerid != 0xff) {
		sys_kill_timer(storage_mount_timerid);
		storage_mount_timerid = 0xff;
	}
}

void ap_storage_service_usb_plug_in(void)
{
	device_plug_phase = 0;
}

#ifdef SDC_DETECT_PIN
extern INT32S ap_peripheral_SDC_at_plug_OUT_detect();
extern INT32S ap_peripheral_SDC_at_plug_IN_detect();
#endif
INT8S Time_file_get(void)
{
  
}
INT32S ap_storage_service_storage_mount(void)
{
	INT32S nRet, i,led_type= -1;
	INT32U size;
	INT16S fd;

	if(storage_mount_timerid == 0xff) return	STATUS_FAIL;

	if (device_plug_phase == 0) {
		ap_storage_service_timer_stop();				//prohibit too many MSG_STORAGE_SERVICE_STORAGE_CHECK sent during sd mounting
#ifndef SDC_DETECT_PIN
        gOutOfFreeSpace = 0;
        gSelect_Size = 5*1024*1024/512;     // select 5M free size
		nRet = _devicemount(MINI_DVR_STORAGE_TYPE);
		if(FatSpeedUp_Array[MINI_DVR_STORAGE_TYPE].UsedCnt == 0)
		{   
		    gSelect_Size = 500*1024/512;    // select 500K free size
		    nRet = _devicemount(MINI_DVR_STORAGE_TYPE);
			if(FatSpeedUp_Array[MINI_DVR_STORAGE_TYPE].UsedCnt == 0)
				gOutOfFreeSpace = 1;
		}
#else
		nRet = ap_peripheral_SDC_at_plug_OUT_detect();
		DBG_PRINT("nRet= %d\r\n",nRet);
		if(!nRet){
			 _devicemount(MINI_DVR_STORAGE_TYPE);
		}
#endif
	} else {
#ifndef SDC_DETECT_PIN
		nRet = drvl2_sdc_live_response();
#else
		nRet = ap_peripheral_SDC_at_plug_IN_detect();
#endif
		if(nRet != 0) {
			ap_storage_service_timer_stop();				//prohibit too many MSG_STORAGE_SERVICE_STORAGE_CHECK sent during sd mounting
			nRet = _devicemount(MINI_DVR_STORAGE_TYPE);
		}
	}

	if (nRet < 0) {
		device_plug_phase = 0;  // plug out phase
		card_first_dete=2;

		if (curr_storage_id != NO_STORAGE) {
			///INT16S ret;

			// add by josephhsieh@140717 // 若螢幕保護開時，要點亮背光
			if(screen_saver_enable) {
				screen_saver_enable = 0;
					#if C_SCREEN_SAVER == CUSTOM_ON
	        		ap_state_handling_lcd_backlight_switch(1);
	        		#endif
			}
           #if (defined RAMDISK_EN) && (RAMDISK_EN == 1)
			_devicemount(RAMDISK_TYPE);
			ret = sformat(RAMDISK_TYPE, 32*1024*1024/512, C_RAMDISK_SIZE/512);
			if(ret != 0) {
				DBG_PRINT("RAM disk format fail...\r\n");
			}

			//mkdir("D:\\DCIM");
			//chdir("D:\\DCIM");
			mkdir("D:\\VIDEO");
			//mkdir("D:\\PHOTO");
			chdir("D:\\VIDEO");
	    #endif
			g_jpeg_index = g_avi_index = g_wav_index = g_file_index = 0;
			g_file_num = 0;
			for (i=0 ; i<625 ; i++) {
				avi_file_table[i] = jpg_file_table[i] = wav_file_table[i] = 0;
			}
			card_space_less_flag =0;

			curr_storage_id = NO_STORAGE;
			msgQSend(ApQ, MSG_STORAGE_SERVICE_NO_STORAGE, &curr_storage_id, sizeof(INT8U), MSG_PRI_NORMAL);
		} else {
			//chdir("D:\\DCIM");
			chdir("D:\\VIDEO");
		}

		ap_storage_service_timer_start();		//enable again
		return STATUS_FAIL;
	} else {
	  
	  if(card_first_dete)
				 {
				  card_space_less_flag =0;
				  if(card_first_dete ==1)
				  led_type=LED_CARD_DETE_SUC;
				  else
				  led_type=LED_WAITING_RECORD;	 
				  card_first_dete=0;
				  //msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
				 }
	  
        device_plug_phase = 1;  // plug in phase

		if (curr_storage_id != MINI_DVR_STORAGE_TYPE) {

			// add by josephhsieh@140717 // 若螢幕保護開時，要點亮背光
			if(screen_saver_enable) {
				screen_saver_enable = 0;
					#if C_SCREEN_SAVER == CUSTOM_ON
	        		ap_state_handling_lcd_backlight_switch(1);
	        		#endif
			}

			if(curr_storage_id == NO_STORAGE) {
		#if (defined RAMDISK_EN) && (RAMDISK_EN == 1)
				_deviceunmount(RAMDISK_TYPE);
		#endif
			}

			size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE)>>20;
			DBG_PRINT("Mount OK, free size [%d]\r\n", size);
			curr_storage_id = MINI_DVR_STORAGE_TYPE;

            if(size < CARD_FULL_MB_SIZE)
            	{
            	  led_type=LED_CARD_NO_SPACE;	
				  //card_space_less_flag=1;
				  //msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
            	}
			else
				card_space_less_flag =0;
		
		    //mkdir("C:\\DCIM");
			//chdir("C:\\DCIM");
			mkdir("C:\\VIDEO");
			//mkdir("C:\\PHOTO");
			g_jpeg_index = g_avi_index = g_wav_index = g_file_index = 0;
			g_file_num = 0;
			for (i=0 ; i<625 ; i++) {
				avi_file_table[i] = jpg_file_table[i] = wav_file_table[i] = 0;
			}
			chdir("C:\\VIDEO");
			get_file_final_avi_index(1);
			//chdir("C:\\PHOTO");
			get_file_final_jpeg_index(1);
			chdir("C:\\VIDEO");
			get_file_final_wav_index(1);
			if (g_avi_index > g_jpeg_index) {
				if (g_avi_index > g_wav_index) {
					g_file_index = g_avi_index;
				} else {
					g_file_index = g_wav_index;
				}
			} else {
				if (g_jpeg_index > g_wav_index) {
					g_file_index = g_jpeg_index;
				} else {
					g_file_index = g_wav_index;
				}
			}

			if(!g_avi_index_9999_exist) {
				g_oldest_avi_file_index = 0;
			}

			if (sd_upgrade_file_flag == 0) 
				{
                    INT8U  FileName_Path[3+5+8+4+1];				
					struct f_info	file_info;
					char   p[4];
					chdir("C:\\");
					//if(Time_file_get() == STATUS_OK)
					//{
					//  DBG_PRINT("TIME_SET0\r\n");
					//  msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_TIME_SET, NULL, NULL, MSG_PRI_NORMAL);
					//}
				
                     JH_Message_Get();
					 sprintf((char*)p,(const char*)"%04d",PRODUCT_NUM);
	                 gp_strcpy((INT8S*)FileName_Path,(INT8S*)"C:\\JH_");
	                 gp_strcat((INT8S*)FileName_Path,(INT8S*)p);
	                 gp_strcat((INT8S*)FileName_Path,(INT8S*)"*.bin");

	                 nRet = _findfirst((CHAR*)FileName_Path, &file_info ,D_ALL); //脤梑bin恅璃
	                 if (nRet < 0) 
	                  {
		              sd_upgrade_file_flag =1;
					  DBG_PRINT("No updatafile\r\n");
	                  }
					 else
					 	{
					  sd_upgrade_file_flag = 2;
					  DBG_PRINT("updatafile\r\n");
					  
					 	}

				
				 }
			#if C_AUTO_DEL_FILE == CUSTOM_ON
			ap_storage_service_del_thread_mb_set();
			#endif
			msgQSend(ApQ, MSG_STORAGE_SERVICE_MOUNT, &curr_storage_id, sizeof(INT8U), MSG_PRI_NORMAL);
		} else {
			//chdir("C:\\DCIM");
			chdir("C:\\VIDEO");
		}

		ap_storage_service_timer_start();		//enable again
		if(led_type != -1)
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
			
		return STATUS_OK;
	}
}

void ap_storage_service_file_open_handle(INT32U req_type)
{
	INT32U reply_type;
	STOR_SERV_FILEINFO file_info;
	ap_state_handling_file_creat_set(0);

	ap_storage_service_timer_stop();
    switch (req_type)
    {
    	case MSG_STORAGE_SERVICE_VID_REQ:

			reply_type = MSG_STORAGE_SERVICE_VID_REPLY;
			sprintf((char *)g_file_path, (const char *)"C:\\VIDEO\\MOVI%04d.avi", g_avi_index);
		    file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);
    		if (file_info.file_handle >= 0)
            {
                ap_state_handling_file_creat_set(1);
				file_info.file_path_addr = (INT32U) g_file_path;
				avi_file_table[g_avi_index >> 4] |= 1 << (g_avi_index & 0xF);
			//	g_file_num++;
				g_avi_index++;
				if(g_avi_index == 10000){
					g_avi_index = 0;
					g_avi_index_9999_exist = 1;
				}				
				//g_avi_index = g_file_index;
				DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);
			}
            break;

        case MSG_STORAGE_SERVICE_PIC_REQ:
			DBG_PRINT("jh_dbg6\r\n");
			reply_type = MSG_STORAGE_SERVICE_PIC_REPLY;

			if (curr_storage_id != NO_STORAGE) {
				#if ENABLE_SAVE_SENSOR_RAW_DATA
				sprintf((char *)g_file_path, (const char *)"C:\\PHOTO\\PICT%04d.dat", g_jpeg_index);
				#else
				sprintf((char *)g_file_path, (const char *)"C:\\PHOTO\\PICT%04d.jpg", g_jpeg_index);
				#endif
				file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);
				if (file_info.file_handle >= 0) {
				   ap_state_handling_file_creat_set(1);
					file_info.file_path_addr = (INT32U) g_file_path;
					if( (jpg_file_table[g_jpeg_index >> 4] & (1 << (g_jpeg_index & 0xF))) == 0){
						jpg_file_table[g_jpeg_index >> 4] |= 1 << (g_jpeg_index & 0xF);
						//g_file_num++;
					}
					g_jpeg_index++;
					if(g_jpeg_index == 10000){
						g_jpeg_index = 0;
					}				
					//g_jpeg_index = g_file_index;
					DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);
				}
			} else {
				g_file_index = 0;
				#if ENABLE_SAVE_SENSOR_RAW_DATA
				sprintf((char *)g_file_path, (const char *)"C:\\PHOTO\\PICT%04d.dat", g_jpeg_index);
				#else
				sprintf((char *)g_file_path, (const char *)"C:\\PHOTO\\PICT%04d.jpg", g_jpeg_index);
				#endif
				unlink(g_file_path);
				file_info.file_path_addr = (INT32U) g_file_path;
				file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);

				jpg_file_table[g_jpeg_index >> 4] |= 1 << (g_jpeg_index & 0xF);
				g_file_num = 1;

			}
            break;

    	case MSG_STORAGE_SERVICE_AUD_REQ:
    		#if 0
    		reply_type = MSG_STORAGE_SERVICE_AUD_REPLY;
       #if AUD_REC_FORMAT == AUD_FORMAT_WAV
	    	sprintf((char *)g_file_path, (const char *)"C:\\RECORD\\RECR%04d.wav", g_wav_index);//20110927
       #else
		    sprintf((char *)g_file_path, (const char *)"RECR%04d.mp3", g_wav_index);//20110927
       #endif
    		//sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_file_index);
    		file_info.file_handle = open (g_file_path, O_WRONLY|O_CREAT|O_TRUNC);
    		if (file_info.file_handle >= 0) {
				 ap_state_handling_file_creat_set(1);
    			file_info.file_path_addr = (INT32U) g_file_path;
    			if( (wav_file_table[g_wav_index >> 4] & (1 << (g_wav_index & 0xF))) == 0){
    				wav_file_table[g_wav_index >> 4] |= 1 << (g_wav_index & 0xF);
    				//g_file_num++;
    			}
    			g_wav_index++;
				if(g_wav_index == 10000){
					g_wav_index = 0;
				}    			
    			//g_wav_index = g_file_index;
    			DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);
			}
			#endif
            break;
        default:
            DBG_PRINT ("UNKNOW STORAGE SERVICE\r\n");
            break;
    }

	msgQSend(ApQ, reply_type, &file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
}

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
void ap_storage_service_cyclic_record_file_open_handle(void)
{
	INT32U reply_type;
	STOR_SERV_FILEINFO file_info;
	//CHAR f_name[24], f_name_b[24];
	//struct stat_t buf_tmp, buf_tmp_b;
	INT32S /*del_index,*/ i, ret;
	INT32U file_size_to_create;
	//INT8U read_only_flag;

	ap_storage_service_timer_stop();

	file_size_to_create = avi_file_size;
	file_size_to_create <<= 20;
	//storage_free_size = (vfsFreeSpace(curr_storage_id) >> 20);

	for(i=0; i<10000; i++) 
	{
		ret = UsrCheckFreeSize(MINI_DVR_STORAGE_TYPE, file_size_to_create);
		if(ret >= 2)	break;
	}

	UsrFlushBuffers(MINI_DVR_STORAGE_TYPE);

	reply_type = MSG_STORAGE_SERVICE_VID_CYCLIC_REPLY;
	if(ret < 2)
	{
		file_info.file_handle = -1;
	}
	else
	{
		sprintf((char *)g_file_path, (const char *)"C:\\VIDEO\\MOVI%04d.avi", g_avi_index);
		file_info.file_handle = CreatFileBySize(g_file_path, (avi_file_size<<20));
		if (file_info.file_handle >= 0)
		{
			file_info.file_path_addr = (INT32U) g_file_path;
			DBG_PRINT("FileName = %s\r\n", file_info.file_path_addr);

			if( (avi_file_table[g_avi_index >> 4] & (1 << (g_avi_index & 0xF))) == 0)
			{
				avi_file_table[g_avi_index >> 4] |= 1 << (g_avi_index & 0xF);
			}

			g_avi_index++;
			if(g_avi_index == 10000)
			{
				g_avi_index = 0;
			}					
		} 
	}

	msgQSend(ApQ, reply_type, &file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
}
#endif



void ap_storage_service_play_req(STOR_SERV_PLAYINFO *info_ptr, INT32U req_msg)
{
	INT16S index_tmp, i = 0, k, l;
	INT8U type = (req_msg & 0xFF), err_flag = ((req_msg & 0xFF00) >> 8);
	INT16U given_play_index = ((req_msg>>16) & 0xFFFF);
	struct stat_t buf_tmp;

	ap_storage_service_timer_stop();

	info_ptr->search_type = type;
	if (type == STOR_SERV_SEARCH_INIT) {
		g_play_index = -1;
	}
	if (g_play_index < 0) {
		get_file_final_avi_index(0);
		get_file_final_jpeg_index(0);
		get_file_final_wav_index(0);

		if(!g_avi_index_9999_exist) {
			g_oldest_avi_file_index = 0;
		}

		if(g_avi_index_9999_exist){
			if(g_avi_file_time > g_jpg_file_time)
			{
				if(g_avi_file_time > g_wav_file_time)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
					g_play_index = g_avi_index - 1;
					sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
				} else {
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);				
				}
			}
			else
			{
				if(g_jpg_file_time > g_wav_file_time)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
					g_play_index = g_jpeg_index - 1;
					sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
				}else{
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
				
				}
			}
		} else {
			if (g_jpeg_index > g_avi_index) 
			{
				if(g_jpeg_index > g_wav_index)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
					g_play_index = g_jpeg_index - 1;
					sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
				}else{
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);				
				}
			} else if (g_jpeg_index < g_avi_index) {
				if(g_avi_index > g_wav_index)
				{
					info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
					g_play_index = g_avi_index - 1;
					sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
				}else{
					info_ptr->file_type = TK_IMAGE_TYPE_WAV;
					g_play_index = g_wav_index - 1;
					sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);				
				}
			} else if(g_wav_index) {
				info_ptr->file_type = TK_IMAGE_TYPE_WAV;
				g_play_index = g_wav_index - 1;
				sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);			
			} else {
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		}
	} else {
		if (type == STOR_SERV_SEARCH_PREV) {
			INT8U flag = 0; //wwj add

			index_tmp = g_play_index - 1;
			if (index_tmp < 0) {
				index_tmp = 9999;
			}
			k = index_tmp >> 4;
			l = index_tmp & 0xF;
			while (i <= 626) {
				i++;
				if (avi_file_table[k] || jpg_file_table[k] || wav_file_table[k]) {
					for ( ; l>=0 ; l--) {
						if (avi_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
							sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
							flag = 1;  //wwj add
							break;
						}
						if (jpg_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
							sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
							flag = 1;  //wwj add
							break;
						}
						if (wav_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_WAV;
							sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
							flag = 1;  //wwj add
							break;
						}						
					}
					//l = 0xF; //wwj mark
					//if (index_tmp != g_play_index - 1) {
					if(flag) {
						break;
					}
				}
				l = 0xF; //wwj add
				k--;
				if (k < 0) {
					k = 624;
				}
			}
			if (i > 626) {
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				g_err_cnt = 0;
				msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		} else if (type == STOR_SERV_SEARCH_NEXT) {
			INT8U flag = 0; //wwj add

			index_tmp = g_play_index + 1;
			if (index_tmp > 9999) {
				index_tmp = 0;
			}
			k = index_tmp >> 4;
			l = index_tmp & 0xF;
			while (i <= 626) {
				i++;
				if (avi_file_table[k] || jpg_file_table[k] || wav_file_table[k]) {
					for ( ; l<0x10 ; l++) {
						if (avi_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
							sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
							flag = 1; //wwj add
							break;
						}
						if (jpg_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
							sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
							flag = 1; //wwj add
							break;
						}
						if (wav_file_table[k] & (1<<l)) {
							g_play_index = (k << 4) + l;
							info_ptr->file_type = TK_IMAGE_TYPE_WAV;
							sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
							flag = 1; //wwj add
							break;
						}						
					}
					//l = 0x0; //wwj mark
					//if (index_tmp != g_play_index + 1) {
					if(flag) {
						break;
					}
				}
				l = 0x0; //wwj add
				k++;
				if (k > 624) {
					k = 0;
				}
			}
			if (i > 626) {
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				g_err_cnt = 0;
				msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		} else {
			if (type == STOR_SERV_SEARCH_GIVEN) {
				g_play_index = given_play_index;
			}
			k = g_play_index >> 4;
			l = g_play_index & 0xF;
			if (avi_file_table[k] & (1<<l)) {
				info_ptr->file_type = TK_IMAGE_TYPE_MOTION_JPEG;
				sprintf((char *)g_file_path, (const char *)"MOVI%04d.avi", g_play_index);
			} else if (jpg_file_table[k] & (1<<l)) {
				info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
				sprintf((char *)g_file_path, (const char *)"PICT%04d.jpg", g_play_index);
			} else if (wav_file_table[k] & (1<<l)) {
				info_ptr->file_type = TK_IMAGE_TYPE_WAV;
				sprintf((char *)g_file_path, (const char *)"RECR%04d.wav", g_play_index);
			}
		}
	}
DBG_PRINT("File Path=%s\r\n", g_file_path);	
	info_ptr->file_path_addr = (INT32U) g_file_path;
	info_ptr->file_handle = open(g_file_path, O_RDONLY);
	if (info_ptr->file_handle >= 0) {
		stat(g_file_path, &buf_tmp);
		info_ptr->file_size = buf_tmp.st_size;
		if (!err_flag) {
			info_ptr->err_flag = STOR_SERV_OPEN_OK;
			g_err_cnt = 0;
		} else {
			g_err_cnt++;
			if (g_err_cnt <= g_file_num) {
				info_ptr->err_flag = STOR_SERV_OPEN_OK;
			} else {
				info_ptr->err_flag = STOR_SERV_DECODE_ALL_FAIL;
			}
		}
	} else {
		info_ptr->err_flag = STOR_SERV_OPEN_FAIL;
		g_err_cnt = 0;
	}
	info_ptr->deleted_file_number = get_deleted_file_number();
	info_ptr->play_index = g_play_index;
	info_ptr->total_file_number = g_file_num - g_same_index_num;
	msgQSend(ApQ, MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
}

INT32S get_file_final_avi_index(INT8U count_total_num_enable)
{
	CHAR  *pdata;
	INT32S nRet, temp;
	INT32U temp_time;
	struct f_info file_info;	
	//struct stat_t buf_tmp;
	
	g_avi_index = -1;
	g_avi_index_9999_exist = g_avi_file_time = g_avi_file_oldest_time = 0;
	nRet = _findfirst("*.avi", &file_info, D_ALL);
	if (nRet < 0) {
		g_avi_index++;
		return g_avi_index;
	}
	while (1) {	
		pdata = (CHAR *) file_info.f_name;
		// Remove 0KB AVI files
		if (file_info.f_size<512 && unlink((CHAR *) file_info.f_name)==0) {
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				break;
			}
			continue;
		}
        //stat((CHAR *) file_info.f_name, &buf_tmp);	//deleted by wwj, it spent too long time
		if (gp_strncmp((INT8S *) pdata, (INT8S *) "MOVI", 4) == 0) {
			temp = (*(pdata + 4) - 0x30)*1000;
			temp += (*(pdata + 5) - 0x30)*100;
			temp += (*(pdata + 6) - 0x30)*10;
			temp += (*(pdata + 7) - 0x30);
			if (temp < 10000) {
				avi_file_table[temp >> 4] |= 1 << (temp & 0xF);
				if(count_total_num_enable){
					g_file_num++;
				}
				if (temp > g_avi_index) {
					g_avi_index = temp;
				}
				temp_time = (file_info.f_date<<16)|file_info.f_time;
				if( ((!g_avi_file_time) || (temp_time > g_avi_file_time)) && /*((buf_tmp.st_mode & D_RDONLY) == 0)*/((file_info.f_attrib & _A_RDONLY) == 0) ){
					g_avi_file_time = temp_time;
					g_latest_avi_file_index = temp;
				}
				if( ((!g_avi_file_oldest_time) || (temp_time < g_avi_file_oldest_time)) && /*((buf_tmp.st_mode & D_RDONLY) == 0)*/((file_info.f_attrib & _A_RDONLY) == 0) ){
					g_avi_file_oldest_time = temp_time;
					g_oldest_avi_file_index = temp;
				}
			}
		}
		nRet = _findnext(&file_info);
		if (nRet < 0) {
			break;	
		}
	}
	g_avi_index++;
	if(g_avi_index == 10000){
		g_avi_index_9999_exist = 1;
		g_avi_index = g_latest_avi_file_index+1;
	}
	
	return g_avi_index;
}

INT32S get_file_final_jpeg_index(INT8U count_total_num_enable)
{
	CHAR  *pdata;
	struct f_info   file_info;
	//INT16S latest_file_index;
	INT32S nRet, temp;
	//INT32U temp_time;
	
	g_jpeg_index = -1;
	g_jpg_file_time = 0;

	#if ENABLE_SAVE_SENSOR_RAW_DATA		
	nRet = _findfirst("*.dat", &file_info, D_ALL);
	#else
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
	#endif
	if (nRet < 0) {
		g_jpeg_index++;
		return g_jpeg_index;
		
	}	
	while (1) {	
		pdata = (CHAR*)file_info.f_name;
		// Remove 0KB JPG files
		if (file_info.f_size<256 && unlink((CHAR *) file_info.f_name)==0) {
			nRet = _findnext(&file_info);
			if (nRet < 0) {
				break;
			}
			continue;
		}
		
		if (gp_strncmp((INT8S *) pdata, (INT8S *) "PICT", 4) == 0) {
			temp = (*(pdata + 4) - 0x30)*1000;
			temp += (*(pdata + 5) - 0x30)*100;
			temp += (*(pdata + 6) - 0x30)*10;
			temp += (*(pdata + 7) - 0x30);
			if (temp < 10000) {
				jpg_file_table[temp >> 4] |= 1 << (temp & 0xF);
				if(count_total_num_enable){
					g_file_num++;
				}
				if (temp > g_jpeg_index) {
					g_jpeg_index = temp;
				}
				
				/*temp_time = (file_info.f_date<<16)|file_info.f_time;
				if( (!g_jpg_file_time) || (temp_time > g_jpg_file_time) ){
					g_jpg_file_time = temp_time;
					latest_file_index = temp;
				}*/			
			}
		}		
		nRet = _findnext(&file_info);
		if (nRet < 0) {
			break;
		}
	}
	g_jpeg_index++;
	/*if( (g_jpeg_index == 10000) || (g_avi_index_9999_exist == 1) ){
		g_avi_index_9999_exist = 1;
		g_jpeg_index = latest_file_index+1;
		g_avi_index = g_latest_avi_file_index+1;
	}*/
	if (g_jpeg_index > 9999 || g_jpeg_index < 0) {
		g_jpeg_index = 0;
	}
	
	return g_jpeg_index;
}

INT32S get_file_final_wav_index(INT8U count_total_num_enable)
{
	CHAR  *pdata;
	struct f_info   file_info;
    //INT16S latest_file_index;
	INT32S nRet, temp;
    //INT32U temp_time;

	g_wav_index = -1;
	g_wav_file_time = 0;

	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if (nRet < 0) {
		g_wav_index++;
		return g_wav_index;

	}
	while (1) {
		pdata = (CHAR*)file_info.f_name;
		if (gp_strncmp((INT8S *) pdata, (INT8S *) "RECR", 4) == 0) {
			temp = (*(pdata + 4) - 0x30)*1000;
			temp += (*(pdata + 5) - 0x30)*100;
			temp += (*(pdata + 6) - 0x30)*10;
			temp += (*(pdata + 7) - 0x30);

			if (temp < 10000)
            {
				wav_file_table[temp >> 4] |= 1 << (temp & 0xF);
				if(count_total_num_enable){
					g_file_num++;
				}

    			if (temp > g_wav_index) {
    				g_wav_index = temp;
    			}

                /*temp_time = (file_info.f_date<<16)|file_info.f_time;

                if( (!g_wav_file_time) || (temp_time > g_wav_file_time) ){
					g_wav_file_time = temp_time;
					latest_file_index = temp;
				}*/
			}
		}

		nRet = _findnext(&file_info);
		if (nRet < 0) {
			break;
		}
	}
	g_wav_index++;
	/*if( (g_wav_index == 10000) || (g_avi_index_9999_exist == 1) ){
		g_avi_index_9999_exist = 1;
		g_wav_index = latest_file_index+1;
		g_avi_index = g_latest_avi_file_index+1;
	}*/
	if (g_wav_index > 9999 || g_wav_index < 0) {
		g_wav_index = 0;
	}

	return g_wav_index;	
}

INT16U get_deleted_file_number(void)
{
	INT16U i;
	INT16U deleted_file_number = 0;
	
	for(i=0; i<g_play_index+1; i++)
	{
        if( ((avi_file_table[(g_play_index-i)>>4] & (1<<((g_play_index-i)&0xF))) == 0) && 
          ((jpg_file_table[(g_play_index-i)>>4] & (1<<((g_play_index-i)&0xF))) == 0) &&
          ((wav_file_table[(g_play_index-i)>>4] & (1<<((g_play_index-i)&0xF))) == 0))
		{
			deleted_file_number++;
		}
	}
	return deleted_file_number;
}


INT16U get_same_index_file_number(void)
{
	INT16U i, j;
	INT16U same_index_file_number = 0;
	
	for(i=0; i<AP_STG_MAX_FILE_NUMS; i++)
	{
		for(j=0; j<16; j++)
		{
			if( (avi_file_table[i] & (1<<j)) && (jpg_file_table[i] & (1<<j)) && (wav_file_table[i] & (1<<j)) ){
				same_index_file_number++;
			}
		}
	}
	return same_index_file_number;
}

void ap_storage_service_format_req(void)
{
	INT32U i;
    INT32U max_temp;

	if(drvl2_sd_sector_number_get()<=2097152) {  // Dominant  1GB card should use FAT16
		_format(MINI_DVR_STORAGE_TYPE, FAT16_Type);
	} else {
		_format(MINI_DVR_STORAGE_TYPE, FAT32_Type);
	}

	mkdir("C:\\DCIM");
	chdir("C:\\DCIM");
	g_jpeg_index = 0;
    g_avi_index = 0;
    g_file_index = 0;
    g_file_num = 0;
    g_wav_index = 0;
	g_play_index = -1;
	for (i=0 ; i<AP_STG_MAX_FILE_NUMS ; i++) {
		avi_file_table[i] = 0;
		jpg_file_table[i] = 0;
        wav_file_table[i] = 0;
	}
	get_file_final_avi_index(1);
	get_file_final_jpeg_index(1);
	get_file_final_wav_index(1);

	if(g_avi_index_9999_exist) {
		if(g_avi_index == 10000) {
			g_file_index = 0;
        } else if (g_jpeg_index == 10000) {
            g_file_index = 0;
        } else if (g_wav_index == 10000) {
            g_file_index = 0;
        } else {
			if(g_avi_file_time > g_jpg_file_time) {
                max_temp = g_avi_index;
				if (g_avi_file_time > g_wav_file_time) {
                	g_file_index = max_temp;
				} else {
                	g_file_index = g_wav_index;
            	}
			} else {
                max_temp = g_jpeg_index;
				if (g_jpg_file_time > g_wav_file_time) {
                	g_file_index = max_temp;
				} else {
                	g_file_index = g_wav_index;
            	}                        
			}
		}
	} else {
	    if (g_avi_index > g_jpeg_index) {
	        max_temp = g_avi_index;
		} else {
	        max_temp = g_jpeg_index;
	    }

	    if (max_temp > g_wav_index) {
	        g_file_index = max_temp;
		}else{
			g_file_index = g_wav_index;
		}
	}
	FNodeInfo[SD_SLOT_ID].audio.MaxFileNum = 0;
	msgQSend(ApQ, MSG_STORAGE_SERVICE_FORMAT_REPLY, NULL, NULL, MSG_PRI_NORMAL);
}


void FileSrvRead(P_TK_FILE_SERVICE_STRUCT para)
{
	INT32S read_cnt;
    
    if (para->rev_seek) {
    	if(para->FB_seek)
    	{
	        lseek(para->fd, -para->rev_seek, SEEK_CUR);
	        DBG_PRINT("reverse seek- %d\r\n",para->rev_seek);
        }
        else
        {
 	        lseek(para->fd, para->rev_seek, SEEK_CUR);
	        DBG_PRINT("reverse seek+ %d\r\n",para->rev_seek);
       }
    }
	read_cnt = read(para->fd, para->buf_addr, para->buf_size);
	if(para->result_queue)
	{
		OSQPost(para->result_queue, (void *) read_cnt);
	}
}

