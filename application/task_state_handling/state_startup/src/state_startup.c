#include "state_startup.h"
#include "ap_music.h"
#include "ap_state_config.h"
#include "ap_state_handling.h"
#include "ap_peripheral_handling.h"
#include "ap_display.h"

#if C_LOGO == CUSTOM_ON
	static INT32S startup_logo_img_ptr;
	static INT32U startup_logo_decode_buff;
#endif

#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
static INT8U  audio_done = 0;
#endif

//	prototypes
void state_startup_init(void);
void state_startup_exit(void);
#if AUDIO_DEC_FUNC==1
extern void audio_task_init(void);
#endif
void state_startup_init(void)
{
	#if C_LOGO == CUSTOM_ON
	IMAGE_DECODE_STRUCT img_info;
	INT32U size;
	INT16U	logo_fd;
	#endif
	
	DBG_PRINT("Startup state init enter\r\n");
#if TV_DET_ENABLE
   	if(tv_plug_status_get()) {
		ap_state_handling_tv_init();
	}
#endif

	#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
	ap_music_effect_resource_init();
	audio_vol_set(ap_state_config_volume_get());
	#endif

    #if C_LOGO == CUSTOM_ON
	    startup_logo_decode_buff = (INT32U) gp_malloc_align(getDispDevBufSize(), 64);
    	if (!startup_logo_decode_buff) {
    		DBG_PRINT("State startup allocate jpeg output buffer fail.\r\n");
    		return;
    	}
    	logo_fd = nv_open((INT8U *) "POWER_ON_LOGO.JPG");
    	if (logo_fd != 0xFFFF) {
    		size = nv_rs_size_get(logo_fd);
    		startup_logo_img_ptr = (INT32S) gp_malloc(size);
    		if (!startup_logo_img_ptr) {
    			DBG_PRINT("State startup allocate jpeg input buffer fail.[%d]\r\n", size);
    			gp_free((void *) startup_logo_decode_buff);
    			return;
    		}
    		///if (nv_read(logo_fd, (INT32U) startup_logo_img_ptr, size)) 
    		{
    			DBG_PRINT("Failed to read resource_header in ap_startup_init\r\n");
    			gp_free((void *) startup_logo_img_ptr);
    			gp_free((void *) startup_logo_decode_buff);
    			return;
    		}
    		img_info.image_source = (INT32S) startup_logo_img_ptr;
    		img_info.source_size = size;
    		img_info.source_type = TK_IMAGE_SOURCE_TYPE_BUFFER;
    		img_info.output_format = C_SCALER_CTRL_OUT_RGB565;
    		img_info.output_ratio = 0;
    		img_info.out_of_boundary_color = 0x008080;

			if(ap_display_get_device()==DISP_DEV_TV) {
	    		img_info.output_buffer_width = TV_WIDTH;
	    		img_info.output_buffer_height = TV_HEIGHT;
	    		img_info.output_image_width = TV_WIDTH;
	    		img_info.output_image_height = TV_HEIGHT;
			} else {
	    		img_info.output_buffer_width = TFT_WIDTH;
	    		img_info.output_buffer_height = TFT_HEIGHT;
	    		img_info.output_image_width = TFT_WIDTH;
	    		img_info.output_image_height = TFT_HEIGHT;
	    	}
    		img_info.output_buffer_pointer = startup_logo_decode_buff;
    		if (jpeg_buffer_decode_and_scale(&img_info) == STATUS_FAIL) {
    			gp_free((void *) startup_logo_img_ptr);
    			gp_free((void *) startup_logo_decode_buff);
    			DBG_PRINT("State startup decode jpeg file fail.\r\n");
    			return;
    		}

    		OSQPost(DisplayTaskQ, (void *) (startup_logo_decode_buff|MSG_DISPLAY_TASK_JPEG_DRAW));
    		OSTimeDly(5);
    	} else {
    		DBG_PRINT("open POWER_ON_LOGO.JPG fail.\r\n");
    	}
    	gp_free((void *) startup_logo_decode_buff);
    #endif

	#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
	#if _DRV_L1_TFT == 1
	tft_backlight_en_set(TRUE);
	#endif
	if(ap_display_get_device()==DISP_DEV_TV) {
		OSTimeDly(100);
	} else {
		OSTimeDly(10);
	}

    //if (audio_effect_play(EFFECT_BEEP)) {
    //	audio_done++;
    //}
    //OSTimeDly(3);
    #if AUDIO_DEC_FUNC==1
    if (audio_effect_play(EFFECT_POWER_ON)) {
    	audio_done++;
    }
#endif
    #endif
    
    #if C_LOGO == CUSTOM_ON
	gp_free((void *) startup_logo_img_ptr);
	#endif
	//OSTimeDly(100);
}


void state_startup_entry(void *para)
{
	EXIT_FLAG_ENUM exit_flag = EXIT_RESUME;
	INT32U msg_id;
	#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
	STAudioConfirm *audio_confirm;
	#endif
	INT8U flag1 = 0;

	//task_display_init();
    #if AUDIO_DEC_FUNC==1
	audio_task_init();
#endif
	task_peripheral_handling_init();
	state_startup_init();

	while (exit_flag == EXIT_RESUME) {
		if (msgQReceive(ApQ, &msg_id, (void *) ApQ_para, AP_QUEUE_MSG_MAX_LEN) == STATUS_FAIL) {
			continue;
		}
		switch (msg_id) {
		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
			case EVENT_APQ_ERR_MSG:
			    audio_confirm = (STAudioConfirm*) ApQ_para;
			    if (audio_confirm->result_type == MSG_AUD_PLAY_RES && audio_confirm->result != AUDIO_ERR_NONE) {
			        audio_done++;
			    }
			    break;
	    #endif

			case MSG_STORAGE_SERVICE_MOUNT:
			case MSG_STORAGE_SERVICE_NO_STORAGE:
				flag1 = 1;
				ap_state_handling_storage_id_set(ApQ_para[0]);
        		break;

#if C_BATTERY_DETECT == CUSTOM_ON
        	case MSG_APQ_BATTERY_LVL_SHOW:
        		//ap_state_handling_battery_icon_show(ApQ_para[0]);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_SHOW:
				//ap_state_handling_charge_icon_show(1);
        		break;
        	case MSG_APQ_BATTERY_CHARGED_CLEAR:
				//ap_state_handling_charge_icon_show(0);
        		break;
#endif
			default:
				break;
		}
		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
		if((audio_done == 1) && flag1) 
		#else
		if(flag1)
		#endif
		{
			exit_flag = EXIT_BREAK;
		}
	}
	
    ap_state_firmware_upgrade();

	ap_startup_init();
//	R_SYSTEM_CTRL |= 0x30;			//27M => weak mode : |= 0x30
//	R_SYSTEM_CLK_CTRL |= 0x1000;	//32768	=>   mode : |= 0x1000

	state_startup_exit();
}


void state_startup_exit(void)
{
	INT8U temp;

    //DBG_PRINT("Exit Startup state\r\n");
	//gpio_write_io(SPEAKER_EN, DATA_LOW);
//-----G_sensor
	temp = ap_state_config_G_sensor_get();
   //ap_gsensor_set_sensitive(temp);
	temp = ap_gsensor_power_on_get_status();
	if(temp){// g_sensor power on
		//msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_G_SENSOR_POWER_ON_START, NULL, NULL, MSG_PRI_NORMAL);
	}

//------Flsh LED
	temp = ap_state_config_flash_LED_get();	
//	ap_peripheral_night_mode_set(temp);

		OSQPost(StateHandlingQ, (void *) STATE_VIDEO_RECORD);
   
	//msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TV_POLLING_START, NULL, NULL, MSG_PRI_NORMAL);
}
