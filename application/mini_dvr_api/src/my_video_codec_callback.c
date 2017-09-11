#include "my_video_codec_callback.h"

//+++ GuanYu
#include "drv_l1_wrap.h" 
#include "drv_l1_scaler.h"
#include "drv_l1_conv422to420.h"
#include "drv_l1_conv420to422.h"
#include "avi_encoder_app.h"
#include "drv_l2_sensor.h"
#include "ap_state_config.h"
#include "application.h"
#include "ap_state_resource.h"
//---

/****************************************************************************/
//+++ Zoom
/*
#define DO_ZOOM_NOTHING	0
#define DO_ZOOM_IN 		1
#define DO_ZOOM_OUT		2
*/
//---

//+++ Video Recording
#define VIDEO_RECORD_NOTHING	0
#define VIDEO_RECORD_START		1
#define VIDEO_RECORD_RECING		2
#define VIDEO_RECORD_STOP		3
#define VIDEO_RECORD_STOP1		4
//---

//+++ Scaler
typedef enum
{
    SCALER_BUFFER_IDX_A,                     
    SCALER_BUFFER_IDX_B,
    SCALER_BUFFER_IDX_MAX
} SCALER_BUFFER_IDX;

/****************************************************************************/
//static INT8U zoomInOutFlag = DO_ZOOM_NOTHING;

static volatile INT8U videoRecordFlag = VIDEO_RECORD_NOTHING;
static volatile INT8U wantToStopPreviewFlag = 0;

INT8U PreviewIsStopped = 0;
INT8U PreviewInitDone = 0;

static INT8U protect_recover_flag = 0;
static INT32U protect_recover_cnt = 0;
static INT32U fifo_addr_bak0 = 0;
static INT32U fifo_addr_bak1 = 0;

static INT8U wantToPreview2DummyFlag = 0;
static INT8U DoBlackEdgeFlag = 0;
static INT32U ZOOM_WIDTH_INTERVAL = 0;
static INT32U ZOOM_HEIGHT_INTERVAL = 0;

preview_args_t gGpreviewArgs = {0};

/****************************************************************************/
//+++ Zoom
/*typedef struct zoom_args_s
{
	sensor_frame_range_t sensor_clip_range;
	INT32U source_width;
	INT32U source_height;
	INT32U scaler_factor_w;
	INT32U scaler_factor_h;	
	INT32U zoom_step;
}zoom_args_t;

zoom_args_t gZoomArgs = {0};*/

//+++ Conv422to420
typedef struct conv422_args_s
{
	INT32U conv422_buffer_A;
	INT32U conv422_buffer_B;
	INT32U conv422_buffer_next;
	INT32U conv422_fifo_total_count;
	INT32U conv422_fifo_line_len;
	INT32U conv422_fifo_data_size;
	INT32U (*conv422_fifo_ready_notify)(INT32U fifoMsg, INT32U fifoAddrs ,INT32U fifoIdx);	
	INT32U (*conv422_fifo_buffer_addrs_get)(void);
	INT32S (*conv422_fifo_buffer_addrs_put)(INT32U addr);
	INT8U  conv422_output_format;
	INT8U  conv422_isr_count_max;
	INT8U  conv422_isr_count;
	INT8U  conv422_fifo_path;
}conv422_args_t;

conv422_args_t gConv422Args = {0};

//+++ scale up
#if 0
typedef struct scale_up_args_s
{
       INT32U scaler_in_buffer_A;
	INT32U scaler_in_buffer_B;
	INT32U scaler_buffer_A;
	INT32U scaler_buffer_B;
	INT32U scaler_buffer_size;
	INT32U (*scaler_ready_notify)(INT32U fifoMsg, INT32U fifoAddrs);
	INT8U  scaler_buffer_idx;
	INT8U  scaler_1st_buffer_notify;
	INT8U  scaler_buffer_remaining_idx;
	INT8U  scaler_engine_status;
	INT8U  scaler_in_buffer_idx;
}scale_up_args_t;
#endif
scale_up_args_t gScalupArgs = {0};

//+++ sensor
sensor_apis_ops* pSensorApiOps;

INT8U lastFIFOIdx = 0;
INT8U showOnTVFlag;

INT32U cdsp_overflow_count = 0;
volatile INT8U cdsp_hangup_flag = 0;
INT8U gDo_Scale_Up = 0;

extern INT32U sensor_error_power_off_timer;
extern INT32U display_isr_queue[];

#if TV_DET_ENABLE
static void sensor_display_update(void);
#endif

#define bit(n) (1UL << (n))
#define DateTimeStampBufWidth	45 // Unit: Byte 
#define DateTimeStampBufSize	(DateTimeStampBufWidth*40)
INT8U  DateTimeStampBuf[DateTimeStampBufSize];
TIME_T	DateTimeStamp = 0;
INT8U   DateTimeFormatFlag = 0;
INT8U   DateTimeInitFlag = 0;

extern const unsigned char *number[];

typedef struct date_time_byte_offset_s
{
	INT32U Year_Ones;
	INT32U Year_Tens;
	INT32U Year_Hundreds;
	INT32U Year_Thousands;
	INT32U Month_Ones;
	INT32U Month_Tens;
	INT32U Day_Ones;
	INT32U Day_Tens;	
	INT32U Forward_Slash_1;
	INT32U Forward_Slash_2;
}date_time_byte_offset_t;

date_time_byte_offset_t DateTimeByteOffset = 0;

/****************************************************************************/
/*
 *	cpu_draw_time_osd : 
 */
 
void DateTimeDraw(INT32U targetBuf,STRING_INFO strInfo,INT32U showTimeStamp,INT8U drawFormat)
{
	INT8U* strDateTimeBuf;
	INT32U strWidth,strHeight;
	INT32U strWidthLoop;
	INT8U  strDateTime;
	INT16U* drawTargetBuf = (INT16U*)0xF8500000;
	INT32U startWidth;
	INT32U startHeight;
	INT32U targetBufWidth;
	INT32U jumpByteAddrs;
	INT32U startHeightStart;
	INT32U startHeightEnd;
	INT8U* drawTargetBufByte = (INT8U*)0xF8500000;
	INT32U oddOffsetByteCount;
	INT32U evenOffsetByteCount;
	INT32U width15Size;

	startWidth = strInfo.pos_x;
	startHeight = strInfo.pos_y;
	targetBufWidth = strInfo.buff_w;
	startHeightStart = strInfo.font_offset_h_start;
	startHeightEnd = strInfo.font_offset_h_width;

	if(showTimeStamp == DRAW_DATE_TIME)
	{
		strWidthLoop = DateTimeStampBufWidth;
	}
	else // 丁材 24Byte 
	{
		strWidthLoop = 24;		
	}

	{
		jumpByteAddrs = (DateTimeStampBufWidth-strWidthLoop);

		strDateTimeBuf = (DateTimeStampBuf+(startHeightStart*DateTimeStampBufWidth));
	}
	
	if(drawFormat == YUV420_DRAW)
	{
		oddOffsetByteCount = ((startHeight*targetBufWidth*3>>1) + (targetBufWidth<<1) + startWidth);
		evenOffsetByteCount = ((startHeight*targetBufWidth*3>>1) + startWidth);
		width15Size = ((targetBufWidth*3)>>1);
		for(strHeight=0; strHeight<startHeightEnd; strHeight++)
		{
			if((startHeight+strHeight) & 0x01)
			{
				drawTargetBufByte = (((INT8U*)targetBuf)+(((strHeight-1)*width15Size) + oddOffsetByteCount));
			}
			else
			{
				drawTargetBufByte = (((INT8U*)targetBuf)+((strHeight*width15Size) + evenOffsetByteCount));
			}

			for(strWidth=strWidthLoop; strWidth>0; strWidth--,drawTargetBufByte+=8)
			{
				strDateTime = *(strDateTimeBuf++);
				if(strDateTime == 0)
				{
					continue;
				}

				if(strDateTime & 0xF0)
				{
					if(strDateTime & 0x80)
					{
						*drawTargetBufByte = 0xFF;
					}

					if(strDateTime & 0x40)
					{
						*(drawTargetBufByte+1) = 0xFF;
					}

					if(strDateTime & 0x20)
					{
						*(drawTargetBufByte+2) = 0xFF;
					}
			
					if(strDateTime & 0x10)
					{
						*(drawTargetBufByte+3) = 0xFF;
					}
				}

				if(strDateTime & 0x0F)
				{							
					if(strDateTime & 0x08)
					{
						*(drawTargetBufByte+4) = 0xFF;
					}		
			
					if(strDateTime & 0x04)
					{
						*(drawTargetBufByte+5) = 0xFF;
					}
				
					if(strDateTime & 0x02)
					{
						*(drawTargetBufByte+6) = 0xFF;
					}
					
					if(strDateTime & 0x01)
					{
						*(drawTargetBufByte+7) = 0xFF;
					}
				}
			}
			
			strDateTimeBuf += jumpByteAddrs;	
		}
	}
	else
	{
		evenOffsetByteCount = startWidth+(targetBufWidth*startHeight);

		for(strHeight=0; strHeight<startHeightEnd; strHeight++)
		{
			drawTargetBuf = (((INT16U*)targetBuf)+evenOffsetByteCount+(targetBufWidth*strHeight));

			for(strWidth=strWidthLoop; strWidth>0; strWidth--,drawTargetBuf+=8)
			{
				strDateTime = *(strDateTimeBuf++);
				if(strDateTime == 0)
				{
					continue;
				}

				if(strDateTime & 0xF0)
				{
					if(strDateTime & 0x80)
					{
						*drawTargetBuf = strInfo.font_color;
					}

					if(strDateTime & 0x40)
					{
						*(drawTargetBuf+1) = strInfo.font_color;
					}

					if(strDateTime & 0x20)
					{
						*(drawTargetBuf+2) = strInfo.font_color;
					}
			
					if(strDateTime & 0x10)
					{
						*(drawTargetBuf+3) = strInfo.font_color;
					}
				}

				if(strDateTime & 0x0F)
				{								
					if(strDateTime & 0x08)
					{
						*(drawTargetBuf+4) = strInfo.font_color;
					}		
			
					if(strDateTime & 0x04)
					{
						*(drawTargetBuf+5) = strInfo.font_color;
					}
				
					if(strDateTime & 0x02)
					{
						*(drawTargetBuf+6) = strInfo.font_color;
					}
					
					if(strDateTime & 0x01)
					{
						*(drawTargetBuf+7) = strInfo.font_color;
					}
				}
			}
			
			strDateTimeBuf += jumpByteAddrs;
		}
	}
}

void copybit(INT8U srcByte, INT8U* destByte, INT32U srcBitIdx, INT32U destBitIdx)
{
	if(srcByte & bit(srcBitIdx))
	{
		*destByte |= bit(destBitIdx);
	}
	else
	{
		*destByte &= ~bit(destBitIdx);	
	}
}

void DateTimeStringReplace(INT8U* dateTimeBuf,INT32U strChar,INT32U strWidth,INT32U bitOffset)
{
	//INT32U dataOffset;
	INT8U* dataContent;
	INT8U* strTargetBuffer;
	INT32U byteOffsetStart;
	INT32U bitOffsetStart;
	INT32U strWidthIdx,strHeightIdx;
	INT32U strByteWidth;
	INT8U  srcStrData;
	INT8U  destStrData;
	INT8U  needNBitToOneByte;
	
	//dataOffset = (INT32U) (&number[strChar - 0x2F]);
	//dataContent = (INT8U *) (((t_FONT_TABLE_STRUCT *) dataOffset)->font_content);
	dataContent = (INT8U *)number[strChar - 0x2F];

	strByteWidth = (strWidth/8);
	if((strWidth%8)!=0)
	{
		strByteWidth++;
	}

	byteOffsetStart = (bitOffset >> 3);
	bitOffsetStart = (bitOffset & 0x07);

	needNBitToOneByte = 0;

	//铬璶ゴ竚
	strTargetBuffer = (INT8U*)(dateTimeBuf+byteOffsetStart);

	for(strHeightIdx=0; strHeightIdx<40; strHeightIdx++)
	{		
		if(bitOffsetStart != 0)
		{
			needNBitToOneByte = (8-bitOffsetStart);
		}
	
		for(strWidthIdx=0; strWidthIdx<strWidth; strWidthIdx++)
		{		
			srcStrData = *(dataContent+(strWidthIdx >> 3)+(strByteWidth*strHeightIdx));

			//矪瞶玡计逞緇竚
			if(needNBitToOneByte != 0)
			{
				{
					destStrData = *(strTargetBuffer+(DateTimeStampBufWidth*strHeightIdx));
				}
			}
			else // 玡计逞緇竚癸霍挡
			{								
				if(((bitOffsetStart+strWidthIdx) & 0x07) == 0)
				{	
					// Target Buffer程Byteぃ籠筁戈,惠璶弄ㄓ矪瞶

					{
						destStrData = *(strTargetBuffer+((bitOffsetStart+strWidthIdx) >> 3)+(DateTimeStampBufWidth*strHeightIdx));					
					}					
				}			
			}

			// Resource Tool 眖蔼逼癬
			copybit(srcStrData,&destStrData,(0x07-(strWidthIdx & 0x07)),(0x07-((bitOffsetStart+strWidthIdx) & 0x07)));
			
			{
				*(strTargetBuffer+((bitOffsetStart+strWidthIdx) >> 3)+(DateTimeStampBufWidth*strHeightIdx)) = destStrData;
			}

			if(needNBitToOneByte != 0)
			{
				needNBitToOneByte--;
			}			
		}	
	}		
}
static volatile INT8U  fff_s=0;
static volatile INT8U  fff_cnt_s=0;

static volatile INT8U  fff_min=0;
static volatile INT8U  fff_cnt_min=0;

static volatile INT8U  fff_min_1=0;
static volatile INT8U  fff_cnt_min1=0;


static volatile INT8U  fff_hour=0;
static volatile INT8U  fff_cnt_hour=0;

static volatile INT8U  fff_hour_1=0;
static volatile INT8U  fff_cnt_hour1=0;

static INT8U  video_osd_state=0;
void DateTimeBufUpdate(void)
{
	TIME_T	current_system_time;
	INT32U  current_tm,dataTime_tm;
	INT32U  storeTime;
	INT8U   current_DateTimeFormatFlag;
	INT8U   repaintFlag;

	repaintFlag = 0;
	cal_time_get(&current_system_time);
	current_DateTimeFormatFlag = ap_state_config_data_time_mode_get(); 

	if((current_DateTimeFormatFlag != DateTimeFormatFlag) || (DateTimeInitFlag == 0))	
	{
		DateTimeInitFlag = 1;
		
  	 	gp_memset((INT8S*)DateTimeStampBuf, 0x00, DateTimeStampBufSize);
		DateTimeFormatFlag = current_DateTimeFormatFlag;
		repaintFlag = 1;

		switch(current_DateTimeFormatFlag)
		{
			// ら戳丁丁筳琌 1 pixel = 1 bit
			case 0:	// Y/M/D 
			default:
				DateTimeByteOffset.Year_Thousands = 0;
				DateTimeByteOffset.Year_Hundreds = 21;
				DateTimeByteOffset.Year_Tens = 42;
				DateTimeByteOffset.Year_Ones = 63;
				
				DateTimeByteOffset.Forward_Slash_1 = 84;
				DateTimeByteOffset.Month_Tens = 96;
				DateTimeByteOffset.Month_Ones = 117;
				DateTimeByteOffset.Forward_Slash_2 = 138;
				DateTimeByteOffset.Day_Tens = 150;
				DateTimeByteOffset.Day_Ones = 171;
			break;

			case 1: // D/M/Y
				DateTimeByteOffset.Day_Tens = 0;
				DateTimeByteOffset.Day_Ones = 21;
				DateTimeByteOffset.Forward_Slash_1 = 42;
				DateTimeByteOffset.Month_Tens = 54;
				DateTimeByteOffset.Month_Ones = 75;
				DateTimeByteOffset.Forward_Slash_2 = 96;
				DateTimeByteOffset.Year_Thousands = 108;
				DateTimeByteOffset.Year_Hundreds = 129;
				DateTimeByteOffset.Year_Tens = 150;
				DateTimeByteOffset.Year_Ones = 171;
			break;

			case 2: // M/D/Y
				DateTimeByteOffset.Month_Tens = 0;
				DateTimeByteOffset.Month_Ones = 21;
				DateTimeByteOffset.Forward_Slash_1 = 42;
				DateTimeByteOffset.Day_Tens = 54;
				DateTimeByteOffset.Day_Ones = 75;
				DateTimeByteOffset.Forward_Slash_2 = 96;
				DateTimeByteOffset.Year_Thousands = 108;
				DateTimeByteOffset.Year_Hundreds = 129;
				DateTimeByteOffset.Year_Tens = 150;
				DateTimeByteOffset.Year_Ones = 171;
			break;
		}
	}
	
	if((current_system_time.tm_sec != DateTimeStamp.tm_sec) || repaintFlag||fff_s)
	{
	if(fff_s)
		{
		 fff_cnt_s++;
		 if(fff_cnt_s<1)
		 	return;
		}
	    fff_cnt_s=0;
		storeTime = current_system_time.tm_sec;

		//+++
		current_tm = current_system_time.tm_sec/10;
		dataTime_tm = DateTimeStamp.tm_sec/10;
		if((current_tm != dataTime_tm) || repaintFlag||fff_s)
		{
		   if(video_osd_state){
		    if(fff_s)
		    	{
		    	fff_s=0;
			    DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,318);//318
		    	}
			else
				{
			    fff_s=1;//秒的十位变化
				}
		   	}
		   else
		   	{
		   	 
			   DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,318);//318
		   	}
		}	
		current_system_time.tm_sec -= current_tm*10;
		DateTimeStamp.tm_sec -= dataTime_tm*10;

		//+++
		current_tm = current_system_time.tm_sec;
		dataTime_tm = DateTimeStamp.tm_sec;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,339);//339
		}	

		DateTimeStamp.tm_sec = storeTime;
	}

	if((current_system_time.tm_min != DateTimeStamp.tm_min) || repaintFlag||fff_min||fff_min_1)
	{
	if(fff_min_1)
		{
		 fff_cnt_min1++;
		 if(fff_cnt_min1<2)
		 	return;
		}
	    fff_cnt_min1=0;
		storeTime = current_system_time.tm_min;

		//+++
		current_tm = current_system_time.tm_min/10;
		dataTime_tm = DateTimeStamp.tm_min/10;
		if((current_tm != dataTime_tm) || repaintFlag||fff_min_1)
		{
		if(video_osd_state){
		if(fff_min_1)
			{
			fff_min_1=0;
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,264);//264
			}
		else
			{
			 fff_min_1=1;
			}
			}
		else
			{
			 
			 DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,264);//264
			}
		}	

		if(fff_min)
		{
		 fff_cnt_min++;
		 if(fff_cnt_min<2)
		 	return;
		}
	    fff_cnt_min=0;
		
		current_system_time.tm_min -= current_tm*10;
		DateTimeStamp.tm_min -= dataTime_tm*10;

		//+++
		current_tm = current_system_time.tm_min;
		dataTime_tm = DateTimeStamp.tm_min;
		if((current_tm != dataTime_tm) || repaintFlag||fff_min)
		{
		if(video_osd_state){
		if(fff_min)
			{
			fff_min=0;
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,285);//285
			}
		else
			{
			fff_min=1;
			}
			}
		else
			{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,285);//285
			}
		}	

		DateTimeStamp.tm_min = storeTime;
	}

	if((current_system_time.tm_hour != DateTimeStamp.tm_hour) || repaintFlag||fff_hour||fff_hour_1)
	{
	if(fff_hour_1)
		{
		 fff_cnt_hour1++;
		 if(fff_cnt_hour1<2)
		 	return;
		}
	    fff_cnt_hour1=0;
		storeTime = current_system_time.tm_hour;

		//+++
		current_tm = current_system_time.tm_hour/10;
		dataTime_tm = DateTimeStamp.tm_hour/10;
		if((current_tm != dataTime_tm) || repaintFlag||fff_hour_1)
		{
		if(video_osd_state){
		if(fff_hour_1)
			{
			fff_hour_1=0;
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,210);
			}
		else
			{
			 fff_hour_1=1;
			}
			}
		else
			{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,210);
			}
		}	
		if(fff_hour)
		{
		 fff_cnt_hour++;
		 if(fff_cnt_hour<2)
		 	return;
		}
	    fff_cnt_hour=0;
		current_system_time.tm_hour -= current_tm*10;
		DateTimeStamp.tm_hour -= dataTime_tm*10;

		//+++
		current_tm = current_system_time.tm_hour;
		dataTime_tm = DateTimeStamp.tm_hour;
		if((current_tm != dataTime_tm) || repaintFlag||fff_hour)
		{
		if(video_osd_state){
		if(fff_hour)
			{
			fff_hour=0;
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,231);
			}
		else
			{
			 fff_hour=1;
			}
			}
		else
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,231);
		}	

		DateTimeStamp.tm_hour = storeTime;
	}

	if((current_system_time.tm_mday != DateTimeStamp.tm_mday) || repaintFlag)
	{
		storeTime = current_system_time.tm_mday;

		//+++
		current_tm = current_system_time.tm_mday/10;
		dataTime_tm = DateTimeStamp.tm_mday/10;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Day_Tens);
		}	
		current_system_time.tm_mday -= current_tm*10;
		DateTimeStamp.tm_mday -= dataTime_tm*10;

		//+++
		current_tm = current_system_time.tm_mday;
		dataTime_tm = DateTimeStamp.tm_mday;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Day_Ones);
		}	

		DateTimeStamp.tm_mday = storeTime;
	}

	if((current_system_time.tm_mon != DateTimeStamp.tm_mon) || repaintFlag)
	{
		storeTime = current_system_time.tm_mon;

		//+++
		current_tm = current_system_time.tm_mon/10;
		dataTime_tm = DateTimeStamp.tm_mon/10;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Month_Tens);
		}	
		current_system_time.tm_mon -= current_tm*10;
		DateTimeStamp.tm_mon -= dataTime_tm*10;

		//+++
		current_tm = current_system_time.tm_mon;
		dataTime_tm = DateTimeStamp.tm_mon;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Month_Ones);
		}	

		DateTimeStamp.tm_mon = storeTime;
	}
	
	if((current_system_time.tm_year != DateTimeStamp.tm_year) || repaintFlag)
	{
		storeTime = current_system_time.tm_year;

		//+++
		current_tm = current_system_time.tm_year/1000;
		dataTime_tm = DateTimeStamp.tm_year/1000;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Year_Thousands);
		}
		current_system_time.tm_year -= current_tm*1000;
		DateTimeStamp.tm_year -= dataTime_tm*1000;

		//+++
		current_tm = current_system_time.tm_year/100;
		dataTime_tm = DateTimeStamp.tm_year/100;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Year_Hundreds);
		}	
		current_system_time.tm_year -= current_tm*100;
		DateTimeStamp.tm_year -= dataTime_tm*100;

		//+++
		current_tm = current_system_time.tm_year/10;
		dataTime_tm = DateTimeStamp.tm_year/10;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Year_Tens);
		}	
		current_system_time.tm_year -= current_tm*10;
		DateTimeStamp.tm_year -= dataTime_tm*10;

		//+++
		current_tm = current_system_time.tm_year;
		dataTime_tm = DateTimeStamp.tm_year;
		if((current_tm != dataTime_tm) || repaintFlag)
		{
			DateTimeStringReplace(DateTimeStampBuf,(current_tm+0x30),19,DateTimeByteOffset.Year_Ones);
		}	

		DateTimeStamp.tm_year = storeTime;
	}

	if (repaintFlag)
	{
		DateTimeStringReplace(DateTimeStampBuf,0x2F,10,DateTimeByteOffset.Forward_Slash_1); // Forward Slash	
		DateTimeStringReplace(DateTimeStampBuf,0x2F,10,DateTimeByteOffset.Forward_Slash_2); // Forward Slash	
		DateTimeStringReplace(DateTimeStampBuf,0x3A,10,252); // Colon 	
		DateTimeStringReplace(DateTimeStampBuf,0x3A,10,309); // Colon 309	
	}		
}

extern INT16S ap_state_resource_time_stamp_position_x_get(void);
extern INT16S ap_state_resource_time_stamp_position_y_get(void);
#if 1
void cpu_draw_time_osd(TIME_T current_time, INT32U target_buffer, INT8U draw_type, INT8U state,INT32U ImgWidth, INT32U ImgHeight)
{
	STRING_INFO str_info = {0};
	INT8U enable_draw_time_flag = 0;

	enable_draw_time_flag = draw_type & 0xF0;
	draw_type &= 0x0F;

	if(((state & 0xF) == (STATE_VIDEO_RECORD & 0xF)) || ((state & 0xF) == (STATE_VIDEO_PREVIEW & 0xF))) 
	{
		if(draw_type == YUYV_DRAW)
		{
			str_info.font_color = 0xFF80;	//white		//0x5050;	//green
		}
		else // UYVY
		{
			str_info.font_color = 0x80FF;	//white		//0x5050;	//green
		}
		str_info.pos_x = 0;//ap_state_resource_time_stamp_position_x_get();
		
		// Video Recording
		if((state & 0xF) == (STATE_VIDEO_RECORD & 0xF))
		{
		    video_osd_state=1;
			if(ImgHeight == 16)
			{
				// 蔼40,ㄏノ3fifo(16 line)ㄓ丁 
				// Τ7兵フ,Τ兵フ
				if(state & 0x80)
				{	// The last part of date stamp
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 28;
					str_info.font_offset_h_width = 4; // Ι奔兵フ
				}
				else if(state & 0x40)
				{	// The 2nd part of date stamp 
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 12;
					str_info.font_offset_h_width = 16;
				}
				else
				{	// The first part of date stamp
					str_info.pos_y = 11;
					str_info.font_offset_h_start = 7;
					str_info.font_offset_h_width = 5; // Ι奔7兵フ
				}
			}
			else if(ImgHeight == 8)
			{
				// 蔼40,ㄏノ3fifo(16 line)ㄓ丁 
				// Τ7兵フ,Τ兵フ
				if((state & 0x60)==0x60)
				{   // The last part of date stamp
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 35;
					str_info.font_offset_h_width = 5; // Ι奔兵フ
				}
				else if(state & 0x80)
				{	// The 5th part of date stamp
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 27;
					str_info.font_offset_h_width = 8; // Ι奔兵フ
				}
				else if(state & 0x40)
				{	// The 4nd part of date stamp 
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 19;
					str_info.font_offset_h_width = 8;
				}
				else if(state & 0x20)
				{	// The 3nd part of date stamp 
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 11;
					str_info.font_offset_h_width = 8;
				}
				else if(state & 0x10)
				{	// The 2nd part of date stamp 
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 3;
					str_info.font_offset_h_width = 8;
				}
				else
				{	// The first part of date stamp
					str_info.pos_y = 5;
					str_info.font_offset_h_start = 0;
					str_info.font_offset_h_width = 3; // Ι奔7兵フ
				}
			}
			else
			{
				// 蔼40,ㄏノ2fifo(32 line)ㄓ丁 
				if(state & 0x40)
				{	// The 2nd part of date stamp 
					str_info.pos_y = 0;
					str_info.font_offset_h_start = 20;
					str_info.font_offset_h_width = 14;//12; // Ι奔兵フ
				}
				else
				{	// The first part of date stamp
					str_info.pos_y = 17;//19;
					str_info.font_offset_h_start = 5;//7;
					str_info.font_offset_h_width =15;//13; // Ι奔7兵フ
				}
			}
		} 
		else
		{	// Capture Photo
		    video_osd_state=0;
			if(ImgHeight == AVI_HEIGHT_720P)
			{
				str_info.pos_y = 651;
			}
			else if (ImgHeight == AVI_HEIGHT_WVGA)
			{
				str_info.pos_y = 434;
			}
			else // AVI_HEIGHT_QVGA
			{
				str_info.pos_y = 217;
			}

			str_info.font_offset_h_start = 0;
			str_info.font_offset_h_width = 40;
		}

		str_info.buff_w = ImgWidth;
		str_info.buff_h = ImgHeight; 	
	}

	DateTimeDraw(target_buffer,str_info,enable_draw_time_flag,draw_type);
	
}
#endif

/****************************************************************************/
/*
 *	cdsp_overflow_isr_func:
 */
void cdsp_overflow_isr_func(void)
{
    if(PreviewInitDone == 0) return;
	
    (*((volatile INT32U *) 0xC0130004)) = (0xFF<<16);

	scaler_stop(SCALER_0);
	scaler_stop(SCALER_1);

	if(gGpreviewArgs.display_buffer_addrs != DUMMY_BUFFER_ADDRS)
	{
		(*gGpreviewArgs.display_buffer_pointer_put)(display_isr_queue, gGpreviewArgs.display_buffer_addrs); //release display buffer
		gGpreviewArgs.display_buffer_addrs = DUMMY_BUFFER_ADDRS;
		scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs, 0, 0);
	}

	if(PreviewIsStopped) {
		if(cdsp_hangup_flag) {
			return;
		} else if(cdsp_overflow_count > 10*30) { //10s
			cdsp_hangup_flag = 1;
	   	} else {
			cdsp_overflow_count++;
		}
		return;
	}

	protect_recover_flag = 1;
	protect_recover_cnt = 0;

	wrap_filter_flush(WRAP_CSIMUX);
	wrap_filter_flush(WRAP_CSI2SCA);
	scaler_restart(SCALER_1);
	scaler_restart(SCALER_0);
}

/****************************************************************************/
/*
 *	conv422_isr_func:
 */
void conv422_isr_func(void)
{
	INT32U buf_A_B;
	INT32U buf_addrs;
	INT32U date_stamp_flag;
	INT32U date_stamp_msg;

	buf_A_B = conv422_idle_buf_get(0);

	// 倒數2/3?fifo 加上時間 %依據FIFO_LINE_LN 調整%
	date_stamp_flag = 0;
	gConv422Args.conv422_isr_count++;

	if(gConv422Args.conv422_fifo_path == FIFO_PATH_TO_VIDEO_RECORD)
	{
		if((gConv422Args.conv422_isr_count >= (gConv422Args.conv422_isr_count_max-lastFIFOIdx)) &&
		   (gConv422Args.conv422_isr_count < (gConv422Args.conv422_isr_count_max))
		)
		{
			date_stamp_flag = gConv422Args.conv422_isr_count-(gConv422Args.conv422_isr_count_max-lastFIFOIdx);
			date_stamp_msg = MSG_VIDEO_ENCODE_FIFO_1ST_DATE+(date_stamp_flag*0x00100000);
			date_stamp_flag++; // 這行用意是?底下進入判斷式
		}
	}
	
	if(buf_A_B == CONV422_IDLE_BUF_A)
	{
		/*
			取回之前Buffer A 位置傳出去,?位置是起始Buffer A地址
			通知 MSG_VIDEO_ENCODE_FIFO_START
		*/
		buf_addrs = conv422_output_A_addr_get();

		if(buf_addrs != fifo_addr_bak0) { //error occur
			buf_addrs = DUMMY_BUFFER_ADDRS;
			if(fifo_addr_bak0 != DUMMY_BUFFER_ADDRS) {
				gConv422Args.conv422_fifo_buffer_addrs_put(fifo_addr_bak0);
				DBG_PRINT("\r\nR = 0x%x\r\n", fifo_addr_bak0);
			}
		}
		fifo_addr_bak0 = fifo_addr_bak1;
		fifo_addr_bak1 = 0;
		
		if(gConv422Args.conv422_fifo_ready_notify)
		{
			if(gConv422Args.conv422_isr_count == 1)
			{
				// 通知jpeg 開始壓第1?fifo
				(*gConv422Args.conv422_fifo_ready_notify)(MSG_VIDEO_ENCODE_FIFO_START,buf_addrs,gConv422Args.conv422_isr_count);
			}
			else
			{
				if(date_stamp_flag > 0)
				{	// 通知上層可以加上Date Stamp
					(*gConv422Args.conv422_fifo_ready_notify)(date_stamp_msg,buf_addrs,gConv422Args.conv422_isr_count);
				}
				else
				{
					(*gConv422Args.conv422_fifo_ready_notify)(MSG_VIDEO_ENCODE_FIFO_CONTINUE,buf_addrs,gConv422Args.conv422_isr_count);
				}
			}
			
			if((buf_addrs != DUMMY_BUFFER_ADDRS) && (buf_addrs != 0))
			{
				gConv422Args.conv422_fifo_buffer_addrs_put(buf_addrs);
			}
		}

		if((gConv422Args.conv422_fifo_path == FIFO_PATH_TO_VIDEO_RECORD) || (gConv422Args.conv422_fifo_path == FIFO_PATH_TO_CAPTURE_PHOTO))
		{
			gConv422Args.conv422_buffer_next = gConv422Args.conv422_fifo_buffer_addrs_get();
			conv422_output_A_addr_set(gConv422Args.conv422_buffer_next);

			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			fifo_addr_bak1 = gConv422Args.conv422_buffer_next;
			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		}
	}

	if(buf_A_B == CONV422_IDLE_BUF_B)
	{
		buf_addrs = conv422_output_B_addr_get();

		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// To fix continuously printing "MMMMMMM...."
		if(buf_addrs != fifo_addr_bak0) { //error occur
			buf_addrs = DUMMY_BUFFER_ADDRS;
			if(fifo_addr_bak0 != DUMMY_BUFFER_ADDRS) {
				gConv422Args.conv422_fifo_buffer_addrs_put(fifo_addr_bak0);
				DBG_PRINT("\r\nR = 0x%x\r\n", fifo_addr_bak0);
			}
		}
		fifo_addr_bak0 = fifo_addr_bak1;
		fifo_addr_bak1 = 0;
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		if(gConv422Args.conv422_fifo_ready_notify)
		{
			if(gConv422Args.conv422_isr_count == 1)
			{
				// 通知jpeg 開始壓第1?fifo
				(*gConv422Args.conv422_fifo_ready_notify)(MSG_VIDEO_ENCODE_FIFO_START,buf_addrs,gConv422Args.conv422_isr_count);
			}
			else
			{
				if(date_stamp_flag > 0)
				{	// 通知上層可以加上Date Stamp
					(*gConv422Args.conv422_fifo_ready_notify)(date_stamp_msg,buf_addrs,gConv422Args.conv422_isr_count);
				}
				else
				{
					(*gConv422Args.conv422_fifo_ready_notify)(MSG_VIDEO_ENCODE_FIFO_CONTINUE,buf_addrs,gConv422Args.conv422_isr_count);
				}
			}

			if((buf_addrs != DUMMY_BUFFER_ADDRS) && (buf_addrs != 0))
			{
				gConv422Args.conv422_fifo_buffer_addrs_put(buf_addrs);
			}
		}

		if((gConv422Args.conv422_fifo_path == FIFO_PATH_TO_VIDEO_RECORD) || (gConv422Args.conv422_fifo_path == FIFO_PATH_TO_CAPTURE_PHOTO))
		{
			gConv422Args.conv422_buffer_next = gConv422Args.conv422_fifo_buffer_addrs_get();
			conv422_output_B_addr_set(gConv422Args.conv422_buffer_next);

			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			fifo_addr_bak1 = gConv422Args.conv422_buffer_next;
			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		}
	}

	if(conv422_frame_end_check() == STATUS_OK)
	{
		if(gConv422Args.conv422_fifo_ready_notify)
		{
			gConv422Args.conv422_isr_count = 0;
		}

		/*
			ePe]?O
		*/
		if(videoRecordFlag == VIDEO_RECORD_STOP1)
		{
			videoRecordFlag = VIDEO_RECORD_NOTHING;
			conv422_fifo_interrupt_enable(0);
			vic_irq_disable(18);

			fifo_addr_bak0 = 0;
			fifo_addr_bak1 = 0;

			DBG_PRINT("V");
		}
	}
}

/****************************************************************************/
/*
 *	zoom_in_out_func:
 */
/*void zoom_in_out_func(void)
{
	pSensorApiOps->frameRangeClip(&(gZoomArgs.sensor_clip_range));

	if((gZoomArgs.source_width == SENSOR_WIDTH) ||
	   (gZoomArgs.source_width == 960)
	)
	{
		scaler_input_pixels_set(SCALER_1,gZoomArgs.sensor_clip_range.clip_w, gZoomArgs.sensor_clip_range.clip_h);
		scaler_output_pixels_set(SCALER_1,gZoomArgs.scaler_factor_w, gZoomArgs.scaler_factor_h, gZoomArgs.source_width, gZoomArgs.source_height);
	}
}*/


/****************************************************************************/
/*
 *	sensor_display_update:
 */

/*
	將 Scaler 做完的 Buffer 傳給 Display
	DUMMY_BUFFER_ADDRS: 代表 queue 內 Buffer 都在使用 
*/ 

#if TV_DET_ENABLE
static void sensor_display_update(void)
{
	if(gGpreviewArgs.display_buffer_addrs != DUMMY_BUFFER_ADDRS) // Get display buffer from queue 
	{
		if(gGpreviewArgs.display_frame_ready_notify)
		{
			if(wantToPreview2DummyFlag == 0) {
				(*gGpreviewArgs.display_frame_ready_notify)(gGpreviewArgs.display_buffer_addrs);
			} else {
				(*gGpreviewArgs.display_buffer_pointer_put)(display_isr_queue, gGpreviewArgs.display_buffer_addrs);
			}
		}
	}

	/*
		取的下一?Display Buffer 傳給 Scaler
	*/
	if(wantToPreview2DummyFlag == 1) // Preview buffer pass to dummy address when connect to pc
	{
		gGpreviewArgs.display_buffer_addrs = DUMMY_BUFFER_ADDRS;
	}
	else
	{
		gGpreviewArgs.display_buffer_addrs = (*gGpreviewArgs.display_buffer_pointer_get)();
	}

	if(DoBlackEdgeFlag)
	{
		if(showOnTVFlag)
		{
			#if (TV_WIDTH == 640)
			scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs+(gGpreviewArgs.display_width*60*2), 0, 0); 
			#else
			scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs+(40<<1), 0, 0); 
			#endif
		}
		else
		{
		  #if (USE_PANEL_NAME == PANEL_T27P05_ILI8961)
			scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs+(40<<1), 0, 0); 
		#elif(USE_PANEL_NAME == PANEL_T43)
			scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs+(60<<1), 0, 0); 
		  #else
			scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs+(gGpreviewArgs.display_width*30*2), 0, 0); 
		  #endif
		}
	}
	else
	{
		scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs, 0, 0); 
	}

	/*
		Zoom In / Out
	*/
	/*
	if(zoomInOutFlag != DO_ZOOM_NOTHING)
	{
		zoom_in_out_func();			
		zoomInOutFlag = DO_ZOOM_NOTHING;
	} */
}
#endif

/****************************************************************************/
/*
 *	scaler_isr_func_for_encode
 */
void scaler_isr_func_for_encode(INT32U scaler0_event, INT32U scaler1_event)
{
	if((scaler0_event & C_SCALER_STATUS_DONE) && (scaler1_event & C_SCALER_STATUS_DONE))
	{
		//DBG_PRINT("-");

		scaler_stop(SCALER_0);
		scaler_stop(SCALER_1);
		if(PreviewIsStopped) return;

		#if TV_DET_ENABLE   // for tv out
		sensor_display_update();
		#endif   // end for tv out

		sensor_error_power_off_timer = 0; //reset auto power off timer
		cdsp_hangup_flag = 0;
		cdsp_overflow_count = 0;

		/*
			ePe]?O
		*/
		if(videoRecordFlag == VIDEO_RECORD_STOP1)  //error occurs here!!!! this should be done in conv422_isr_func()
		{
			/*
				Close csi2sca_wrap's o path
			*/
			wrap_path_set(WRAP_CSI2SCA,1,0);

			// 2??ˇconv422to420 protect
			(*((volatile INT32U *) 0xC0130004)) = (0x5C<<16);

			videoRecordFlag = VIDEO_RECORD_NOTHING;
			conv422_fifo_interrupt_enable(0);
			vic_irq_disable(18);

			fifo_addr_bak0 = 0;
			fifo_addr_bak1 = 0;

			DBG_PRINT("W");
		}

		/*
			e_?A?O
		*/
		else if(videoRecordFlag == VIDEO_RECORD_START)
		{
			if(protect_recover_flag) {
				protect_recover_cnt += 1;
				if(protect_recover_cnt > 5) {
					protect_recover_flag = 0;
					protect_recover_cnt = 0;					
				}				
			} else {
				/*
					切換到con422to420 做 flush
				*/
				videoRecordFlag = VIDEO_RECORD_RECING;

				// 多看conv422to420 protect	
				(*((volatile INT32U *) 0xC0130004)) = ((0x1Cul<<16)|(0xBFul<<24));

				/*
					Open csi2sca_wrap's o path
				*/
				wrap_path_set(WRAP_CSI2SCA,1,1);

				OSMboxPost(my_avi_encode_ack_m, (void*)C_ACK_SUCCESS);
			}
		}
		else if(videoRecordFlag == VIDEO_RECORD_STOP)
		{
			if(protect_recover_flag) {
				protect_recover_cnt += 1;
				if(protect_recover_cnt > 5) {
					protect_recover_flag = 0;
					protect_recover_cnt = 0;
				}
			} else {
				/*
					Close csi2sca_wrap's o path
				*/
				wrap_path_set(WRAP_CSI2SCA,1,0);

				// 2??ˇconv422to420 protect
				(*((volatile INT32U *) 0xC0130004)) = (0x5C<<16);

				videoRecordFlag = VIDEO_RECORD_STOP1;  //for conv422_isr_func() process
			}
		}
		else if(videoRecordFlag == VIDEO_RECORD_RECING)
		{
			// OUs@NeOO3E1ECOo?eAAOOECE1OAframe mode,2?DeOaOUpreview?E?astop 
		#if 1
			if(gConv422Args.conv422_fifo_path == FIFO_PATH_TO_CAPTURE_PHOTO)
			{
				protect_recover_flag = 0;
				protect_recover_cnt = 0;

				/*
					Close csi2sca_wrap's o path
				*/
				wrap_path_set(WRAP_CSI2SCA,1,0);

				// 2??ˇconv422to420 protect
				(*((volatile INT32U *) 0xC0130004)) = (0x5C<<16);

				videoRecordFlag = VIDEO_RECORD_STOP1;  //for conv422_isr_func() process
				wantToStopPreviewFlag = 1;
			}
			else
		#endif
			{
				if(protect_recover_flag) {
					protect_recover_cnt += 1;
					if(protect_recover_cnt > 5) {
						protect_recover_flag = 0;
						protect_recover_cnt = 0;

						// ?a?ˇconv422to420 protect	
						(*((volatile INT32U *) 0xC0130004)) = ((0x1Cul<<16)|(0xBFul<<24));
					}
				}
			}
		}
		else if(videoRecordFlag == VIDEO_RECORD_NOTHING)
		{
			if(protect_recover_flag) {
				protect_recover_cnt += 1;
				if(protect_recover_cnt > 5) {
					protect_recover_flag = 0;
					protect_recover_cnt = 0;

					// 不看conv422to420 protect
					(*((volatile INT32U *) 0xC0130004)) = (0x5C<<16);
				}				
			}

			/*
				先導到bypass 路徑再關sensor
			*/
			else if(wantToStopPreviewFlag == 1)
			{
			       if(gGpreviewArgs.display_buffer_addrs != DUMMY_BUFFER_ADDRS)
				{
					(*gGpreviewArgs.display_buffer_pointer_put)(display_isr_queue, gGpreviewArgs.display_buffer_addrs); //release display buffer
					gGpreviewArgs.display_buffer_addrs = DUMMY_BUFFER_ADDRS;
					scaler_output_addr_set(SCALER_0,gGpreviewArgs.display_buffer_addrs, 0, 0);
				}
				   
				(*(volatile INT32U *)(0xD0000084)) &= ~(1<<4);
				wrap_path_set(WRAP_CSIMUX,0,0);
				wrap_filter_flush(WRAP_CSIMUX);
				wrap_filter_flush(WRAP_CSI2SCA);
				wrap_filter_enable(WRAP_CSIMUX,0);
				wrap_protect_enable(WRAP_CSIMUX,0);
				wrap_filter_enable(WRAP_CSI2SCA,0);
				wantToStopPreviewFlag = 0;
				PreviewIsStopped = 1;
				return;
			}
		}
		wrap_filter_flush(WRAP_CSIMUX);
		wrap_filter_flush(WRAP_CSI2SCA);
		scaler_restart(SCALER_1);
		scaler_restart(SCALER_0);
	}
}

/****************************************************************************/
/*
 *	video_encode_preview_start
 */
INT32S video_preview_start(preview_args_t* pPreviewArgs)
{
	INT32U clip_width;
	INT32U clip_height;
	INT32U sensor_width;
	INT32U sensor_height;
	INT32U display_width;
	INT32U display_height;
	INT32U display_buffer_addrs;
	INT32U w_factor_scaler0,h_factor_scaler0;
	INT32U w_factor_scaler1,h_factor_scaler1;
	SCALER_MAS scaler0_mas;
	SCALER_MAS scaler1_mas;
	INT32U dummy_addrs = DUMMY_BUFFER_ADDRS;
	INT32U filter_addrs_size;
	sensor_frame_range_t sensorFrameRange = {0};
	OS_CPU_SR cpu_sr;

	/*
	  參數設值
	*/
	PreviewInitDone = 0;
	sensor_error_power_off_timer = 0;

	OS_ENTER_CRITICAL();
	showOnTVFlag = 0;
	wantToStopPreviewFlag = 0;
	DoBlackEdgeFlag = 0;
	videoRecordFlag = VIDEO_RECORD_NOTHING;

	gp_memcpy((INT8S*)&gGpreviewArgs,(INT8S*)pPreviewArgs,sizeof(preview_args_t));

	scaler_init(0);						// Initiate Scaler engine0
	scaler_init(1);						// Initiate Scaler engine1
	OS_EXIT_CRITICAL();
	   
	clip_width = gGpreviewArgs.clip_width;
	clip_height = gGpreviewArgs.clip_height;
	sensor_width = gGpreviewArgs.sensor_width;
	sensor_height = gGpreviewArgs.sensor_height;
	display_width = gGpreviewArgs.display_width;
	display_height = gGpreviewArgs.display_height; 

	if(display_width > TFT_WIDTH)
	{
		showOnTVFlag = 1;
	}

	if(showOnTVFlag) // On TV
	{
		#if (TV_WIDTH == 640)
			switch(sensor_width)
			{
				// 16:9 璶骸
				case AVI_WIDTH_720P:
				case AVI_WIDTH_WVGA:
					DoBlackEdgeFlag = 1;
				break;
				// 4:3 璶オ痙堵娩
				case AVI_WIDTH_VGA:
				case AVI_WIDTH_QVGA:
					DoBlackEdgeFlag = 0;
				break;			
				case 960:
					if(gGpreviewArgs.run_ap_mode == STATE_VIDEO_RECORD)
					{
						DoBlackEdgeFlag = 1;						
					}
					else
					{
						DoBlackEdgeFlag = 0;						
					}
				break;				
			}
			
			if(DoBlackEdgeFlag)
			{	
				display_height = 360; // 16:9 单ゑㄒ
			}
			else
			{
				display_height = gGpreviewArgs.display_height; 
			}
			/*
				scaler0 暗罽
			*/
			w_factor_scaler0 = sensor_width*65536/display_width;

			/*
				scaler0 罽,搭1
			*/
			if(sensor_height == display_height)
			{
				display_height--;
			}

			h_factor_scaler0 = sensor_height*65536/(display_height);			
		#else // 720
			switch(sensor_width)
			{
				// 16:9 璶骸
				case AVI_WIDTH_720P:
				case AVI_WIDTH_WVGA:
					DoBlackEdgeFlag = 0;
				break;
				// 4:3 璶オ痙堵娩
				case 960:
				case AVI_WIDTH_VGA:
				case AVI_WIDTH_QVGA:
					DoBlackEdgeFlag = 1;
				break;			
			}
			if(DoBlackEdgeFlag)
			{	
				w_factor_scaler0 = sensor_width*65536/640;
			}
			else
			{
				w_factor_scaler0 = sensor_width*65536/720;
			}	

			/*
				scaler0 暗罽
			*/
			h_factor_scaler0 = sensor_height*65536/display_height;
		#endif
	}
	else
	{			
	  #if (USE_PANEL_NAME == PANEL_T27P05_ILI8961)
		switch(sensor_width)
		{
			// 16:9 要滿屏
			case AVI_WIDTH_720P:
			case AVI_WIDTH_WVGA:
				DoBlackEdgeFlag = 0;
			break;
			// 4:3 要左右留黑?
			case 960:
			case AVI_WIDTH_VGA:
			case AVI_WIDTH_QVGA:
				DoBlackEdgeFlag = 1;
			break;			
		}
		if(DoBlackEdgeFlag)
		{	
			w_factor_scaler0 = sensor_width*65536/240;
		}
		else
		{
			w_factor_scaler0 = sensor_width*65536/320;
		}	

		/*
			scaler0 做縮小到屏
		*/
		h_factor_scaler0 = sensor_height*65536/display_height;
	  #elif (USE_PANEL_NAME == PANEL_T43)
		switch(sensor_width)
		{
			// 16:9 要滿屏
			case AVI_WIDTH_720P:
			case AVI_WIDTH_WVGA:
				DoBlackEdgeFlag = 0;
			break;
			// 4:3 要左右留黑?
			case 960:
			case AVI_WIDTH_VGA:
			case AVI_WIDTH_QVGA:
				DoBlackEdgeFlag = 1;
			break;			
		}
		if(DoBlackEdgeFlag)
		{	
			w_factor_scaler0 = sensor_width*65536/360;
		}
		else
		{
			w_factor_scaler0 = sensor_width*65536/480;
		}	

		/*
			scaler0 做縮小到屏
		*/
		h_factor_scaler0 = sensor_height*65536/display_height;


	  #else
		switch(sensor_width)
		{
			case 960:
			case AVI_WIDTH_VGA:
				DoBlackEdgeFlag = 0;
			break;

			case AVI_WIDTH_720P:
			case AVI_WIDTH_WVGA:
				DoBlackEdgeFlag = 1;
			break;			
		}
		
		if(DoBlackEdgeFlag)
		{	
			display_height = 180; // 為了16:9 等比例
		}
		else
		{
			display_height = gGpreviewArgs.display_height; 
		}
		/*
			scaler0 做縮小到屏
		*/
		w_factor_scaler0 = sensor_width*65536/display_width;
		h_factor_scaler0 = sensor_height*65536/display_height;
	  #endif
	}

	#if TV_DET_ENABLE   // for tv out
	display_buffer_addrs = gGpreviewArgs.display_buffer_addrs;
	#else
	display_buffer_addrs = 0xF8500000;///gGpreviewArgs.display_buffer_addrs;
	#endif   // end for tv out
	
	filter_addrs_size = gGpreviewArgs.sensor_width*gGpreviewArgs.sensor_height*2;


	/*
		scaler1 作放大
	*/
	w_factor_scaler1 = clip_width*65536/sensor_width;
	h_factor_scaler1 = clip_height*65536/sensor_height;

	/*
	  sensor 丟一 frame ?定後,走 csi_mux 的 sr path	  
	*/
	if(gGpreviewArgs.sensor_do_init)
	{
		cdsp_hangup_flag = 0;
		cdsp_overflow_count = 0;
		///gp_memset((INT8S*)&gZoomArgs,0,sizeof(zoom_args_t));

		//?理cdsp overflow
		cdsp_overflow_isr_register(cdsp_overflow_isr_func);
		
		// sensor 吐出 SENSOR_WIDTH & SENSOR_HEIGHT
		pSensorApiOps = sensor_attach();
		pSensorApiOps->init();	// Initiate sensor
		pSensorApiOps->start(dummy_addrs,0);
	}
	else if(s_usbd_pin)
		{
		  if(pSensorApiOps->get_fps() != 20)
  	 	  {
  	 	    OSSchedLock();
			pSensorApiOps->wait4FrameEnd();
			pSensorApiOps->set_fps(20);
			pSensorApiOps->wait4FrameEnd();
			OSSchedUnlock();
  	 	  }
		}

	//++ 開完機後等sensor frame end
	if((gGpreviewArgs.sensor_width == SENSOR_WIDTH) &&
		(gGpreviewArgs.sensor_height == SENSOR_HEIGHT)
	)
	{
		sensorFrameRange.point_x = (SENSOR_WIDTH-clip_width)>>1;
		sensorFrameRange.point_y = (SENSOR_HEIGHT-clip_height)>>1;

		sensorFrameRange.clip_w = clip_width;
		sensorFrameRange.clip_h = clip_height+4;

		sensorFrameRange.scaledown_w = clip_width;
		sensorFrameRange.scaledown_h = clip_height;

		sensorFrameRange.frame_w = SENSOR_WIDTH;
		sensorFrameRange.frame_h = SENSOR_HEIGHT;
		sensorFrameRange.docropflag = 1;
	}
	else if((gGpreviewArgs.sensor_width == 960) &&
		(gGpreviewArgs.sensor_height == 720)
	)
	{
		sensorFrameRange.point_x = (SENSOR_WIDTH-clip_width)>>1;
		sensorFrameRange.point_y = (SENSOR_HEIGHT-clip_height)>>1;

		sensorFrameRange.clip_w = clip_width;
		sensorFrameRange.clip_h = clip_height+2;

		sensorFrameRange.scaledown_w = clip_width;
		sensorFrameRange.scaledown_h = clip_height;

		sensorFrameRange.frame_w = SENSOR_WIDTH;
		sensorFrameRange.frame_h = SENSOR_HEIGHT;
		sensorFrameRange.docropflag = 1;

	}
	else if((gGpreviewArgs.sensor_width == 848) &&
		(gGpreviewArgs.sensor_height == 480)
	)
	{
		sensorFrameRange.point_x = (SENSOR_WIDTH-1268)>>1;
		sensorFrameRange.point_y = (SENSOR_HEIGHT-720)>>1;
		sensorFrameRange.clip_w = 1268;
		sensorFrameRange.clip_h = 720;
		sensorFrameRange.scaledown_w = gGpreviewArgs.sensor_width;
		sensorFrameRange.scaledown_h = gGpreviewArgs.sensor_height;
		sensorFrameRange.frame_w = 1268;
		sensorFrameRange.frame_h = 720;
		sensorFrameRange.docropflag = 1;
		sensorFrameRange.doscaledownflag = 1;
	}
	else if((gGpreviewArgs.sensor_width == 640) &&
		(gGpreviewArgs.sensor_height == 480)
	)
	{
		sensorFrameRange.point_x = (SENSOR_WIDTH-960)>>1;
		sensorFrameRange.point_y = (SENSOR_HEIGHT-720)>>1;
		sensorFrameRange.clip_w = 960;
		sensorFrameRange.clip_h = 720;
		sensorFrameRange.scaledown_w = gGpreviewArgs.sensor_width;
		sensorFrameRange.scaledown_h = gGpreviewArgs.sensor_height;
		sensorFrameRange.frame_w = 960;
		sensorFrameRange.frame_h = 720;
		sensorFrameRange.docropflag = 1;
		sensorFrameRange.doscaledownflag = 1;
	}
	else if((gGpreviewArgs.sensor_width == 320) &&
		(gGpreviewArgs.sensor_height == 240)
	)
	{
		sensorFrameRange.point_x = (SENSOR_WIDTH-960)>>1;
		sensorFrameRange.point_y = (SENSOR_HEIGHT-720)>>1;
		sensorFrameRange.clip_w = 960;
		sensorFrameRange.clip_h = 720;
		sensorFrameRange.scaledown_w = gGpreviewArgs.sensor_width;
		sensorFrameRange.scaledown_h = gGpreviewArgs.sensor_height;
		sensorFrameRange.frame_w = 960;
		sensorFrameRange.frame_h = 720;
		sensorFrameRange.docropflag = 1;
		sensorFrameRange.doscaledownflag = 1;
	}
	OSSchedLock();
	pSensorApiOps->wait4FrameEnd();
	pSensorApiOps->frameRangeClip(&sensorFrameRange);
	pSensorApiOps->wait4FrameEnd();
	OSSchedUnlock();
	//---
	
	wrap_addr_set(WRAP_CSIMUX,dummy_addrs);
	wrap_filter_addr_set(WRAP_CSIMUX,dummy_addrs,filter_addrs_size);	
	wrap_path_set(WRAP_CSIMUX,0,0);
	wrap_filter_enable(WRAP_CSIMUX,0);

	//+++ protect mechanism
	(*((volatile INT32U *) 0xC0130004)) = (0x5C<<16);
	wrap_protect_enable(WRAP_CSIMUX,0);
	wrap_protect_pixels_set(WRAP_CSIMUX,clip_width,clip_height);
	//---
	
	/*	
		走 csi2sca_wrap 的 sr path
	*/	
	wrap_addr_set(WRAP_CSI2SCA,dummy_addrs);
	wrap_path_set(WRAP_CSI2SCA,1,0);
	wrap_filter_enable(WRAP_CSI2SCA,0);

	
	scaler_isr_callback_set(&scaler_isr_func_for_encode);	

	/*
		scaler1
	*/
	gp_memset((INT8S*)&scaler1_mas,0,sizeof(SCALER_MAS));
	scaler1_mas.mas_2 = MAS_EN_READ;
	scaler1_mas.mas_3 = MAS_EN_WRITE;
	scaler_mas_set(SCALER_1,&scaler1_mas);
	
	scaler_input_A_addr_set(SCALER_1,dummy_addrs, 0, 0);
	scaler_fifo_line_set(SCALER_1,C_SCALER_CTRL_FIFO_DISABLE);
	scaler_input_format_set(SCALER_1,C_SCALER_CTRL_IN_UYVY);
	scaler_output_format_set(SCALER_1,C_SCALER_CTRL_OUT_YUYV);
	scaler_input_pixels_set(SCALER_1,clip_width, clip_height);
	scaler_output_pixels_set(SCALER_1,w_factor_scaler1, h_factor_scaler1, sensor_width, sensor_height);
	scaler_output_addr_set(SCALER_1,dummy_addrs, 0, 0); 

	/*
		scaler0
	*/
	gp_memset((INT8S*)&scaler0_mas,0,sizeof(SCALER_MAS));
	scaler0_mas.mas_2 = MAS_EN_READ;
	scaler0_mas.mas_0 = MAS_EN_WRITE;
	scaler_mas_set(SCALER_0,&scaler0_mas);

       if(showOnTVFlag == 0) // 琵TFT陪ボぃ亏睛
	{
		scaler_input_offset_set(SCALER_0,0x8000, 0x8000);
	}

	scaler_input_A_addr_set(SCALER_0,dummy_addrs, 0, 0);
	scaler_fifo_line_set(SCALER_0,C_SCALER_CTRL_FIFO_DISABLE);
	scaler_input_format_set(SCALER_0,C_SCALER_CTRL_IN_YUYV);
	scaler_output_format_set(SCALER_0,C_SCALER_CTRL_OUT_RGB565);
	scaler_input_pixels_set(SCALER_0,sensor_width, sensor_height);
	scaler_output_pixels_set(SCALER_0,w_factor_scaler0, h_factor_scaler0, display_width, display_height);
	scaler_out_of_boundary_mode_set(SCALER_0,1);

	if(DoBlackEdgeFlag)
	{
		if(showOnTVFlag)
		{
		  #if (TV_WIDTH == 640)
			scaler_output_addr_set(SCALER_0,(display_buffer_addrs+(display_width*60*2)), 0, 0); 
		  #else
			// 铬筁玡80Bytes
			scaler_output_addr_set(SCALER_0,(display_buffer_addrs+(40<<1)), 0, 0); 		
		  #endif
		}
		else
		{
		  #if (USE_PANEL_NAME == PANEL_T27P05_ILI8961)
			// 跳過前80Bytes
			scaler_output_addr_set(SCALER_0,(display_buffer_addrs+(40<<1)), 0, 0); 		
	  #elif (USE_PANEL_NAME == PANEL_T43)
			scaler_output_addr_set(SCALER_0,(display_buffer_addrs+(60<<1)), 0, 0); 		
		  #else
			// 铬筁玡30兵
			scaler_output_addr_set(SCALER_0,(display_buffer_addrs+(display_width*30*2)), 0, 0); 
		  #endif
		}
	}
	else
	{
		scaler_output_addr_set(SCALER_0,display_buffer_addrs, 0, 0); 
	}

	OSSchedLock();

	pSensorApiOps->wait4FrameEnd();

	(*(volatile INT32U *)(0xD0000084)) |= (1<<4);
	
	wrap_path_set(WRAP_CSIMUX,1,0);
	wrap_filter_enable(WRAP_CSIMUX,1);
	wrap_protect_enable(WRAP_CSIMUX,1);
	wrap_filter_enable(WRAP_CSI2SCA,1);
	PreviewIsStopped = 0;

	wrap_filter_flush(WRAP_CSIMUX);
	wrap_filter_flush(WRAP_CSI2SCA);

	scaler_start(SCALER_1);
	scaler_start(SCALER_0);

       OSSchedUnlock();
	PreviewInitDone = 1;
	
    return 0;
}

/****************************************************************************/
/*
 *	video_preview_stop
 */
INT32S video_preview_stop(INT8U closeSensor)
{
	INT32U time_begin, t;

	wantToStopPreviewFlag = 1;
	if(PreviewIsStopped == 0) {
		time_begin = OSTimeGet();
		while(wantToStopPreviewFlag == 1) {
			t = OSTimeGet();
			if((t - time_begin) > 50) { //500ms
			       DBG_PRINT("preview stop wait error!\r\n");
				break;
			}
		}
	}
	wantToStopPreviewFlag = 0;

	// Close sensor
	if(closeSensor)
	{
		pSensorApiOps->stop();
	}
	
	return 0;
}

/****************************************************************************/
/*
 *	sensor_2_dummy_addrs_wait
 */
INT32S sensor_2_dummy_addrs_wait(void)
{
	INT32U time_begin, t;

	time_begin = OSTimeGet();
	while(wantToStopPreviewFlag == 1) {
		t = OSTimeGet();
		if((t - time_begin) > 50) { //500ms
			DBG_PRINT("dummy wait error!\r\n");
			break;
		}
	}

	return 0;	
}

/****************************************************************************/
/*
 *	video_preview_zoom_to_zero
 */
void video_preview_zoom_to_zero(void)
{
	///gZoomArgs.zoom_step = 0;
}

/****************************************************************************/
/*
 *	video_preview_zoom_setp_get
 */
INT32U video_preview_zoom_setp_get(void)
{
	///return gZoomArgs.zoom_step;
	return 0;
}

/****************************************************************************/
/*
 *	video_preview_zoom_in_out:
 *
 *  	zoomInOut: 1: zoom in 0: zoom out
 */
INT32S video_preview_zoom_in_out(INT8U zoomInOut, INT8U zoomIn_zoomOut)
{
/*	gp_memset((INT8S*)&(gZoomArgs.sensor_clip_range),0,sizeof(sensor_frame_range_t));

	if(zoomIn_zoomOut == 1) // Zoom In
	{
    	if(gZoomArgs.zoom_step < 5)
    	{
        	gZoomArgs.zoom_step++;
    	}
    	else
    	{
    		return 0;
    	}
	}
	else // Zoom Out
	{
		if(gZoomArgs.zoom_step != 0)
		{
    		gZoomArgs.zoom_step--;
    	}	     
    	else
    	{
    		return 0;
    	}    	
	}

	gZoomArgs.source_width = gGpreviewArgs.sensor_width;
	gZoomArgs.source_height = gGpreviewArgs.sensor_height;

	if((gGpreviewArgs.sensor_width == SENSOR_WIDTH) &&
	   (gGpreviewArgs.sensor_height == SENSOR_HEIGHT)
	)
	{
		ZOOM_WIDTH_INTERVAL = 128;
		ZOOM_HEIGHT_INTERVAL = 72;
		gZoomArgs.sensor_clip_range.point_x = ((ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step)>>1);
		gZoomArgs.sensor_clip_range.point_y = ((ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step)>>1);
		gZoomArgs.sensor_clip_range.clip_w = SENSOR_WIDTH - (ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.clip_h = SENSOR_HEIGHT - (ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.scaledown_w = gZoomArgs.sensor_clip_range.clip_w;
		gZoomArgs.sensor_clip_range.scaledown_h = gZoomArgs.sensor_clip_range.clip_h;
		gZoomArgs.sensor_clip_range.docropflag = 1;		
		gZoomArgs.sensor_clip_range.doscaledownflag = 0;		

		gZoomArgs.scaler_factor_w = gZoomArgs.sensor_clip_range.clip_w*65536/gZoomArgs.source_width;
		gZoomArgs.scaler_factor_h = gZoomArgs.sensor_clip_range.clip_h*65536/gZoomArgs.source_height;
	
	}
	else if((gGpreviewArgs.sensor_width == 960) &&
	   (gGpreviewArgs.sensor_height == 720)
	)
	{
		ZOOM_WIDTH_INTERVAL = 96;
		ZOOM_HEIGHT_INTERVAL = 72;
		gZoomArgs.sensor_clip_range.point_x = ((SENSOR_WIDTH-960)>>1) + ((ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step)>>1);
		gZoomArgs.sensor_clip_range.point_y = ((ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step)>>1);
		gZoomArgs.sensor_clip_range.clip_w = 960 - (ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.clip_h = 720 - (ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.scaledown_w = gZoomArgs.sensor_clip_range.clip_w;
		gZoomArgs.sensor_clip_range.scaledown_h = gZoomArgs.sensor_clip_range.clip_h;
		gZoomArgs.sensor_clip_range.docropflag = 1;		
		gZoomArgs.sensor_clip_range.doscaledownflag = 0;		

		gZoomArgs.scaler_factor_w = gZoomArgs.sensor_clip_range.clip_w*65536/gZoomArgs.source_width;
		gZoomArgs.scaler_factor_h = gZoomArgs.sensor_clip_range.clip_h*65536/gZoomArgs.source_height;
	
	}	
 	else if((gGpreviewArgs.sensor_width == 848) &&
	   (gGpreviewArgs.sensor_height == 480)
	)
	{
		ZOOM_WIDTH_INTERVAL = 84;
		ZOOM_HEIGHT_INTERVAL = 48;

		gZoomArgs.sensor_clip_range.point_x = ((SENSOR_WIDTH-1268)>>1) + ((ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step)>>1);
		gZoomArgs.sensor_clip_range.point_y = (ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step)>>1;
		gZoomArgs.sensor_clip_range.clip_w = 1268 - (ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.clip_h = 720 - (ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.scaledown_w = gGpreviewArgs.sensor_width;
		gZoomArgs.sensor_clip_range.scaledown_h = gGpreviewArgs.sensor_height;
		gZoomArgs.sensor_clip_range.frame_w = gZoomArgs.sensor_clip_range.clip_w;
		gZoomArgs.sensor_clip_range.frame_h = gZoomArgs.sensor_clip_range.clip_h;
		gZoomArgs.sensor_clip_range.docropflag = 1;		
		gZoomArgs.sensor_clip_range.doscaledownflag = 0;		
		if(gZoomArgs.zoom_step < 5)
		{
			gZoomArgs.sensor_clip_range.doscaledownflag = 1;		
		}
	
	}
 	else if((gGpreviewArgs.sensor_width == 640) &&
	   (gGpreviewArgs.sensor_height == 480)
	)
	{
		ZOOM_WIDTH_INTERVAL = 64;
		ZOOM_HEIGHT_INTERVAL = 48;

		gZoomArgs.sensor_clip_range.point_x = ((SENSOR_WIDTH-960)>>1) + ((ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step)>>1);
		gZoomArgs.sensor_clip_range.point_y = (ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step)>>1;
		gZoomArgs.sensor_clip_range.clip_w = 960 - (ZOOM_WIDTH_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.clip_h = 720 - (ZOOM_HEIGHT_INTERVAL*gZoomArgs.zoom_step);
		gZoomArgs.sensor_clip_range.scaledown_w = gGpreviewArgs.sensor_width;
		gZoomArgs.sensor_clip_range.scaledown_h = gGpreviewArgs.sensor_height;
		gZoomArgs.sensor_clip_range.frame_w = gZoomArgs.sensor_clip_range.clip_w;
		gZoomArgs.sensor_clip_range.frame_h = gZoomArgs.sensor_clip_range.clip_h;
		gZoomArgs.sensor_clip_range.docropflag = 1;		
		gZoomArgs.sensor_clip_range.doscaledownflag = 0;		
		if(gZoomArgs.zoom_step < 5)
		{
			gZoomArgs.sensor_clip_range.doscaledownflag = 1;		
		}
	}

	if(zoomInOut) 
	{// zoom in
		zoomInOutFlag = DO_ZOOM_IN;
	}
	else 
	{// zoom out
		zoomInOutFlag = DO_ZOOM_OUT;
	}
*/
	return 0;
}

/****************************************************************************/
/*
 *	sensor_crop_W_H_get:
 *
 */
void sensor_crop_W_H_get(INT32U* pCropWValue,INT32U* pCropHValue)
{
	*pCropWValue = ZOOM_WIDTH_INTERVAL;
	*pCropHValue = ZOOM_HEIGHT_INTERVAL;
}

/****************************************************************************/
/*
 *	video_recording_start:
 *
 */
INT32S video_recording_start(vid_rec_args_t* pVidRecArgs)
{
	gConv422Args.conv422_fifo_ready_notify = pVidRecArgs->fifo_ready_notify;
	gConv422Args.conv422_fifo_buffer_addrs_get = pVidRecArgs->fifo_buffer_addrs_get;
	gConv422Args.conv422_fifo_buffer_addrs_put = pVidRecArgs->fifo_buffer_addrs_put;

	// FIFO Mode Setting
	gConv422Args.conv422_fifo_total_count = pVidRecArgs->vid_rec_fifo_total_count;
	gConv422Args.conv422_fifo_line_len = pVidRecArgs->vid_rec_fifo_line_len;

	if(gConv422Args.conv422_fifo_line_len == 16)
	{
		lastFIFOIdx = 3;
		//DBG_PRINT("------jh1----\r\n");
	}
	else
	{
		lastFIFOIdx = 2;//6;//2;
		//DBG_PRINT("------jh2----\r\n");
	}
	
	gConv422Args.conv422_fifo_path = pVidRecArgs->vid_rec_fifo_path;
	gConv422Args.conv422_fifo_data_size = pVidRecArgs->vid_rec_fifo_data_size;
	gConv422Args.conv422_output_format = pVidRecArgs->vid_rec_fifo_output_format;

	gConv422Args.conv422_buffer_A = gConv422Args.conv422_fifo_buffer_addrs_get();

	if(gConv422Args.conv422_fifo_path == FIFO_PATH_TO_VIDEO_RECORD)
	{
		gConv422Args.conv422_buffer_B = gConv422Args.conv422_fifo_buffer_addrs_get();
	}
	else
	{
		gConv422Args.conv422_buffer_B = gConv422Args.conv422_buffer_A;
	}
	
	// Start recording
	gConv422Args.conv422_isr_count_max = my_pAviEncVidPara->sensor_height/gConv422Args.conv422_fifo_line_len;
	
	gConv422Args.conv422_isr_count = 0;

	/*
		Conv_422_to_420 setting
	*/
	vic_irq_register(18,conv422_isr_func);
	vic_irq_enable(18);

	conv422_input_pixels_set(gGpreviewArgs.sensor_width,gGpreviewArgs.sensor_height);
	conv422_fifo_line_set(gConv422Args.conv422_fifo_line_len);
	conv422_output_A_addr_set(gConv422Args.conv422_buffer_A);
	conv422_output_B_addr_set(gConv422Args.conv422_buffer_B);

	fifo_addr_bak0 = gConv422Args.conv422_buffer_A;
	fifo_addr_bak1 = gConv422Args.conv422_buffer_B;

	if(gConv422Args.conv422_output_format == FIFO_FORMAT_422)
	{
		conv422_output_format_set(FORMAT_422);
	}
	else
	{
		conv422_output_format_set(FORMAT_420);
	}

	conv422_mode_set(FIFO_MODE);
	conv422_fifo_interrupt_enable(1);
	conv422_clear_set();
	
	videoRecordFlag = VIDEO_RECORD_START;
	return 0;
}

/****************************************************************************/
/*
 *	video_recording_stop_get
 */
INT32U video_recording_stop_get(void)
{
	return (videoRecordFlag == VIDEO_RECORD_NOTHING)?1:0;
}

/****************************************************************************/
/*
 *	video_recording_stop
 */
void video_recording_stop(void)
{
	videoRecordFlag = VIDEO_RECORD_STOP;
}

/****************************************************************************/
/*
 *	do_scale_up_next_block
 */
void do_scale_up_next_block(void)
{
	// Scaler A/B 都填滿不可以再做 scaler_restart
	if(gScalupArgs.scaler_buffer_remaining_idx != 0)
	{
		// Scaler engine ?未完成, 繼續動作
		if(gScalupArgs.scaler_engine_status != SCALER_ENGINE_STATUS_FINISH) 
		{
			gScalupArgs.scaler_engine_status = SCALER_ENGINE_STATUS_BUSY;
			gScalupArgs.scaler_buffer_idx = !gScalupArgs.scaler_buffer_idx;
			scaler_restart(SCALER_0);
		}
	}
	else
	{   // Scaler A/B Buffer 都滿狀況
		gScalupArgs.scaler_engine_status = SCALER_ENGINE_STATUS_IDLE;
	}
}
 
/****************************************************************************/
/*
 *	scaler_engine_status_get
 */
INT32U scaler_engine_status_get(void)
{
	return gScalupArgs.scaler_engine_status;
}

/****************************************************************************/
/*
 *	scaler_isr_func_for_capture
 */
 extern INT8U gphoto_save_data;
void scaler_isr_func_for_capture(INT32U scaler0_event, INT32U scaler1_event)
{

	if(scaler0_event & C_SCALER_STATUS_DONE)
	{
		if(gScalupArgs.scaler_buffer_idx == SCALER_BUFFER_IDX_A)
		{
			(*gScalupArgs.scaler_ready_notify)(MSG_VIDEO_ENCODE_FIFO_END,gScalupArgs.scaler_buffer_A);
		}
		else
		{
			(*gScalupArgs.scaler_ready_notify)(MSG_VIDEO_ENCODE_FIFO_END,gScalupArgs.scaler_buffer_B);					
		}	

		gScalupArgs.scaler_engine_status = SCALER_ENGINE_STATUS_FINISH;
	}
	else if(scaler0_event & C_SCALER_STATUS_OUTPUT_FULL)
	{
		if(gScalupArgs.scaler_buffer_idx == SCALER_BUFFER_IDX_A)
		{
			if(gScalupArgs.scaler_1st_buffer_notify == 0)
			{
				gScalupArgs.scaler_1st_buffer_notify = 1;
				(*gScalupArgs.scaler_ready_notify)(MSG_VIDEO_ENCODE_FIFO_START,gScalupArgs.scaler_buffer_A);
			}
			else
			{
				(*gScalupArgs.scaler_ready_notify)(MSG_VIDEO_ENCODE_FIFO_CONTINUE,gScalupArgs.scaler_buffer_A);
			}
		}
		else // SCALER_BUFFER_IDX_B
		{
			(*gScalupArgs.scaler_ready_notify)(MSG_VIDEO_ENCODE_FIFO_CONTINUE,gScalupArgs.scaler_buffer_B);					
		}

		gDo_Scale_Up = 1;
        if(gphoto_save_data == 0)
        {
            gDo_Scale_Up = 0;
		    do_scale_up_next_block();
        }
	}
}

/****************************************************************************/
/*
 *	do_scale_up_stop
 */
void do_scale_up_stop(void)
{
	scaler_stop(SCALER_0);	

	if(gScalupArgs.scaler_buffer_A != 0)
	{
		gp_free((void *)gScalupArgs.scaler_buffer_A);		
	}
}

/****************************************************************************/
/*
 *	do_scale_up_start
 */
void do_scale_up_start(scaler_args_t* pScalerArgs)
{
	SCALER_MAS scaler0_mas;
	INT32U w_factor_scaler0,h_factor_scaler0;

	scaler_init(SCALER_0);
	#if !TV_DET_ENABLE
	conv420_init(); 
	#endif

	//+++ Parameters
	gScalupArgs.scaler_ready_notify = pScalerArgs->scaler_ready_notify;
	gScalupArgs.scaler_buffer_idx = SCALER_BUFFER_IDX_A;
	gScalupArgs.scaler_1st_buffer_notify = 0;
	gScalupArgs.scaler_buffer_remaining_idx = SCALER_BUFFER_IDX_MAX; // Scaler A/B Buffer

	w_factor_scaler0 = pScalerArgs->scaler_in_width*65536/pScalerArgs->scaler_out_width;
	h_factor_scaler0 = pScalerArgs->scaler_in_height*65536/pScalerArgs->scaler_out_height;

	gScalupArgs.scaler_buffer_size = pScalerArgs->scaler_out_width*SCALER_CTRL_OUT_FIFO_16LINE*2; // C_SCALER_CTRL_OUT_FIFO_16LINE
	gScalupArgs.scaler_buffer_A = pScalerArgs->scaler_out_buffer_addrs;
	gScalupArgs.scaler_buffer_B = gScalupArgs.scaler_buffer_A+gScalupArgs.scaler_buffer_size;

       //+++ conv420_to_422
	#if !TV_DET_ENABLE
	conv420_reset();
	conv420_path(CONV420_TO_SCALER0);
	conv420_input_A_addr_set(pScalerArgs->scaler_in_buffer_addrs);
	conv420_input_pixels_set(pScalerArgs->scaler_in_width);

	if(pScalerArgs->scaler_input_format == FIFO_FORMAT_422)		
	{
		conv420_convert_enable(0); // conversion disable: source data is 422
	}
	else
	{
		conv420_convert_enable(1); //420 -> 422
	}
	conv420_start();
	#endif
	//+++ Scaler 0
	scaler_isr_callback_set(&scaler_isr_func_for_capture);	

	gp_memset((INT8S*)&scaler0_mas,0,sizeof(SCALER_MAS));
	#if TV_DET_ENABLE
	scaler0_mas.mas_0 = (MAS_EN_READ|MAS_EN_WRITE);
	#else
	scaler0_mas.mas_1 = MAS_EN_READ;
	scaler0_mas.mas_0 = MAS_EN_WRITE;	
	#endif
	scaler_mas_set(SCALER_0,&scaler0_mas);

	scaler_input_A_addr_set(SCALER_0,pScalerArgs->scaler_in_buffer_addrs, 0, 0);
	scaler_input_format_set(SCALER_0,C_SCALER_CTRL_IN_YUYV);
	scaler_fifo_line_set(SCALER_0,C_SCALER_CTRL_FIFO_DISABLE);
	scaler_output_format_set(SCALER_0,C_SCALER_CTRL_OUT_YUYV);
	scaler_input_pixels_set(SCALER_0,pScalerArgs->scaler_in_width, pScalerArgs->scaler_in_height);
	scaler_output_pixels_set(SCALER_0,w_factor_scaler0, h_factor_scaler0, pScalerArgs->scaler_out_width, pScalerArgs->scaler_out_height);
	scaler_output_fifo_line_set(SCALER_0,C_SCALER_CTRL_OUT_FIFO_16LINE);
	scaler_output_addr_set(SCALER_0,gScalupArgs.scaler_buffer_A, 0, 0); 

	gScalupArgs.scaler_engine_status = SCALER_ENGINE_STATUS_BUSY;
	scaler_start(SCALER_0);
}

/****************************************************************************/
/*
 *	scale_up_buffer_idx_add : ?jpeg engine 完成呼叫此func
 */
void scale_up_buffer_idx_add(void)
{
	OS_CPU_SR cpu_sr;

	OS_ENTER_CRITICAL();

	if(gScalupArgs.scaler_buffer_remaining_idx < SCALER_BUFFER_IDX_MAX)
	{
		gScalupArgs.scaler_buffer_remaining_idx++;
	}

	OS_EXIT_CRITICAL();	
}

/****************************************************************************/
/*
 *	scale_up_buffer_idx_decrease : ?scaler engine 完成呼叫此func
 */
void scale_up_buffer_idx_decrease(void)
{
	OS_CPU_SR cpu_sr;

	OS_ENTER_CRITICAL();

	if(gScalupArgs.scaler_buffer_remaining_idx != 0)
	{
		gScalupArgs.scaler_buffer_remaining_idx--;
	}	

	OS_EXIT_CRITICAL();	
}

/****************************************************************************/
/*
 *	video_preview_buf_to_dummy
 */
void video_preview_buf_to_dummy(INT8U enableValue)
{
	wantToPreview2DummyFlag = enableValue;
}

/****************************************************************************/
/*
 *	enable_black_edge_get
 */
INT32U enable_black_edge_get(void)
{
	return DoBlackEdgeFlag;
}

