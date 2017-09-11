#ifndef __MY_AVI_ENCODER_STATE_H__
#define __MY_AVI_ENCODER_STATE_H__

#include "application.h"
#include "avi_encoder_app.h"
#include "video_encoder.h"

#include "my_video_codec_callback.h"
#include "my_encoder_jpeg.h"

/****************************************************************************/
#define AVI_ENCODER_STATE_IDLE       0x00000000
#define AVI_ENCODER_STATE_VIDEO      0x00000001
#define AVI_ENCODER_STATE_SENSOR     0x00000002
#define AVI_ENCODER_STATE_PACKER0	 0x00000004

//extern INT32U jpeg_out_buffer_large_size;
//extern INT32U jpeg_out_buffer_middle_size;
//extern INT32U jpeg_out_buffer_small_size;

// Video Record buffer alloc
//extern INT32U jpeg_out_buffer_large_cnt;
//extern INT32U jpeg_out_buffer_middle_cnt;
//extern INT32U jpeg_out_buffer_small_cnt;

//extern INT32U jpeg_out_1080p_large_cnt;
//extern INT32U jpeg_out_1080p_middle_cnt;
//extern INT32U jpeg_out_1080p_small_cnt;

// WebCam buffer allc
#define JPEG_UVC_BUFFER_LARGE_CNT 	 	4
#define JPEG_UVC_BUFFER_MIDDLE_CNT 	 	0
#define JPEG_UVC_BUFFER_SMALL_CNT		0

// Video Record buffer alloc
#define  JPEG_OUT_BUFFER_LARGE_CNT	0

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

//+++ Jpeg
#define RECORDING_JPEG_VLC_BUFFER_CNT		80///90
///#define JPEG_VLC_BUFFER_CNT		90
#define PHOTO_JPEG_VLC_BUFFER_CNT	3
///#define JPEG_VLC_BUFFER_SIZE	64*1024
#define RECORDING_JPEG_VLC_BUFFER_SIZE	16*1024
#define PHOTO_JPEG_VLC_BUFFER_SIZE	64*1024


#define UVC_JPEG_VLC_BUFFER_CNT		8
#define UVC_JPEG_VLC_BUFFER_SIZE	150*1024

#define JPEG_Q_MAX_CNT  8

#define  JPEG_OUT_BUFFER_CNT_MAX  RECORDING_JPEG_VLC_BUFFER_CNT//(JPEG_OUT_BUFFER_LARGE_CNT+JPEG_OUT_BUFFER_MIDDLE_CNT+JPEG_OUT_BUFFER_SMALL_CNT)

typedef struct jpeg_Q_args_s
{
	INT32U jpeg_buffer_addrs;
	INT32U jpeg_fifo_msg;
	INT32U jpeg_fifo_idx;
}jpeg_Q_args_t;

typedef struct jpeg_comress_args_s
{
	jpeg_Q_args_t jpeg_Q_array[JPEG_Q_MAX_CNT];
	INT32U jpeg_input_addrs;
	INT32U jpeg_input_size;
	AVIPACKER_FRAME_INFO jpeg_output_addrs[JPEG_OUT_BUFFER_CNT_MAX];
	INT32U jpeg_img_width;
	INT32U jpeg_img_height;
	INT32U jpeg_isr_status_flag;
	INT32U jpeg_vlc_size;
	INT32U jpeg_engine_status;
	void   (*jpeg_status_notify)(INT32S eventMsg, INT32U vlcCount);
	INT32U jpeg_file_handle;
	INT32U jpeg_compress_fifo_idx;
	INT8U  jpeg_img_format;
	INT8U  jpeg_output_addrs_idx;
	INT8U  jpeg_send_to_target;
	INT8U  jpeg_enable_scaler_up_func_flag; 
	INT8U  jpeg_skip_this_one_flag;
	INT8U  jpeg_input_empty_count;
	INT8U  jpeg_input_empty_count_max;
	INT32U jpeg_output_full_count;
}jpeg_comress_args_t;


#define ENABLE_DYNAMIC_TUNING_JPEG_Q	1
#define AVI_Y_Q_VALUE_VIDEO_720P		50
#define AVI_Y_Q_VALUE_VIDEO_1080FHD		40
#define AVI_UV_Q_VALUE_VIDEO_1080FHD	30

/****************************************************************************/
extern INT32S my_avi_encode_state_task_create(INT8U pori);
extern INT32S my_avi_encode_state_task_del(void);

extern void avi_encoder_state_set(INT32U stateSetting);
extern INT32U avi_encoder_state_get(void);
extern void avi_encoder_state_clear(INT32U stateSetting);
extern void avi_encode_stop(void);
extern void Dynamic_Tune_Q(INT32U jpeg_size, INT32U full_size_flag);
extern jpeg_comress_args_t gJpegCompressArgs;
/****************************************************************************/
#endif		// __MY_AVI_ENCODER_STATE_H__
/****************************************************************************/
