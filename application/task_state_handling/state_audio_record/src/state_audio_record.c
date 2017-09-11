#include "state_audio_record.h"
#include "ap_state_handling.h"
#include "ap_state_config.h"
#include "ap_audio_record.h"
#include "ap_music.h"
#include "audio_record.h"

#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)
//	prototypes
INT8S state_audio_record_init(INT32U prev_state);
void state_audio_record_exit(INT32U next_state_msg);
void state_audio_record_exit_entry_PC_CAM(INT32U next_state_msg);

extern INT8U ap_audio_record_sts_get(void);
extern INT32S vid_enc_disable_sensor_clock(void);

static INT8U g_during_switch_process = 0;

extern INT8U audio_record_proc;

extern void video_encode_preview_restart(void);

INT8S state_audio_record_init(INT32U prev_state)
{
	DBG_PRINT("audio_record state init enter\r\n");
	
	audio_encode_entrance();    // create audio task
	if (ap_audio_record_init(prev_state) == STATUS_OK) {
			return STATUS_OK;
		} else {
			return STATUS_FAIL;
		}
	return STATUS_OK;
}
static INT8U audio_record_from=0;
static INT8U send_apq_message=0;
void state_audio_record_entry(void *para)
{
#if 1
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id, prev_state, audio_record_status_temp = 0, disk_free_size;
	INT32U led_type;
//	INT16U temp_vol;
	INT8U switch_flag;
    INT8U type;
	prev_state = *((INT32U *) para);
	if(ap_state_handling_storage_id_get()==NO_STORAGE)
	{
	//SWITCH开关直接进PREVIEW时无卡消息会接收不到
		led_type = LED_NO_SDC;
		msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
		
			
	}
	if(*(INT32U *) para == STATE_STARTUP) 
	{
	        type=2;
			msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_USBD_DETECT_INIT, &type, sizeof(INT8U), MSG_PRI_NORMAL);
					
	}
	else if((*(INT32U *) para == STATE_VIDEO_PREVIEW)||(*(INT32U *) para == STATE_VIDEO_RECORD))
		{
		 
		 	msgQSend(ApQ, MSG_APQ_AUDO_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
		}
	switch_flag = 0;
	state_audio_record_init(prev_state);
	DBG_PRINT("audio_record state init enter OK!\r\n");
	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}
		switch (msg_id) {
			case EVENT_APQ_ERR_MSG:
				  #if AUDIO_DEC_FUNC==1
				audio_confirm_handler((STAudioConfirm *)ApQ_para);
				  #endif
				break;
			case MSG_STORAGE_SERVICE_MOUNT:
				ap_state_handling_storage_id_set(ApQ_para[0]);
			//	ap_state_handling_str_draw_exit();
        	//	ap_state_handling_icon_clear_cmd(ICON_NO_SD_CARD, NULL, NULL);
        		//ap_music_update_icon_status();
        		ap_audio_record_sts_set(~AUDIO_RECORD_UNMOUNT);
				//audio_calculate_left_recording_time_enable();
        		//ap_audio_record_timer_draw();
        		DBG_PRINT("[Audio Record Mount OK]\r\n");
        		break;
        	case MSG_STORAGE_SERVICE_NO_STORAGE:
				led_type = LED_NO_SDC;
				msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
        		ap_state_handling_storage_id_set(ApQ_para[0]);
        		//ap_state_handling_str_draw_exit();
        		//ap_state_handling_icon_show_cmd(ICON_NO_SD_CARD, NULL, NULL);
 				//ap_state_handling_mp3_index_show_zero_cmd();
 				//ap_state_handling_mp3_total_index_show_zero_cmd();
        		ap_audio_record_sts_set(AUDIO_RECORD_UNMOUNT);
        		if (ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
        			// Stop audio recording
        			ap_audio_record_func_key_active();
        		}
				//audio_calculate_left_recording_time_disable();
				//ap_audio_record_timer_draw();
				//DBG_PRINT("F+++\r\n");
        		DBG_PRINT("[Audio Record Mount FAIL]\r\n");
        		break;

        	case MSG_APQ_MENU_KEY_ACTIVE:
				if(audio_record_proc) {
					break;
				}

			//	ap_state_handling_str_draw_exit();
			//	ap_audio_record_timer_draw();
				OSTimeDly(3);	//wait to update display
				OSQPost(StateHandlingQ, (void *) STATE_SETTING);
				exit_flag = EXIT_BREAK;
	       		break;

        	case MSG_APQ_NEXT_KEY_ACTIVE:
        	case MSG_APQ_PREV_KEY_ACTIVE:
        		break;

        	case MSG_APQ_MODE:
				#if 0
	           DBG_PRINT("audio state Mode key\r\n");
				if(audio_record_proc) {
					break;
				}
				ap_state_handling_str_draw_exit();
				ap_audio_record_timer_draw();
				OSTimeDly(3);	//wait to update display
        		OSQPost(StateHandlingQ, (void *) STATE_BROWSE);
	            DBG_PRINT("audio state Mode key OK!\r\n");
	        	exit_flag = EXIT_BREAK;
                 #endif
        		break;
			
			case MSG_APQ_VIDEO_RECORD_ACTIVE:
				if (ap_audio_record_sts_get() == AUDIO_RECORD_BUSY) {
					//DBG_PRINT("1++\r\n");
					ap_audio_record_func_key_active();
					}
				OSQPost(StateHandlingQ, (void *) STATE_VIDEO_RECORD);
	        	exit_flag = EXIT_BREAK;
				break;
			case MSG_APQ_CAPTUER_ACTIVE:
				
				if (ap_audio_record_sts_get() == AUDIO_RECORD_BUSY) {
					//DBG_PRINT("2++\r\n");
        			ap_audio_record_func_key_active();	//Stop audio recording
        			send_apq_message=0;
        		}
				else
					send_apq_message=0;
				OSQPost(StateHandlingQ, (void *) STATE_VIDEO_PREVIEW);
				if(send_apq_message)
				msgQSend(ApQ, MSG_APQ_CAPTUER_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
				exit_flag = EXIT_BREAK;
				break;
			case MSG_APQ_POWER_ON_AUDO_ACTIVE:
				if (ap_state_handling_storage_id_get() != NO_STORAGE)
					{
				    audio_record_from=1;
					//DBG_PRINT("3++\r\n");
				    ap_audio_record_func_key_active();
					}
				break;
            case MSG_APQ_AUDO_ACTIVE:
        	case MSG_APQ_FUNCTION_KEY_ACTIVE:		// OK key pressed
      	
				if (ap_state_handling_storage_id_get() == NO_STORAGE) {
					//ap_state_handling_str_draw_exit();
					//ap_state_handling_str_draw(STR_NO_SD, 0xF800);
					//ap_audio_record_timer_draw();
				} else {
				     audio_record_from=0;
					// DBG_PRINT("4++\r\n");
					ap_audio_record_func_key_active();
				}
				
        		break;

        	case MSG_STORAGE_SERVICE_AUD_REPLY:
        		ap_audio_record_reply_action((STOR_SERV_FILEINFO *) ApQ_para);
        		switch_flag = 0;
				if(ap_state_handling_file_creat_get())
					{
					 //if(audio_record_from == 0)
					 	led_type=LED_AUDIO_RECORD;
					// else
					// 	led_type=LED_AUTO_AUDIO_RECORD;
						msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
					 	
					}
        		break;

        	case MSG_APQ_RECORD_SWITCH_FILE:
        		if (!switch_flag) {
	        		g_during_switch_process = 1;
	        		switch_flag = 1;
					//DBG_PRINT("5++\r\n");
	        		ap_audio_record_func_key_active();
	        		ap_audio_record_func_key_active();
	        	}
        		break;

        	case MSG_APQ_POWER_KEY_ACTIVE:
        		ap_audio_record_exit(msg_id);
        		ap_state_handling_power_off();
        		break;

        	case MSG_APQ_CONNECT_TO_PC:
        		if (ap_audio_record_sts_get() == AUDIO_RECORD_BUSY) {
					//DBG_PRINT("6++\r\n");
        			ap_audio_record_func_key_active();	//Stop audio recording
        		}
				//ap_audio_record_timer_stop();
        		//audio_calculate_left_recording_time_disable();
				//ap_state_handling_str_draw_exit();
				//ap_audio_record_timer_draw();
				OSTimeDly(20);
				msgQFlush(ApQ);
        		if (ap_state_config_usb_mode_get() == 0) {
        			state_audio_record_exit_entry_PC_CAM(msg_id);
        		}
				ap_state_handling_connect_to_pc(STATE_AUDIO_RECORD);
        		break;

        	case MSG_APQ_DISCONNECT_TO_PC:
				ap_state_handling_disconnect_to_pc();
				OSTimeDly(100);
				if (ap_state_config_usb_mode_get() == 0) {
					vid_enc_disable_sensor_clock();
				} else {
					if(ap_state_handling_storage_id_get() != NO_STORAGE) {
		        		//audio_calculate_left_recording_time_enable();
					}
					//ap_audio_record_timer_draw();
					#if 0
					ap_audio_record_timer_start();
					#endif
        		}
        		//ap_music_update_icon_status();		// Restore MP3 icons
				break;
#if 0//C_BATTERY_DETECT == CUSTOM_ON
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		ap_state_handling_battery_icon_show(ApQ_para[0]);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:
				ap_state_handling_charge_icon_show(1);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:
				ap_state_handling_charge_icon_show(0);
        		break;
#endif
#if C_SCREEN_SAVER == CUSTOM_ON
			case MSG_APQ_KEY_IDLE:
        		ap_state_handling_lcd_backlight_switch(0);        	
        		break;
			case MSG_APQ_KEY_WAKE_UP:
        		ap_state_handling_lcd_backlight_switch(1);        	
        		break;        		
#endif 

			case MSG_APQ_USER_CONFIG_STORE:
				ap_state_config_store();
				break;
			case MSG_APQ_DISPLAY_DRAW_TIMER:
				DBG_PRINT("MSG_APQ_DISPLAY_DRAW_TIMER+=\r\n");
				#if 0
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					audio_record_status_temp = audio_record_get_status();
					if(g_during_switch_process && (audio_record_status_temp == 0x00000002)){
						g_during_switch_process = 0;
						DBG_PRINT("finally audio_record_get_status = 0x%x\r\n", audio_record_status_temp);
					}
					else if( (!g_during_switch_process) && ((audio_record_status_temp == 0) || (audio_record_status_temp == 0x1)) )	//check if the recording status is STOP or not
					{
						timer_counter_force_display(0);
						audio_calculate_left_recording_time_disable();
						ap_peripheral_auto_off_force_disable_set(0);
						ap_audio_record_sts_set(~AUDIO_RECORD_BUSY);
						ap_state_handling_led_on();
						ap_state_handling_icon_clear_cmd(ICON_REC, NULL, NULL);

						disk_free_size = vfsFreeSpace(MINI_DVR_STORAGE_TYPE) >> 20;
						if (disk_free_size < CARD_FULL_MB_SIZE) {
							ap_state_handling_str_draw_exit();
							ap_state_handling_str_draw(STR_SD_FULL, 0xF800);
						}
						msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
					}
				}
				ap_audio_record_timer_draw();
				#endif
				break;

			case MSG_APQ_AUDIO_ENCODE_ERR:
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					//DBG_PRINT("7++\r\n");
					ap_audio_record_func_key_active();
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_TIMER_START, NULL, NULL, MSG_PRI_NORMAL);
					msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);
				}
				break;

			case MSG_APQ_AUDIO_EFFECT_MENU:
				if(ap_audio_record_sts_get() & AUDIO_RECORD_BUSY) {
					break;
				}
			case MSG_APQ_AUDIO_EFFECT_OK:
			case MSG_APQ_AUDIO_EFFECT_MODE:
				ap_state_common_handling(msg_id);
				break;

			default:
				break;
		}
	}

	if (exit_flag == EXIT_BREAK) {
		state_audio_record_exit(msg_id);
	}
	#endif
}

void state_audio_record_exit(INT32U next_state_msg)
{
	ap_audio_record_exit(next_state_msg);
	audio_encode_exit();  // delete audio record task
	DBG_PRINT("Exit audio_record state\r\n");
}

void state_audio_record_exit_entry_PC_CAM(INT32U next_state_msg)
{
	state_audio_record_exit(next_state_msg);

}

#endif  //#if (defined AUD_RECORD_EN) && (AUD_RECORD_EN==1)
