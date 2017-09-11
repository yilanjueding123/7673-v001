#include "ap_state_config.h"
#include "state_video_record.h"
#include "ap_state_handling.h"
#include "ap_video_record.h"
#include "ap_display.h"
#include "avi_encoder_app.h"

//	prototypes
INT8S state_video_record_init(INT32U state);
void state_video_record_exit(void);

extern void ap_video_record_md_icon_update(INT8U sts);
extern void ap_video_record_md_icon_clear_all(void);
extern void ap_video_capture_mode_switch(INT8U DoSensorInit, INT16U EnterAPMode);

extern STOR_SERV_FILEINFO curr_file_info;
extern STOR_SERV_FILEINFO next_file_info;

INT8S state_video_record_init(INT32U state)
{
	DBG_PRINT("video_record state init enter\r\n");
	if (ap_video_record_init(state) == STATUS_OK) {
		return STATUS_OK;
	} else {
		return STATUS_FAIL;
	}
}
INT8U cyc_restart_flag=0;
static INT8U check_card_space(void)
{
   INT64U  disk_free_size;
   INT32S  led_type=-1;
   disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
   DBG_PRINT("\r\n[Disk free %dMB]\r\n", disk_free_size);
		  if (disk_free_size < CARD_FULL_SIZE_RECORD)
			led_type = LED_SDC_FULL;
		  else  if (disk_free_size < CARD_FULL_MB_SIZE)
		    led_type = LED_CARD_NO_SPACE;
			   
   if(led_type !=-1)
		{
		  msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);

		  return 0;
		}
   return 1;
		  
}

volatile INT8U cyc_record_flag=0;
void state_video_record_entry(void *para, INT32U state)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id,led_type;
    INT8U power_on_flag;
#if AUDIO_DEC_FUNC==1
	STAudioConfirm *audio_temp;
#endif
     INT32S nRet;
	INT8U	send_active=0;
  #if C_MOTION_DETECTION == CUSTOM_ON
	INT8U md_check_tick = 0;
  #endif
    cyc_record_flag=0;

	power_on_flag = 0;
	//DBG_PRINT("RUN_CODE\r\n");
	if(ap_state_handling_storage_id_get()==NO_STORAGE)
	{
		led_type = LED_NO_SDC;
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
	}
	else
	{
		led_type = LED_WAITING_RECORD;
	}
//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
	
	if(*(INT32U *) para == STATE_STARTUP) { // 開機後進來
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_USBD_DETECT_INIT, NULL, NULL, MSG_PRI_NORMAL);
		power_on_flag = 1;
	} 
	#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
	else if((*(INT32U *) para == STATE_SETTING)|| (*(INT32U *) para == STATE_BROWSE)) { // 菜單後進來
	#else
	else if((*(INT32U *) para == STATE_SETTING)|| (*(INT32U *) para == STATE_VIDEO_PREVIEW)||(*(INT32U *) para == STATE_AUDIO_RECORD)) {
	#endif
		 ap_video_capture_mode_switch(0,STATE_VIDEO_RECORD); 
		 msgQSend(ApQ, MSG_APQ_VIDEO_RECORD_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
	}

	state_video_record_init(state);

	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}
		
		switch (msg_id) {
			case EVENT_APQ_ERR_MSG:
				#if AUDIO_DEC_FUNC==1
				audio_temp = (STAudioConfirm *)ApQ_para;
				if ((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_APP_RS)) {
				//	gpio_write_io(SPEAKER_EN, DATA_LOW);
				} else if((audio_temp->result == AUDIO_ERR_DEC_FINISH) && (audio_temp->source_type == AUDIO_SRC_TYPE_FS)) {
					audio_confirm_handler((STAudioConfirm *)ApQ_para);
				} else {
					audio_confirm_handler((STAudioConfirm *)ApQ_para);
				}
				#endif
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
        		ap_state_handling_storage_id_set(ApQ_para[0]);
				//ap_state_handling_str_draw_exit();
        		//ap_state_handling_icon_clear_cmd(ICON_INTERNAL_MEMORY, NULL, NULL);
        		//ap_state_handling_icon_show_cmd(ICON_SD_CARD, NULL, NULL);
        		ap_video_record_sts_set(~VIDEO_RECORD_UNMOUNT);
        		//video_calculate_left_recording_time_enable();
				if (ap_state_config_md_get()) {
					//ap_state_handling_icon_show_cmd(ICON_MD_STS_0, NULL, NULL);
//					ap_state_handling_icon_clear_cmd(ICON_MOTION_DETECT_START, NULL, NULL);
					//ap_state_handling_icon_show_cmd(ICON_MOTION_DETECT, NULL, NULL);
					msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_MOTION_DETECT_START, NULL, NULL, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_MD);
				}
//        		ap_music_update_icon_status();
			//	led_type = LED_WAITING_RECORD;
			//	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
        		DBG_PRINT("[Video Record Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
        		led_type = LED_NO_SDC;
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, &ApQ_para[0], sizeof(INT8U), MSG_PRI_NORMAL);
					break;
				}
        		ap_state_handling_storage_id_set(ApQ_para[0]);
   				//ap_music_update_icon_status();
				//ap_state_handling_mp3_index_show_zero_cmd();
				//ap_state_handling_mp3_total_index_show_zero_cmd();

   				msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_AUTO_DEL_LOCK, NULL, NULL, MSG_PRI_NORMAL);   
#if C_MOTION_DETECTION == CUSTOM_ON
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					//ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
				}
#endif
				if (ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					ap_video_record_func_key_active();
				}
        		//video_calculate_left_recording_time_disable();
        		//ap_state_handling_str_draw_exit();
        		//ap_state_handling_icon_clear_cmd(ICON_SD_CARD, NULL, NULL);
        		//ap_state_handling_icon_show_cmd(ICON_INTERNAL_MEMORY, NULL, NULL);

				//ap_state_handling_icon_clear_cmd(ICON_LOCKED, NULL, NULL);
				//ap_state_handling_icon_clear_cmd(ICON_VIDEO_LDW_SART, NULL, NULL);

        		ap_video_record_sts_set(VIDEO_RECORD_UNMOUNT);
				power_on_flag = 0;
        		DBG_PRINT("[Video Record Mount FAIL]\r\n");
        		break;

#if C_MOTION_DETECTION == CUSTOM_ON
			case MSG_APQ_MOTION_DETECT_TICK:
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}
				ap_video_record_md_tick(&md_check_tick);
				break;
			case MSG_APQ_MOTION_DETECT_ICON_UPDATE:
				ap_video_record_md_icon_update(ApQ_para[0]);
				break;
			case MSG_APQ_MOTION_DETECT_ACTIVE:
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}
				ap_video_record_md_active(&md_check_tick);
				break;
#endif
			case MSG_APQ_AUDO_ACTIVE:
						  if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)) 
							  {
								if(ap_video_record_func_key_active())
									{
								  
									}
							  }
						  //video_calculate_left_recording_time_disable();
						  OSTimeDly(3);
						  power_on_flag = 0;
			
						  vid_enc_disable_sensor_clock();
						  OSQPost(StateHandlingQ, (void *) STATE_AUDIO_RECORD);
						  exit_flag = EXIT_BREAK;
						   break;

        	case MSG_APQ_FUNCTION_KEY_ACTIVE:
        	case MSG_APQ_VIDEO_RECORD_ACTIVE:
				power_on_flag = 0;
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}

			#if C_MOTION_DETECTION == CUSTOM_ON
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					//ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
				}
			#endif

				ap_video_record_func_key_active();
        		break;

			case MSG_APQ_POWER_ON_AUTO_RECORD:
#if C_MOTION_DETECTION == CUSTOM_ON
        		if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					power_on_flag = 0;
        		}
        		else
#endif
				if(power_on_flag) {
					power_on_flag = 0;
					ap_video_record_func_key_active();
		        	DBG_PRINT("AUTO_Record_start!!!!");
		        }
				break;

        	case MSG_APQ_POWER_KEY_ACTIVE:
				if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)) {
					if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
						OSTimeDly(2);
						msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
						break;
					}
					ap_video_record_func_key_active();
				}
        		OSTimeDly(10);
				video_encode_exit();
        		//video_calculate_left_recording_time_disable();
        		ap_state_handling_power_off();
        		break;

        	case MSG_APQ_MENU_KEY_ACTIVE:
				if (ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					break;
				}
#if C_MOTION_DETECTION == CUSTOM_ON
        		if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					//ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
        		}
#endif
				//ap_state_handling_str_draw_exit();
				OSTimeDly(10);	//wait to update display				
				power_on_flag = 0;

 				vid_enc_disable_sensor_clock();
        		OSQPost(StateHandlingQ, (void *) STATE_SETTING);
	     		exit_flag = EXIT_BREAK;
        		break;

			case MSG_APQ_CAPTURE_KEY_ACTIVE:
	       	case MSG_APQ_MODE:
	       	case MSG_APQ_CAPTUER_ACTIVE:
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}

#if C_MOTION_DETECTION == CUSTOM_ON
        		if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
				//	ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
        		}
#endif
				if (ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					//ap_video_record_func_key_active();
					send_active=0;
					break;
				}
				else
				    send_active=1;
				#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
				//ap_state_handling_str_draw_exit();
				#endif
		        //video_calculate_left_recording_time_disable();
				OSTimeDly(3);
				power_on_flag = 0;

				vid_enc_disable_sensor_clock();
        		OSQPost(StateHandlingQ, (void *) STATE_VIDEO_PREVIEW);
        		#if GPDV_BOARD_VERSION == GPCV1237A_Aerial_Photo
				if(send_active)
        		msgQSend(ApQ, MSG_APQ_CAPTUER_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
                else
				{
				   led_type = LED_WAITING_CAPTURE;
				   msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
				}
        		#endif
        		exit_flag = EXIT_BREAK;
				break;

        	case MSG_STORAGE_SERVICE_VID_REPLY:
        		ap_video_record_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
                 if(ap_state_handling_file_creat_get())
					{
					if(cyc_record_flag==0)
						{
					       led_type = LED_RECORD;
		                   msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
						}
					 cyc_record_flag=0;
					}
        		break;

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
		case MSG_STORAGE_SERVICE_VID_CYCLIC_REPLY:
        		ap_video_record_cycle_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		break;

        	case MSG_APQ_CYCLIC_VIDEO_RECORD:
        		if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
       				break;
        		}
				else if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY) 
				{
				    cyc_record_flag=1;
					video_encode_stop();
					ap_video_record_sts_set(VIDEO_RECORD_BUSY);
					curr_file_info.file_handle = -1;
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_VID_REQ, NULL, NULL, MSG_PRI_NORMAL);
					ap_video_record_sts_set(VIDEO_RECORD_WAIT);
				}
        		break;
#endif
        	case MSG_APQ_NEXT_KEY_ACTIVE:
        		if((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)){
        		  #if KEY_TYPE != KEY_TYPE2 && KEY_TYPE != KEY_TYPE4
        			msgQSend(ApQ, MSG_APQ_SOS_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
        		  #endif
        		}else{
        			ap_video_record_zoom_inout(0);
        		}
       			break;

        	case MSG_APQ_PREV_KEY_ACTIVE:
      			{
      				INT8U temp;
      				temp = ap_state_config_voice_record_switch_get();
      				if(temp){
      					ap_state_config_voice_record_switch_set(0);
      				}else{
       					ap_state_config_voice_record_switch_set(1);
     				}
					if(ap_state_config_voice_record_switch_get() != 0) {
						//ap_state_handling_icon_clear_cmd(ICON_MIC_OFF, NULL, NULL);
					} else {
						//ap_state_handling_icon_show_cmd(ICON_MIC_OFF, NULL, NULL);
					}
      				
        			ap_video_record_zoom_inout(1);
        		}
        		break;

			case MSG_APQ_PARK_MODE_SET:
				if(ap_state_config_park_mode_G_sensor_get())
				{
					ap_state_config_park_mode_G_sensor_set(0);
					ap_video_record_clear_park_mode();
					DBG_PRINT("PARK MODE OFF\r\n");
				}
				else
				{
					ap_state_config_park_mode_G_sensor_set(1);
					ap_video_record_show_park_mode();
					DBG_PRINT("PARK MODE ON\r\n");
				}
				break;

			case MSG_APQ_CONNECT_TO_PC:
				#if TV_DET_ENABLE   // for tv out
				if(ap_display_get_device() != DISP_DEV_TFT) break;
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TV_POLLING_STOP, NULL, NULL, MSG_PRI_NORMAL);
				OSTimeDly(3);
				#endif   // end for tv out
				power_on_flag = 0;
				//led_type =LED_USB_CONNECT;// LED_RECORD;
				//msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
//				audio_send_stop();
#if C_MOTION_DETECTION == CUSTOM_ON
				if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					//ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
				}
#endif
				if (ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					ap_video_record_func_key_active();
				}
        		//video_calculate_left_recording_time_disable();
				//ap_state_handling_str_draw_exit();

				/*
					先將現今preview 關掉切換成web cam
				*/
        		vid_enc_disable_sensor_clock();
				video_encode_preview_off();

        		ap_state_handling_connect_to_pc(STATE_VIDEO_RECORD);
        		break;

        	case MSG_APQ_DISCONNECT_TO_PC:
        		#if TV_DET_ENABLE   // for tv out
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TV_POLLING_START, NULL, NULL, MSG_PRI_NORMAL);
				#endif   // end for tv out
				power_on_flag = 0;
				led_type = LED_WAITING_RECORD;
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
        		ap_state_handling_disconnect_to_pc();
	        	OSTimeDly(100);
				/*
				if(ap_state_handling_storage_id_get() != NO_STORAGE) {
	        		video_calculate_left_recording_time_enable();
				} */

				/*
					將web cam preview 關換成 video preview
				*/
				vid_enc_disable_sensor_clock();
	       		video_encode_preview_on();
				/* sensor不關,將sensor 送出的資料引導到底層preview flow */
		   		ap_video_capture_mode_switch(0, STATE_VIDEO_RECORD);
        		break;

              case MSG_APQ_RECORD_SWITCH_FILE:
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}

			  #if C_MOTION_DETECTION == CUSTOM_ON
				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
	        		ap_video_record_func_key_active();
	        		ap_video_record_func_key_active();
	        	}
			  #endif
        		break;

        	case MSG_APQ_VDO_REC_RESTART:
				///ap_video_record_func_key_active(msg_id);
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
       				break;
        		       }

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					cyc_restart_flag=1;
	        		ap_video_record_func_key_active();
					cyc_restart_flag=0;
	        		ap_video_record_func_key_active();
	        	       }
				break;
            case MSG_APQ_VDO_REC_STOP:
				
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
       				break;
        		       }

				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					ap_video_record_func_key_active();
				}

			    //ap_state_handling_str_draw_exit();
			    //ap_state_handling_str_draw(STR_SD_FULL, WARNING_STR_COLOR);
				//video_calculate_left_recording_time_disable();
				break;

        	case MSG_APQ_AVI_PACKER_ERROR:
				DBG_PRINT("pack_err\r\n");
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}

				if (ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					ap_video_record_func_key_active();
				}

				if(ApQ_para[0])
				{
				DBG_PRINT("jh_dbg_20170616_1\r\n");
                    nRet = _devicemount(MINI_DVR_STORAGE_TYPE);
		            if(FatSpeedUp_Array[MINI_DVR_STORAGE_TYPE].UsedCnt == 0)
		            {
		                if(gSelect_Size == 5*1024*1024/512)   // all free space smaller 5M, not 500K
		                {   
		                    gSelect_Size = 500*1024/512;
			                nRet = _devicemount(MINI_DVR_STORAGE_TYPE);
			                if(FatSpeedUp_Array[MINI_DVR_STORAGE_TYPE].UsedCnt == 0)
			                {
			                    gOutOfFreeSpace = 1;
			                }
		                }
						else     // all free space smaller 500K
						{   
						    gOutOfFreeSpace = 1;
						}
		            }
           
					ap_video_record_func_key_active();
				}
                else if(drvl2_sdc_live_response() == 0)
                {  // Card exist
#if C_MOTION_DETECTION == CUSTOM_ON
					if (ap_state_config_md_get()) {
					    break;  // motion detect mode..... no need restart
					}
					else
#endif
					{
					DBG_PRINT("jh_dbg_20170616_2\r\n");
						ap_video_record_func_key_active();
					}
                } else {
#if C_MOTION_DETECTION == CUSTOM_ON
					// if no card, md detection function need to disable
					if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
						//ap_video_record_md_icon_clear_all();
						ap_video_record_md_disable();
						ap_state_config_md_set(0);
					}
#endif
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, NULL, MSG_PRI_NORMAL);
				DBG_PRINT("jh_dbg_20170616_3\r\n");

                }
                break;

#if C_BATTERY_DETECT == CUSTOM_ON
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		//ap_state_handling_battery_icon_show(ApQ_para[0]);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:
				//ap_state_handling_charge_icon_show(1);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:
			//	ap_state_handling_charge_icon_show(0);
        		break;
        	case MSG_APQ_BATTERY_LOW_SHOW:
        		if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)) {
        			if(ap_video_record_func_key_active()) {
        				break;
        			}
        		}
        		OSTimeDly(5);
        		//ap_state_handling_clear_all_icon();
				//ap_state_handling_str_draw_exit();
				//ap_state_handling_str_draw(STR_BATTERY_LOW, WARNING_STR_COLOR);
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_START, NULL, NULL, MSG_PRI_NORMAL);
        		break;        		        		
#endif

			case MSG_APQ_NIGHT_MODE_KEY:
                                #if AUDIO_DEC_FUNC==1
				audio_effect_play(EFFECT_CLICK);
                                #endif
				ap_state_handling_night_mode_switch();
				break;

			/*
			case MSG_APQ_AUDIO_EFFECT_MENU:
			    if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY) == 0) {
			        ap_state_common_handling(msg_id);
			    }
			    break;
			*/

			case MSG_APQ_AUDIO_EFFECT_UP:
			case MSG_APQ_AUDIO_EFFECT_DOWN:
			    break;			

			case MSG_APQ_USER_CONFIG_STORE:
				ap_state_config_store();
				break;

#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
			case MSG_APQ_SOS_KEY_ACTIVE:
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}
				if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)) {
					//audio_effect_play(EFFECT_CLICK);
					//ap_video_record_lock_current_file();
					//ap_state_handling_icon_show_cmd(ICON_LOCKED, NULL, NULL);
				}
				break;
#endif
#if 0
			case MSG_APQ_HDMI_PLUG_IN:
        		if ((ap_video_record_sts_get() & VIDEO_RECORD_BUSY)) {
        			if(ap_video_record_func_key_active(msg_id)) {
        				break;
        			}
        		}
#if C_MOTION_DETECTION == CUSTOM_ON
        		if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					//ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
					ap_state_config_md_set(0);
        		}
#endif
				//ap_state_handling_str_draw_exit();
				//video_calculate_left_recording_time_disable();
				power_on_flag = 0;

 				vid_enc_disable_sensor_clock();
        		OSQPost(StateHandlingQ, (void *) STATE_VIDEO_PREVIEW);
				msgQSend(ApQ, MSG_APQ_HDMI_PLUG_IN, NULL, NULL, MSG_PRI_NORMAL);
        		exit_flag = EXIT_BREAK;
			break;
#endif
			//+++ TV_OUT_D1
			case MSG_APQ_TV_PLUG_OUT:
			case MSG_APQ_TV_PLUG_IN:
				if (ap_video_record_sts_get() & VIDEO_RECORD_WAIT) {
					OSTimeDly(2);
					msgQSend(ApQ, msg_id, NULL, NULL, MSG_PRI_NORMAL);
					break;
				}
#if C_MOTION_DETECTION == CUSTOM_ON
        		if(ap_video_record_sts_get() & VIDEO_RECORD_MD) {
					//ap_video_record_md_icon_clear_all();
					ap_video_record_md_disable();
//					ap_state_config_md_set(0);
        		}
#endif
				if (ap_video_record_sts_get() & VIDEO_RECORD_BUSY) {
					ap_video_record_func_key_active();
				}
				//ap_state_handling_str_draw_exit();
				ap_video_record_clear_resolution_str();
				ap_video_record_clear_park_mode();

				vid_enc_disable_sensor_clock();
				if(msg_id == MSG_APQ_TV_PLUG_IN) {
					//ap_state_handling_tv_init();
 				} else {
					//ap_state_handling_tv_uninit();
 				}
				ap_video_capture_mode_switch(0, STATE_VIDEO_RECORD);
				ap_video_record_resolution_display();
				if(ap_state_config_park_mode_G_sensor_get()) {
					ap_video_record_show_park_mode();
				}
				power_on_flag = 0;
			break;
			//---

			default:
				if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY){
					if((msg_id != MSG_APQ_AUDIO_EFFECT_OK) && (msg_id != MSG_APQ_AUDIO_EFFECT_MODE)) {
						break;
					}
				}
				ap_state_common_handling(msg_id);
				break;
		}
	}
	state_video_record_exit();
}

void state_video_record_exit(void)
{
	ap_video_record_exit();
	DBG_PRINT("Exit video_record state\r\n");
}
