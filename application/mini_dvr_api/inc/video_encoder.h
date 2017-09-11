#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include "avi_encoder_app.h"

#include "my_avi_encoder_state.h"

/****************************************************************************/

//#define JPEG_OUT_BUFFER_LARGE_SIZE	(250*1024)	// It will influence UVC
//#define JPEG_OUT_BUFFER_MIDDLE_SIZE	(150*1024)
//#define JPEG_OUT_BUFFER_SMALL_SIZE	(120*1024)

// Video Record buffer alloc
//#define  JPEG_OUT_BUFFER_LARGE_CNT	0

/*
#if TV_DET_ENABLE   // for tv out
#define JPEG_OUT_BUFFER_MIDDLE_CNT	8
#else
#define JPEG_OUT_BUFFER_MIDDLE_CNT	8
#endif   // end for tv out

#if (USE_PANEL_NAME == PANEL_T43)
#define JPEG_OUT_BUFFER_SMALL_CNT	0
#else
#define JPEG_OUT_BUFFER_SMALL_CNT	0
#endif
#define JPEG_OUT_1080P_LARGE_CNT		0
#define JPEG_OUT_1080P_MIDDLE_CNT		9
#define JPEG_OUT_1080P_SMALL_CNT		0
*/

#define  JPEG_OUT_BUFFER_CNT_MAX  RECORDING_JPEG_VLC_BUFFER_CNT//(JPEG_OUT_BUFFER_LARGE_CNT+JPEG_OUT_BUFFER_MIDDLE_CNT+JPEG_OUT_BUFFER_SMALL_CNT)

void video_encode_entrance(void);
void video_encode_exit(void);
extern INT8U ap_state_config_voice_record_switch_get(void);
extern void ap_peripheral_gsensor_data_register(void );

//extern INT32U time_stamp_buffer[];
//extern INT32U g_JPEG_OUT_BUFFER_CNT_MAX;

CODEC_START_STATUS video_encode_preview_start(VIDEO_ARGUMENT arg);
CODEC_START_STATUS video_encode_preview_stop(void);
CODEC_START_STATUS video_encode_start(MEDIA_SOURCE src);
CODEC_START_STATUS video_encode_fast_stop_and_start(MEDIA_SOURCE src);
CODEC_START_STATUS video_encode_stop(void);
CODEC_START_STATUS video_encode_auto_switch_csi_frame(void);
CODEC_START_STATUS video_encode_auto_switch_csi_fifo_end(INT8U flag);
CODEC_START_STATUS video_encode_auto_switch_csi_frame_end(INT8U flag);
CODEC_START_STATUS video_encode_set_zoom_scaler(FP32 zoom_ratio);
CODEC_START_STATUS video_encode_capture_picture(MEDIA_SOURCE src);
CODEC_START_STATUS video_encode_fast_switch_stop_and_start(MEDIA_SOURCE src);
#endif
