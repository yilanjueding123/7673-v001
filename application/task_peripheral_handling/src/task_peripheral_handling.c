
#include "task_peripheral_handling.h"
#include "ap_state_config.h"
#include "ap_browse.h"
#include "ap_display.h"

INT8U			usb_charge_cnt = 0;
MSG_Q_ID		PeripheralTaskQ;
void *			peripheral_task_q_stack[PERIPHERAL_TASK_QUEUE_MAX];
static INT8U	peripheral_para[PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN];
INT8U			screen_saver_enable = 0;
INT32U			battery_charge_icon_blink_cnt = 0;
INT32U			battery_low_blink_cnt = 0;
INT32U			display_insert_sdc_cnt = 0;
INT32U			motion_detect_peripheral_cnt = 0;
INT32U			G_sensor_power_on_time = 0;
INT32U			sensor_error_power_off_timer = 0;
INT16U			usb_dete_cnt = 0;
INT16U			shutoff_cnt = 0;
INT16U			zd_cnt = 0;
INT16U			lowbatt_shut_cnt = 0;

#if TV_DET_ENABLE
extern INT8U	tv_debounce_cnt;

#endif

extern INT8U	PreviewIsStopped;
extern volatile INT8U cdsp_hangup_flag;
extern INT8U	adp_status;
extern INT32U gp_ae_awb_process(void);
extern void gp_isp_iso_set(INT32U iso);
extern void set_led_mode(LED_MODE_ENUM mode);
static INT8U	watch_dog_cnt = 0;
extern INT8U	gc_ad_init_done;
extern INT8U usb_state_get(void);
extern void led_red_on(void);
extern void led_green_on(void);
extern void led_all_off(void);
extern void led_green_off(void);
extern void led_red_off(void);


void task_peripheral_handling_init(void)
{
	PeripheralTaskQ 	= msgQCreate(PERIPHERAL_TASK_QUEUE_MAX, PERIPHERAL_TASK_QUEUE_MAX,
		 PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN);
	ap_peripheral_init();
}


INT8U			card_space_less_flag = 0;
extern volatile INT8U DayLight_mode;
extern INT8S video_record_sts;


void task_peripheral_handling_entry(void * para)
{
	INT32U			msg_id, power_on_auto_rec_delay_cnt;
	INT8U			usb_detect_start;

#if TV_DET_ENABLE // for tv out

	INT8U			tick = 0, tv_polling_start = 0;

#endif // end for tv out

	INT8U			aeawb_start;
	INT8S			ret = 0, t_flash_flag = 0;
	INT8U			volume_show_time;

	aeawb_start 		= 1;
	volume_show_time	= 0;
	usb_detect_start	= 0;

	while (1)
	{
		if (msgQReceive(PeripheralTaskQ, &msg_id, peripheral_para, PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN) == STATUS_FAIL)
		{
			continue;
		}

		if (++watch_dog_cnt >= 128)
		{
			watch_dog_cnt		= 0;
			//DBG_PRINT("-----CLEAR WATCH_DOG-----\r\n");
			watchdog_clear();
		}

		//	DBG_PRINT("Handling_message_receive\r\n");
		switch (msg_id)
		{
			case MSG_PERIPHERAL_TASK_KEY_REGISTER:
				ap_peripheral_key_register(peripheral_para[0]);
				break;

			case MSG_PERIPHERAL_TASK_USBD_DETECT_INIT:
				usb_detect_start = 1;
				power_on_auto_rec_delay_cnt = 2 * 128 / PERI_TIME_INTERVAL_AD_DETECT; //2s				
				break;

			case MSG_PERIPHERAL_TASK_LED_SET:
				//ap_peripheral_led_set(peripheral_para[0]);
				set_led_mode(peripheral_para[0]);

				if (peripheral_para[0] == LED_CARD_NO_SPACE)
					card_space_less_flag = 1;

				break;

			case MSG_PERIPHERAL_TASK_LED_FLASH_SET:
				//ap_peripheral_led_flash_set();
				break;

			case MSG_PERIPHERAL_TASK_LED_BLINK_SET:
				//ap_peripheral_led_blink_set();
				break;

			case MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_START:
				battery_charge_icon_blink_cnt = 0x002f;
				break;

			case MSG_PERIPHERAL_TASK_BATTERY_CHARGE_ICON_BLINK_STOP:
				battery_charge_icon_blink_cnt = 0;
				break;

			case MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_START:
				battery_low_blink_cnt = 0x001f;
				battery_low_blink_cnt |= 0x0a00;
				break;

			case MSG_PERIPHERAL_TASK_BATTERY_LOW_BLINK_STOP:
				if (battery_low_blink_cnt)
				{
					battery_low_blink_cnt = 0;
				}

				break;

			case MSG_PERIPHERAL_TASK_DISPLAY_PLEASE_INSERT_SDC:
				display_insert_sdc_cnt = 0x006f;
				break;

			case MSG_PERIPHERAL_TASK_TIMER_VOLUME_ICON_SHOW:
				volume_show_time = peripheral_para[0] *20;
				break;

			case MSG_PERIPHERAL_TASK_G_SENSOR_POWER_ON_START:
				G_sensor_power_on_time = 0x0300;
				break;

			case MSG_PERIPHERAL_TASK_LOWBATT_SHUT:
				lowbatt_shut_cnt = 201;
				break;

			case MSG_PERIPHERAL_TASK_SHUTOFF_SET:
				if (peripheral_para[0] == 2)
					shutoff_cnt = 201;
				else if (peripheral_para[0] == 1)
					shutoff_cnt = 361;

				break;

			case MSG_PERIPHERAL_TASK_TIME_SET:
				ap_peripheral_time_set();
				break;

#if USE_ADKEY_NO
			case MSG_PERIPHERAL_TASK_AD_DETECT_CHECK:
				ap_peripheral_key_judge();

				if ((s_usbd_pin) || (usb_state_get()))
				{
					if (++usb_charge_cnt > 31)
					{
						ap_peripheral_charge_det();
						usb_charge_cnt		= 0;
					}
				}

#if 1
				ap_peripheral_ad_key_judge();
#endif

#if GPDV_BOARD_VERSION							!= GPCV1237A_Aerial_Photo
				ap_peripheral_config_store();
#endif

				if (usb_detect_start == 1)
				{
					ap_peripheral_adaptor_out_judge();

#if GPDV_BOARD_VERSION								!= GPCV1237A_Aerial_Photo
					ap_peripheral_hdmi_detect();
#endif
					if (power_on_auto_rec_delay_cnt)
					{
						power_on_auto_rec_delay_cnt -= 2;

						if (!power_on_auto_rec_delay_cnt)
						{
							if(usb_state_get() != 1)
							{
							   if(ap_state_handling_storage_id_get()!= NO_STORAGE)
							  		msgQSend(ApQ, MSG_APQ_POWER_ON_AUTO_RECORD, NULL, NULL, MSG_PRI_NORMAL);
							}
						}
					}

					if (cdsp_hangup_flag)
					{ //10s
						PreviewIsStopped	= 1;
						DBG_PRINT("too many cdsp overflow!!! Auto Power Off...\r\n");
						msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_URGENT);
					}
					else if (PreviewIsStopped == 0)
					{
						if (sensor_error_power_off_timer > 10 * 128 / PERI_TIME_INTERVAL_AD_DETECT)
						{ //10s auto power off
							PreviewIsStopped	= 1;
							DBG_PRINT("no scaler interrup occurs!!! Auto Power Off...\r\n");
							msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_URGENT);
						}
					}
				}

#if C_MOTION_DETECTION							== CUSTOM_ON

				if (motion_detect_peripheral_cnt)
				{
					motion_detect_peripheral_cnt--;

					if (!motion_detect_peripheral_cnt)
					{
						ap_peripheral_motion_detect_judge();
						motion_detect_peripheral_cnt = 2;
					}
				}

#endif

				if (usb_dete_cnt)
				{
					usb_dete_cnt--;

					if (!usb_dete_cnt)
					{
						//if(usb_state_get() == 2)
						//	msgQSend(ApQ, MSG_APQ_QUICK_RECORD, NULL, NULL, MSG_PRI_NORMAL);
					}
				}

#if 0

				if (lowbatt_shut_cnt)
				{
					lowbatt_shut_cnt--;

					if (usb_state_get() != 0)
					{
						lowbatt_shut_cnt	= 0;
					}

					if (!lowbatt_shut_cnt)
					{
						if (usb_state_get() == 0)
						{
							msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
						}
					}
					else 
					{
						if (lowbatt_shut_cnt % 10 == 0)
						{
							if (t_flash_flag == 0)
							{
								t_flash_flag		= 1;
								led_red_on();
							}
							else 
							{
								//led_red_on();
								led_red_off();
								t_flash_flag		= 0;
							}
						}
					}
				}

#endif

				if (shutoff_cnt)
				{
					shutoff_cnt--;

					if (usb_state_get() != 0)
					{
						//led_green_off();
						shutoff_cnt 		= 0;
						DBG_PRINT("fankun5:%d\r\n", usb_state_get());
					}

					if (!shutoff_cnt)
					{
						if (usb_state_get() == 0)
						{
							DBG_PRINT("fankun1:%d\r\n", usb_state_get());
							msgQSend(ApQ, MSG_APQ_POWER_KEY_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
						}
					}
					else 
					{
						if (shutoff_cnt % 20 == 0)
						{
							if (t_flash_flag == 0)
							{
								t_flash_flag		= 1;
								led_all_off();
							}
							else 
							{
								led_green_on();
								t_flash_flag		= 0;
							}
						}
					}

				}

				if (aeawb_start)
				{
					ret 				= gp_ae_awb_process();

					if (ret != 0)
					{
						//DBG_PRINT(".");
					}
					else 
					{
						//DBG_PRINT("\r\nae, awb process done!! \r\n");
						//DBG_PRINT("*");
					}
				}
				if (video_record_sts & 0x06)
				{
					if (DayLight_mode)
					{
						gpio_write_io(LED_FLOOD_LIGHT, DATA_HIGH);
					}
					else 
					{
						gpio_write_io(LED_FLOOD_LIGHT, DATA_LOW);
					}
				}
				else 
				{
					gpio_write_io(LED_FLOOD_LIGHT, DATA_LOW);
				}
				
				break;

#endif

#if TV_DET_ENABLE // for tv out

			case MSG_PERIPHERAL_TV_POLLING_START:
				tv_polling_start = 1;

#if TV_DET_ENABLE
				tv_debounce_cnt = 0;
#endif

				break;

			case MSG_PERIPHERAL_TV_POLLING_STOP:
				tv_polling_start = 0;

#if TV_DET_ENABLE
				tv_debounce_cnt = 0;
#endif

				break;

			case MSG_PERIPHERAL_TASK_TV_DETECT:
				ap_peripheral_tv_detect();
				break;

#endif // end for tv out

#if GPDV_BOARD_VERSION						!= GPCV1237A_Aerial_Photo
			case MSG_PERIPHERAL_TASK_READ_GSENSOR:
				ap_peripheral_read_gsensor();
				break;

#endif

#if C_MOTION_DETECTION						== CUSTOM_ON

			//			case MSG_PERIPHERAL_TASK_MOTION_DETECT_JUDGE:
			//				ap_peripheral_motion_detect_judge();
			//				break;
			case MSG_PERIPHERAL_TASK_MOTION_DETECT_START:
				ap_peripheral_motion_detect_start();
				motion_detect_peripheral_cnt = 32;
				break;

			case MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP:
				ap_peripheral_motion_detect_stop();
				motion_detect_peripheral_cnt = 0;
				break;

			case MSG_PERIPHERAL_TASK_MOTION_DETECT_DELAY:
				motion_detect_peripheral_cnt = 0xff;
				break;

#endif

#if C_SCREEN_SAVER							== CUSTOM_ON
			case MSG_PERIPHERAL_TASK_LCD_BACKLIGHT_SET:
				ap_peripheral_clr_screen_saver_timer();
				ap_peripheral_lcd_backlight_set(peripheral_para[0]);
				break;

			case MSG_PERIPHERAL_TASK_SCREEN_SAVER_ENABLE:
				screen_saver_enable = 1;
				break;

#endif

			case MSG_PERIPHERAL_TASK_NIGHT_MODE_SET:
				//ap_peripheral_night_mode_set(peripheral_para[0]);
				break;

			case MSG_PERIPHERAL_USBD_EXIT:
				usbd_exit = 1;
				break;

			case MSG_PERIPHERAL_TASK_ISP_ISO_SET:
				gp_isp_iso_set(peripheral_para[0]);
				break;

			default:
				break;
		}
	}
}


