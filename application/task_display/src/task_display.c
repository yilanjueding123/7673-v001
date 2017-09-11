
#include "task_display.h"
#include "ap_display.h"
#if 0
#define DISPLAY_TASK_QUEUE_MAX	256 //64

OS_EVENT *DisplayTaskQ;
OS_EVENT *display_task_ack_m;

void *display_task_q_stack[DISPLAY_TASK_QUEUE_MAX];

void task_display_init(void)
{
	DisplayTaskQ = OSQCreate(display_task_q_stack, DISPLAY_TASK_QUEUE_MAX);
	display_task_ack_m = OSMboxCreate(NULL);

	ap_display_init();
	ap_display_effect_init();
}

void task_display_entry(void *para)
{
	INT32U msg_id;
	INT8U err;
	
	while(1) {
		msg_id = (INT32U) OSQPend(DisplayTaskQ, 0, &err);
		if((!msg_id) || (err != OS_NO_ERR)) {
        	continue;
        }
        switch (msg_id & 0xFF000000) {
			// TV
			case MSG_DISPLAY_TASK_TV_INIT:
				#if TV_DET_ENABLE   // for tv out
				ap_display_tv_init();
	           	OSMboxPost(display_task_ack_m, (void*)C_ACK_SUCCESS);
	           	#endif   // end for tv out
				break;
			case MSG_DISPLAY_TASK_TV_UNINIT:
				#if TV_DET_ENABLE   // for tv out
				ap_display_tv_uninit();
	           	OSMboxPost(display_task_ack_m, (void*)C_ACK_SUCCESS);
	           	#endif   // end for tv out
				break;
			
			// HDMI
			case MSG_DISPLAY_TASK_HDMI_INIT:
				//ap_display_hdmi_init();
				//OSMboxPost(display_task_ack_m, (void*)C_ACK_SUCCESS);
			break;
			case MSG_DISPLAY_TASK_HDMI_UNINIT:
				#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
				ap_display_hdmi_uninit();
	           	OSMboxPost(display_task_ack_m, (void*)C_ACK_SUCCESS);
	           	#endif
				break;

        	case MSG_DISPLAY_TASK_HDMI_WAV_TIME_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_hdmi_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_WAV_TIME);
        		#endif
			break;
        	case MSG_DISPLAY_TASK_HDMI_JPEG_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_hdmi_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_JPEG);
        		#endif
        	break;
        	case MSG_DISPLAY_TASK_HDMI_MJPEG_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_hdmi_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_MJPEG);
        		#endif
        	break;

        	case MSG_DISPLAY_TASK_EFFECT_INIT:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_effect_init();
        		#endif
        		break;

			// TFT & TV
        	case MSG_DISPLAY_TASK_WAV_TIME_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_WAV_TIME);
        		#endif
        		break;       		
        	case MSG_DISPLAY_TASK_JPEG_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_JPEG);
        		#endif
        		break;
        	case MSG_DISPLAY_TASK_MJPEG_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_MJPEG);
        		#endif
        		break;
				
        	case MSG_DISPLAY_TASK_SETTING_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
				ap_display_setting_frame_buff_set(msg_id & 0xFFFFFF);
				#endif
        		break;
        	case MSG_DISPLAY_TASK_SETTING_EXIT:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_setting_frame_buff_set(NULL);
        		#endif
        		break;
        	case MSG_DISPLAY_TASK_USB_SETTING_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_buff_copy_and_draw(msg_id & 0xFFFFFF, DISPLAY_BUFF_SRC_USB_SETTING);
        		#endif
        		break;
        	case MSG_DISPLAY_TASK_ICON_SHOW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_icon_sts_set(msg_id);
        		#endif
        		break;
        	case MSG_DISPLAY_TASK_ICON_CLEAR:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_icon_sts_clear(msg_id);
        		#endif
        		break;
//        	case MSG_DISPLAY_TASK_MD_ICON_SHOW:
//        		ap_display_icon_sts_clear(ICON_MD_STS_0 | (ICON_MD_STS_1<<8) | (ICON_MD_STS_2<<16));
//        		ap_display_icon_sts_clear(ICON_MD_STS_3 | (ICON_MD_STS_4<<8) | (ICON_MD_STS_5<<16));
//        		ap_display_icon_sts_set(msg_id);
//        		break;
        	case MSG_DISPLAY_TASK_ICON_MOVE:
//        		ap_display_icon_move((DISPLAY_ICONMOVE *) (msg_id & 0xFFFFFF));
        		break;
        	case MSG_DISPLAY_TASK_PIC_EFFECT:
//        		ap_display_effect_sts_set(DISPLAY_PIC_EFFECT);
        		break;
        	case MSG_DISPLAY_TASK_PIC_PREVIEW_EFFECT:
//        		ap_display_effect_sts_set(DISPLAY_PIC_PREVIEW_EFFECT);
        		break;
        	case MSG_DISPLAY_TASK_PIC_EFFECT_END:
//        		ap_display_video_preview_end();
        		break;
         	case MSG_DISPLAY_TASK_LEFT_REC_TIME_DRAW:
         		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_left_rec_time_draw((INT32U) (msg_id & 0xFFFFFF), 1);
        		#endif
        		break;
         	case MSG_DISPLAY_TASK_LEFT_REC_TIME_CLEAR:
         		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_left_rec_time_draw(0, 0);
        		#endif
        		break;         		        	
        	case MSG_DISPLAY_TASK_STRING_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_string_draw((STR_ICON *) (msg_id & 0xFFFFFF));
        		#endif
        		break;
        	case MSG_DISPLAY_TASK_STRING_ICON_DRAW:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_string_icon_start_draw((STR_ICON_EXT *) (msg_id & 0xFFFFFF));
        		#endif
        		break;
        	case MSG_DISPLAY_TASK_STRING_ICON_CLEAR:
        		#if GPDV_BOARD_VERSION != GPCV1237A_Aerial_Photo
        		ap_display_string_icon_clear((STR_ICON_EXT *) (msg_id & 0xFFFFFF));
        		#endif
        		break;
        	default:
        		#if TV_DET_ENABLE   // for tv out
        		ap_display_buff_copy_and_draw(msg_id, DISPLAY_BUFF_SRC_SENSOR);
        		#endif   // end for tv out
        		break;
        }
    }
}
#endif