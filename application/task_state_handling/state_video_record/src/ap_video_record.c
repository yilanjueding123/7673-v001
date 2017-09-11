#include "ap_video_record.h"
#include "ap_state_config.h"
#include "ap_state_handling.h"
#include "avi_encoder_app.h"
#include "application.h"
#include "ap_display.h"

STOR_SERV_FILEINFO curr_file_info;
INT8S video_record_sts;
static STR_ICON_EXT resolution_str;
static STR_ICON_EXT park_mode_str;
extern INT32U threshold_size;
extern INT32U avi_file_size;

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	static INT8U cyclic_record_timerid;
	STOR_SERV_FILEINFO next_file_info;
#endif
#if C_MOTION_DETECTION == CUSTOM_ON
	static INT8U motion_detect_timerid = 0xFF;
//prototype
	void ap_video_record_md_icon_clear_all(void);
	void ap_video_record_md_tick(INT8U *md_tick);
	INT8S ap_video_record_md_active(INT8U *md_tick);
	void ap_video_record_md_disable(void);
	void ap_video_record_md_icon_update(INT8U sts);
#endif

extern void led_red_on(void);
extern void led_green_off(void);
//prototype
//void video_calculate_left_recording_time_enable(void);
//void video_calculate_left_recording_time_disable(void);
extern void led_green_on(void);
void ap_video_err_handle(void)
{
	INT32U led_type;
	
	led_type = LED_CARD_NO_SPACE;//LED_SDC_FULL;
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
}

extern void date_time_force_display(INT8U force_en,INT8U postion_flag);
void ap_video_record_show_park_mode(void)
{
	STRING_ASCII_INFO ascii_str;
	ascii_str.font_color = BLUE_COLOR;
	ascii_str.font_type = 0;
	ascii_str.pos_x = 0;
	ascii_str.pos_y = 0;
	ascii_str.str_ptr = "P";
	ascii_str.buff_h = park_mode_str.h = ASCII_draw_char_height;
	ascii_str.buff_w = park_mode_str.w = ASCII_draw_char_width*1;

	if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
		park_mode_str.pos_x = 32*6;
		park_mode_str.pos_y = 2;
	} else { //TV
		park_mode_str.pos_x = 450;
		park_mode_str.pos_y = 30;
	}
	//ap_state_handling_ASCII_str_draw(&park_mode_str,&ascii_str);
}

void ap_video_record_clear_park_mode(void)
{
	//ap_state_handling_ASCII_str_draw_exit(&park_mode_str,1);
}

void ap_video_record_resolution_display(void)
{
#if 0
	STRING_ASCII_INFO ascii_str;

	ascii_str.font_color = 0xffff;
	ascii_str.font_type = 0;
	ascii_str.pos_x = 0;
	ascii_str.pos_y = 0;

	if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
		resolution_str.pos_y = 35;
	} else { //TV
		resolution_str.pos_y = 80;
	}
	ascii_str.buff_h = resolution_str.h = ASCII_draw_char_height;
	if(ap_state_config_video_resolution_get() == 0) {
		ascii_str.str_ptr = "1080FHD";
		ascii_str.buff_w = resolution_str.w = ASCII_draw_char_width*7;
		if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
			resolution_str.pos_x = TFT_WIDTH-5-ASCII_draw_char_width*7;
		} else { //TV
		  #if TV_WIDTH == 720
			resolution_str.pos_x = TV_WIDTH-40-ASCII_draw_char_width*7;
  		  #elif TV_WIDTH == 640
			resolution_str.pos_x = TV_WIDTH-20-ASCII_draw_char_width*7;
  		  #endif
		}
	} else if(ap_state_config_video_resolution_get() == 1) {
		ascii_str.str_ptr = "1080P";
		ascii_str.buff_w = resolution_str.w = ASCII_draw_char_width*5;
		if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
			resolution_str.pos_x = TFT_WIDTH-5-ASCII_draw_char_width*5;
		} else { //TV
		  #if TV_WIDTH == 720
			resolution_str.pos_x = TV_WIDTH-40-ASCII_draw_char_width*5;
  		  #elif TV_WIDTH == 640
			resolution_str.pos_x = TV_WIDTH-20-ASCII_draw_char_width*5;
  		  #endif
		}
	} else if(ap_state_config_video_resolution_get() == 2) {
		ascii_str.str_ptr = "720P";
		ascii_str.buff_w = resolution_str.w = ASCII_draw_char_width*4;
		if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
			resolution_str.pos_x = TFT_WIDTH-5-ASCII_draw_char_width*4;
		} else { //TV
		  #if TV_WIDTH == 720
			resolution_str.pos_x = TV_WIDTH-40-ASCII_draw_char_width*4;
  		  #elif TV_WIDTH == 640
			resolution_str.pos_x = TV_WIDTH-20-ASCII_draw_char_width*4;
  		  #endif
		}
	} else if(ap_state_config_video_resolution_get() == 3) {
		ascii_str.str_ptr = "WVGA";
		ascii_str.buff_w = resolution_str.w = ASCII_draw_char_width*4;
		if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
			resolution_str.pos_x = TFT_WIDTH-5-ASCII_draw_char_width*4;
		} else { //TV
		  #if TV_WIDTH == 720
			resolution_str.pos_x = TV_WIDTH-40-ASCII_draw_char_width*4;
  		  #elif TV_WIDTH == 640
			resolution_str.pos_x = TV_WIDTH-20-ASCII_draw_char_width*4;
  		  #endif
		}
	} else if(ap_state_config_video_resolution_get() == 4) {
		ascii_str.str_ptr = "VGA";
		ascii_str.buff_w = resolution_str.w = ASCII_draw_char_width*3;
		if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
			resolution_str.pos_x = TFT_WIDTH-5-ASCII_draw_char_width*3;
		} else { //TV
		  #if TV_WIDTH == 720
			resolution_str.pos_x = TV_WIDTH-40-ASCII_draw_char_width*3;
  		  #elif TV_WIDTH == 640
			resolution_str.pos_x = TV_WIDTH-20-ASCII_draw_char_width*3;
  		  #endif
		}
	} else {
		ascii_str.str_ptr = "QVGA";
		ascii_str.buff_w = resolution_str.w = ASCII_draw_char_width*4;
		if(ap_display_get_device()==DISP_DEV_TFT) { //TFT
			resolution_str.pos_x = TFT_WIDTH-5-ASCII_draw_char_width*4;
		} else { //TV
		  #if TV_WIDTH == 720
			resolution_str.pos_x = TV_WIDTH-40-ASCII_draw_char_width*4;
  		  #elif TV_WIDTH == 640
			resolution_str.pos_x = TV_WIDTH-20-ASCII_draw_char_width*4;
  		  #endif
		}
	}
	//ap_state_handling_ASCII_str_draw(&resolution_str,&ascii_str);
	#endif
}

void ap_video_record_clear_resolution_str(void)
{
	//ap_state_handling_ASCII_str_draw_exit(&resolution_str,1);
}

INT8S ap_video_record_init(INT32U state)
{
	//INT8U night_mode;
	INT8U temp;

	#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
	OSQPost(DisplayTaskQ, (void *) MSG_DISPLAY_TASK_EFFECT_INIT);
	if(ap_state_config_video_resolution_get() <= 2) {
		date_time_force_display(1, DISPLAY_DATE_TIME_RECORD);
	} else {
		date_time_force_display(1, DISPLAY_DATE_TIME_RECORD2);
	}

	if(ap_state_config_voice_record_switch_get() != 0) {
		//ap_state_handling_icon_clear_cmd(ICON_MIC_OFF, NULL, NULL);
	} else {
		//ap_state_handling_icon_show_cmd(ICON_MIC_OFF, NULL, NULL);
	}

	ap_video_record_resolution_display();

	temp = ap_state_config_record_time_get();
	if(temp) {
		//ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD_CYC_1MINUTE+temp-1, NULL, NULL);
	}
	
	temp = ap_state_config_park_mode_G_sensor_get();
	if(temp) {
		ap_video_record_show_park_mode();
	}
	#endif
		
	temp = ap_state_config_ev_get();
	//ap_state_handling_icon_show_cmd(ICON_VIDEO_EV6+temp, NULL, NULL);
	gp_cdsp_set_ev_val(temp);	//0:+2, 1:+5/3, 2:+4/3, 3:+1.0, 4:+2/3, 5:+1/3, 6:+0.0, 7:-1/3, 8:-2/3, 9:-1.0, 10:-4/3, 11:-5/3, 12:-2.0 

#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
	//ap_state_handling_current_bat_lvl_show();
	//ap_state_handling_current_charge_icon_show();
#endif

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	cyclic_record_timerid = 0xFF;
	next_file_info.file_handle = -1;
#endif

/*	if(ap_state_handling_night_mode_get()) {
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
		night_mode = 1;
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_NIGHT_MODE_SET, &night_mode, sizeof(INT8U), MSG_PRI_NORMAL);
	} else {
		ap_state_handling_icon_show_cmd(ICON_NIGHT_MODE_DISABLED, NULL, NULL);
		ap_state_handling_icon_clear_cmd(ICON_NIGHT_MODE_ENABLED, NULL, NULL);
		night_mode = 0;
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_NIGHT_MODE_SET, &night_mode, sizeof(INT8U), MSG_PRI_NORMAL);
	}
*/

	#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
	//ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
	#endif

	ap_video_record_sts_set(0);
	if (ap_state_handling_storage_id_get() == NO_STORAGE) {
		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
		//ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD, ICON_INTERNAL_MEMORY, NULL);
		#endif
		ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);
		//video_calculate_left_recording_time_disable();
#if C_MOTION_DETECTION == CUSTOM_ON
		if (ap_state_config_md_get()) {
			//ap_video_record_md_icon_clear_all();
			ap_video_record_md_disable();
			ap_state_config_md_set(0);
		}
#endif
		return STATUS_FAIL;
	} else {
		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
		//ap_state_handling_icon_show_cmd(ICON_VIDEO_RECORD, ICON_SD_CARD, NULL);
		#endif
		ap_video_record_sts_set(~VIDEO_RECORD_UNMOUNT);
		//video_calculate_left_recording_time_enable();
#if C_MOTION_DETECTION == CUSTOM_ON
		if (ap_state_config_md_get()) {
			//ap_state_handling_icon_show_cmd(ICON_MD_STS_0, NULL, NULL);
			#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
			//ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT, NULL, NULL);
			#endif
			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
			ap_video_record_sts_set(VIDEO_RECORD_MD);
		}
#endif
		return STATUS_OK;
	}
}
void ap_video_record_error_handle(void)
{
	if (video_record_sts & VIDEO_RECORD_BUSY) {
		ap_video_record_func_key_active();
	}
}

void ap_video_record_exit(void)
{
	ap_video_record_clear_resolution_str();
	ap_video_record_clear_park_mode();

	date_time_force_display(0, DISPLAY_DATE_TIME_RECORD);

}

void ap_video_record_sts_set(INT8S sts)
{
	if (sts > 0) {
		video_record_sts |= sts;
	} else {
		video_record_sts &= sts;
	}
}

#if C_MOTION_DETECTION == CUSTOM_ON
void ap_video_record_md_icon_clear_all(void)
{
  #if 0
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | (ICON_MD_STS_0 | (ICON_MD_STS_1<<8) | (ICON_MD_STS_2<<16))));
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_ICON_CLEAR | (ICON_MD_STS_3 | (ICON_MD_STS_4<<8) | (ICON_MD_STS_5<<16))));
  #else
   // ap_state_handling_icon_clear_cmd(ICON_MOTION_DETECT, NULL, NULL);
  #endif
}
void ap_video_record_md_tick(INT8U *md_tick)
{
	++*md_tick;
	if (*md_tick > MD_STOP_TIME) {
		*md_tick = 0;
		if (motion_detect_timerid != 0xFF) {
			sys_kill_timer(motion_detect_timerid);
			motion_detect_timerid = 0xFF;
		}
		if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
			ap_video_record_error_handle();
	//		ap_state_handling_icon_clear_cmd(ICON_MOTION_DETECT_START, NULL, NULL);
	        DBG_PRINT("Motion detect md_tick Show icon!\r\n");
			//ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT, NULL, NULL);
		}
	}
}

INT8S ap_video_record_md_active(INT8U *md_tick)
{
	*md_tick = 0;
	if (video_record_sts & VIDEO_RECORD_UNMOUNT) {
		return STATUS_FAIL;
	}
    if ((ap_state_config_record_time_get()==0) && (vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20) < CARD_FULL_SIZE_RECORD) 
    {
        DBG_PRINT ("Card full, MD Auto Disable2!!!\r\n");
       // ap_video_record_md_icon_clear_all();
    	ap_video_record_md_disable();
    	ap_state_config_md_set(0); 
       ap_video_record_error_handle();  // super stop MD record
        return	STATUS_FAIL;
    }
	if (!(video_record_sts & VIDEO_RECORD_BUSY)) {
		if (ap_video_record_func_key_active()) {
			return STATUS_FAIL;
		} else {
			if (motion_detect_timerid == 0xFF) {
				motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
				sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
			}
		}
	}
	return STATUS_OK;
}

void ap_video_record_md_disable(void)
{
	if (motion_detect_timerid != 0xFF) {
		sys_kill_timer(motion_detect_timerid);
		motion_detect_timerid = 0xFF;
	}
	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP, NULL, NULL, MSG_PRI_NORMAL);
	ap_video_record_sts_set(~VIDEO_RECORD_MD);
}

void ap_video_record_md_icon_update(INT8U sts)
{
#if 0
	if (ap_state_config_md_get()) {
		OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_MD_ICON_SHOW | (ICON_MD_STS_0 + sts)));
	}
#endif
}

#endif
INT8U video_stop_flag=0;
extern INT8U cyc_restart_flag;
volatile INT8U video_down_flag=0;
//extern INT8U  File_creat_ok;

INT8S ap_video_record_func_key_active(void)
{
	INT32U led_type;
	INT8U type = FALSE;
	video_down_flag=1;
	
	if (video_record_sts & VIDEO_RECORD_BUSY) 
	{
            video_stop_flag=1;
            if(ap_state_handling_file_creat_get())
			{
		     led_green_on();
		    // DBG_PRINT("red_led\r\n");
			}

	  #if C_AUTO_DEL_FILE == CUSTOM_ON
		
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT8U), MSG_PRI_NORMAL);
	  #endif
		timer_counter_force_display(0); // dominant add,function key event, must disable timer OSD 
		//ap_state_handling_led_on();

	  #if 1//C_SCREEN_SAVER == CUSTOM_ON
	  	if ((video_record_sts & VIDEO_RECORD_MD) == 0) 
		ap_peripheral_auto_off_force_disable_set(0);
	  #endif

		//video_calculate_left_recording_time_enable();
		if (video_encode_stop() != START_OK) 
		{
			close(curr_file_info.file_handle);
			curr_file_info.file_handle = -1;
			DBG_PRINT("Video Record Stop Fail\r\n");
		} 
		//UsrFlushBuffers(MINI_DVR_STORAGE_TYPE);

	  #if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
		if (cyclic_record_timerid != 0xFF) 
		{
			sys_kill_timer(cyclic_record_timerid);
			cyclic_record_timerid = 0xFF;
		}
	  #endif
		

		if(ap_state_handling_storage_id_get()!= NO_STORAGE)
			{			
            if(!cyc_restart_flag && ap_state_handling_file_creat_get())
             {
            // if(event != MSG_APQ_VDO_REC_STOP)
             	{
             	   led_green_off();
				   OSTimeDly(7);
				   led_green_on();
				   OSTimeDly(7);
				   led_green_off();
				   OSTimeDly(7);
				   led_green_on();
				   OSTimeDly(7);
				   led_green_off();
				   OSTimeDly(7);
				   led_green_on();
				   OSTimeDly(7);
				   
		           led_type = LED_WAITING_RECORD;
		           msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
		           DBG_PRINT("------video_wait-----\r\n");
             	}
             }
			}
		else
			{
			  led_type = LED_NO_SDC;
		      msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
			}
		ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		ap_state_handling_file_creat_set(0);
		DBG_PRINT("Video Record Stop\r\n");
		video_stop_flag=0;
		video_down_flag=0;
		return STATUS_OK;
	} 
	else 
	{
		if ((video_record_sts & VIDEO_RECORD_UNMOUNT) == 0) 
		{
		  #if 0//C_MOTION_DETECTION == CUSTOM_ON
			if (video_record_sts == 0 || video_record_sts == VIDEO_RECORD_MD) 
			{
			INT64U  disk_free_size;
				ap_video_record_sts_set(VIDEO_RECORD_BUSY + VIDEO_RECORD_WAIT);
#if 0
				disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
				DBG_PRINT("\r\n[Disk free %dMB]\r\n", disk_free_size);
				if (disk_free_size < CARD_FULL_SIZE_RECORD) 
				{
					//if (ap_state_config_record_time_get() == 0) 
					{
						//ap_state_handling_str_draw_exit();
						//ap_state_handling_str_draw(STR_SD_FULL, WARNING_STR_COLOR);
						//video_calculate_left_recording_time_disable();
						led_type = LED_SDC_FULL;
						msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
					}
					//else 
					//{
					//	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREE_FILESIZE_CHECK, NULL, NULL, MSG_PRI_NORMAL);
					//}
                    video_down_flag=0;
					return STATUS_OK;
				}

				ap_video_record_sts_set(VIDEO_RECORD_BUSY);
				#endif
			//	led_type = LED_RECORD;
			//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
			  #if 1//C_SCREEN_SAVER == CUSTOM_ON
				ap_peripheral_auto_off_force_disable_set(1);
			  #endif
				curr_file_info.file_handle = -1;
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);
			
				if (ap_state_config_md_get()) 
				{
					if (!(video_record_sts & VIDEO_RECORD_MD)) 
					{
						msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
						ap_video_record_sts_set(VIDEO_RECORD_MD);
					}

					if (motion_detect_timerid == 0xFF) 
					{
						motion_detect_timerid = VIDEO_PREVIEW_TIMER_ID;
						sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_MOTION_DETECT_TICK, motion_detect_timerid, MOTION_DETECT_CHECK_TIME_INTERVAL);
					}
				}
			} 
			else 
			{
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
				video_down_flag=0;

				return STATUS_FAIL;
			}
		} 
		else if (video_record_sts & VIDEO_RECORD_UNMOUNT) 
		{
			if (ap_state_config_md_get()) 
			{
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD) 
				{
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
				} 
				else 
				{
					msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_MD);
				}
			}
			//ap_state_handling_str_draw_exit();
			//ap_state_handling_str_draw(STR_INSERT_SDC, WARNING_STR_COLOR);
			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_DISPLAY_PLEASE_INSERT_SDC, NULL, NULL, MSG_PRI_NORMAL);
			
		  #if 1//C_SCREEN_SAVER == CUSTOM_ON
			ap_peripheral_auto_off_force_disable_set(0);
		  #endif

		#else

			ap_video_record_sts_set(VIDEO_RECORD_BUSY + VIDEO_RECORD_WAIT);
		  #if 1//C_SCREEN_SAVER == CUSTOM_ON
			ap_peripheral_auto_off_force_disable_set(1);
		  #endif
			//led_type = LED_RECORD;
			//msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
			curr_file_info.file_handle = -1;
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);

		#endif
		}
	}
	video_down_flag=0;
	return STATUS_OK;
}

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
void ap_video_record_cycle_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	MEDIA_SOURCE src;
  INT32U led_type;

	ap_video_record_sts_set(~VIDEO_RECORD_WAIT);

	if (file_info_ptr->file_handle >= 0) 
	{
		gp_memcpy((INT8S *) &next_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));

		if (next_file_info.file_handle >= 0) 
		{
		  #if C_AUTO_DEL_FILE == CUSTOM_ON
			INT8U type;

			type = FALSE;
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT8U), MSG_PRI_URGENT);
		  #endif

			src.type_ID.FileHandle = next_file_info.file_handle;
			src.type = SOURCE_TYPE_FS;
			src.Format.VideoFormat = MJPEG;

	        timer_counter_force_display(1);
			if(video_encode_fast_stop_and_start(src) != START_OK)
			{
				close(src.type_ID.FileHandle);
				next_file_info.file_handle = -1;

				//ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
				//ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);
				timer_counter_force_display(0);
			led_type = LED_CARD_NO_SPACE;//LED_SDC_FULL;
		    msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);

				ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
				ap_state_handling_file_creat_set(0);

				DBG_PRINT("Video Record Fail1\r\n");
				return;
			}
			else 
			{
				gp_memcpy((INT8S *)curr_file_info.file_path_addr, (INT8S *)next_file_info.file_path_addr, 24);
				curr_file_info.file_handle = next_file_info.file_handle;
			}
			next_file_info.file_handle = -1;
		  #if C_AUTO_DEL_FILE == CUSTOM_ON
			type = TRUE;
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT8U), MSG_PRI_NORMAL);
		  #endif
		}
	}
	else 
	{
		DBG_PRINT("Cyclic video record open file Fail\r\n");
	}
}
#endif

INT8U ap_video_record_sts_get(void)
{
	return	video_record_sts;
}

void ap_video_record_reply_action(STOR_SERV_FILEINFO *file_info_ptr)
{
	MEDIA_SOURCE src;
  #if C_AUTO_DEL_FILE == CUSTOM_ON
	INT32U disk_free_size;
  #endif
	INT32U led_type;

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	INT32U time_interval, value;
	INT8U temp[6] = {0, 1, 2, 3, 5, 10};

	ap_video_record_sts_set(~VIDEO_RECORD_WAIT);
	
	value = temp[ap_state_config_record_time_get()];
	if (value == 0) 
	{
		time_interval = 5 * VIDEO_RECORD_CYCLE_TIME_INTERVAL;
	  #if C_AUTO_DEL_FILE == CUSTOM_ON
		bkground_del_disable(1);  // back ground auto delete disable
	  #endif
	} 
	else 
	{
	  #if C_AUTO_DEL_FILE == CUSTOM_ON
		bkground_del_disable(1);  // back ground auto delete enable
	  #endif
		time_interval = value * VIDEO_RECORD_CYCLE_TIME_INTERVAL + 16;  // dominant add 64 to record more
	}
#endif

	if (file_info_ptr) 
	{
		gp_memcpy((INT8S *) &curr_file_info, (INT8S *) file_info_ptr, sizeof(STOR_SERV_FILEINFO));
	} 
	src.type_ID.FileHandle = curr_file_info.file_handle;

	src.type = SOURCE_TYPE_FS;
	src.Format.VideoFormat = MJPEG;

	if (src.type_ID.FileHandle >= 0) 
	{
		timer_counter_force_display(1);
		//video_calculate_left_recording_time_disable();
		//ap_state_handling_icon_show_cmd(ICON_REC, NULL, NULL);
		if (video_encode_start(src) != START_OK)
		{
			timer_counter_force_display(0);
			//video_calculate_left_recording_time_enable();
		  #if 1//C_SCREEN_SAVER == CUSTOM_ON
			ap_peripheral_auto_off_force_disable_set(0);
		  #endif
			//ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);

			close(src.type_ID.FileHandle);
			unlink((CHAR *) curr_file_info.file_path_addr);
			curr_file_info.file_handle = -1;
			ap_state_handling_file_creat_set(0);

			ap_video_err_handle();
			ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
			msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
			DBG_PRINT("Video Record Fail\r\n");
		 } 
		 else 
		 {
		  #if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
			///if (cyclic_record_timerid == 0xFF) 
			{
				cyclic_record_timerid = VIDEO_RECORD_CYCLE_TIMER_ID;
				sys_set_timer((void*)msgQSend, (void*)ApQ, MSG_APQ_CYCLIC_VIDEO_RECORD, cyclic_record_timerid, time_interval);
			}
		  #endif

		  #if C_AUTO_DEL_FILE == CUSTOM_ON
			{
				INT8U type = TRUE;
				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK_SWITCH, &type, sizeof(INT8U), MSG_PRI_NORMAL);

				disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
				if(curr_file_info.storage_free_size < bkground_del_thread_size_get()) 
				{			        
					DBG_PRINT("\r\n[Bkground Del Detect (DskFree: %d MB)]\r\n", disk_free_size);

					if (bkground_del_disable_status_get() == 0) 
					{
						INT32U temp;

						temp = 0xFFFFFFFF;
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VIDEO_FILE_DEL, &temp, sizeof(INT32U), MSG_PRI_NORMAL);
					}
				}
			}
		  #endif
			ap_state_handling_led_blink_on();
		}
	} 
	else 
	{
		led_type = LED_CARD_NO_SPACE;//LED_SDC_FULL;
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
		ap_video_record_sts_set(~VIDEO_RECORD_BUSY);
	  #if 1//C_SCREEN_SAVER == CUSTOM_ON
		ap_peripheral_auto_off_force_disable_set(0);
	  #endif
		msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
		DBG_PRINT("Video Record Stop [NO free space]\r\n");
	}
}

/*
void video_calculate_left_recording_time_enable(void)
{
	INT64U freespace, left_recording_time;

	freespace = vfsFreeSpace(MINI_DVR_STORAGE_TYPE);

	if(freespace > (CARD_FULL_SIZE_RECORD<<20)) freespace -= (CARD_FULL_SIZE_RECORD<<20);
	else freespace = 0;

	if((my_pAviEncVidPara->encode_width == AVI_WIDTH_1080FHD) || (my_pAviEncVidPara->encode_width == AVI_WIDTH_1080P)) {
		left_recording_time = freespace / (3*1024*1024);	//estimate one frame = 100KB, so one second video takes 100KB*30 = 3MB
	} else if (my_pAviEncVidPara->encode_width == AVI_WIDTH_720P) {
		left_recording_time = freespace / (2458*1024);		//estimate one frame = 80KB, so one second video takes 80KB*30 = 2.4MB
	} else {
		left_recording_time = freespace / (1843*1024);	//estimate one frame = 60KB, so one second video takes 60KB*30 = 1.8MB
	}
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW | left_recording_time));
}

void video_calculate_left_recording_time_disable(void)
{	
	OSQPost(DisplayTaskQ, (void *) (MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR));
} */


INT32S ap_video_record_zoom_inout(INT8U inout)
{
	INT8U  err;
	INT32S nRet, msg;
	
	nRet = STATUS_OK;

#if 0
	//+++ Not support digital zoom : 0:1080FHD  1:1080P
	err = ap_state_config_video_resolution_get();
	if((err == 0)|| (err == 1))
	{
		return nRet;
	}
#else
	return nRet;
#endif
	
	if(inout){
		POST_MESSAGE(my_AVIEncodeApQ, MSG_AVI_ZOOM_IN, my_avi_encode_ack_m, 5000, msg, err);	
	}else{
		POST_MESSAGE(my_AVIEncodeApQ, MSG_AVI_ZOOM_OUT, my_avi_encode_ack_m, 5000, msg, err);	
	}
	Return:	
		return nRet;
}
