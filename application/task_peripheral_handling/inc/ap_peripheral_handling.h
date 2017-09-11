#include "task_peripheral_handling.h"


typedef void (*KEYFUNC)(INT16U *tick_cnt_ptr);

typedef struct
{
	KEYFUNC key_function;
	KEYFUNC fast_key_fun;
	INT16U key_io;
	INT16U key_cnt;	
	INT16U key_active;
	INT16U long_flag;
}KEYSTATUS;
extern void ap_peripheral_time_set(void);
extern void ap_peripheral_init(void);
extern void ap_peripheral_key_judge(void);
extern void ap_peripheral_adaptor_out_judge(void);
extern void ap_peripheral_key_register(INT8U type);
extern void ap_peripheral_gsensor_data_register(void );
extern void ap_peripheral_motion_detect_judge(void);
extern void ap_peripheral_motion_detect_start(void);
extern void ap_peripheral_motion_detect_stop(void);
extern void ap_peripheral_charge_det(void);
//extern INT8U  switch_state_get(void);
#if C_SCREEN_SAVER == CUSTOM_ON
extern void ap_peripheral_lcd_backlight_set(INT8U type);
#endif
//extern void ap_peripheral_night_mode_set(INT8U type);

extern void ap_TFT_backlight_tmr_check(void);
extern void ap_peripheral_tv_detect(void);
extern void ap_peripheral_read_gsensor(void);

extern void ap_peripheral_ad_key_judge(void);
extern void ap_peripheral_battery_check_calculate(void);
extern void ap_peripheral_config_store(void);


extern INT8U ap_video_record_sts_get(void);

extern void ap_peripheral_hdmi_detect(void);
extern void ap_peripheral_clr_screen_saver_timer(void);

extern void LED_pin_init(void);
extern void set_led_mode(LED_MODE_ENUM mode);
extern void LED_blanking_isr(void);
extern void led_red_on(void);
extern void led_green_on(void);
extern void led_all_off(void);
extern void led_green_off(void);
extern void led_red_off(void);

#if TV_DET_ENABLE
extern INT8U tv_plug_status_get(void);
#endif

