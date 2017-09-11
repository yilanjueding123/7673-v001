#include "ap_music.h"
#include "ap_state_config.h"
#include "ap_startup.h"
#include "drv_l1_tft.h"
#include "drv_l2_spi_flash.h"
#include "avi_encoder_app.h"



extern void ap_video_capture_mode_switch(INT8U DoSensorInit, INT16U EnterAPMode);

void ap_startup_init(void)
{
	video_encode_entrance();


	   ap_video_capture_mode_switch(1,STATE_VIDEO_RECORD); // 開機要做sensor init
   
}