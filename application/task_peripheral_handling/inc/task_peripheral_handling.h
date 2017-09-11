#include "application.h"

#define PERIPHERAL_TASK_QUEUE_MAX			256///160///50///192
#define PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN	10


#define PERI_TIME_INTERVAL_AD_DETECT	2		//128 = 1s
//#define PERI_TIME_INTERVAL_KEY_DETECT	16		//128 = 1s
#define Long_Single_width              96
#define Short_Single_width              6

#define PERI_COFING_STORE_INTERVAL      5*128/PERI_TIME_INTERVAL_AD_DETECT //5s

#define PERI_TIME_LED_FLASH_ON			64/PERI_TIME_INTERVAL_AD_DETECT	// 0.5s
#define PERI_TIME_LED_BLINK_ON			64/PERI_TIME_INTERVAL_AD_DETECT	// 0.5s
#define PERI_TIME_LED_BLINK_OFF			64/PERI_TIME_INTERVAL_AD_DETECT	// 0.5s

#define PERI_POWER_OFF                  120*128/PERI_TIME_INTERVAL_AD_DETECT//2MIN

#define PERI_TIME_BACKLIGHT_DELAY		16	//128 = 1s

#define PERI_ADP_OUT_PWR_OFF_TIME       1*128/PERI_TIME_INTERVAL_AD_DETECT //15s	//wwj modify from 15sec to 5sec
#define PERI_USB_PHY_SUSPEND_TIME       4*128/PERI_TIME_INTERVAL_AD_DETECT

extern MSG_Q_ID PeripheralTaskQ;
extern void ap_peripheral_time_set(void);

extern void ap_peripheral_init(void);
extern void task_peripheral_handling_entry(void *para);
extern void ap_peripheral_key_judge(void);
extern void ap_peripheral_adaptor_out_judge(void);
extern void ap_peripheral_key_register(INT8U type);
extern void ap_peripheral_gsensor_data_register(void );
extern void ap_peripheral_motion_detect_judge(void);
extern void ap_peripheral_motion_detect_start(void);
extern void ap_peripheral_motion_detect_stop(void);
#if C_SCREEN_SAVER == CUSTOM_ON
extern void ap_peripheral_lcd_backlight_set(INT8U type);
#endif
//extern void ap_peripheral_night_mode_set(INT8U type);

extern void ap_TFT_backlight_tmr_check(void);
extern void ap_peripheral_tv_detect(void);
extern void ap_peripheral_read_gsensor(void);

extern void ap_peripheral_ad_key_judge(void);
extern INT8U usbd_exit;
extern INT8U screen_saver_enable;
extern void ap_peripheral_config_store(void);


extern void ap_peripheral_hdmi_detect(void);

extern void task_peripheral_handling_init(void);
extern void ap_peripheral_charge_det(void);
//extern INT8U  switch_state_get(void);

