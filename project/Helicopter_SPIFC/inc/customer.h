

#ifndef __CUSTOMER_H__
#define __CUSTOMER_H__
#define CUSTOM_ON				1
#define CUSTOM_OFF				0

#define DBG_MESSAGE 			CUSTOM_ON// UART debug message
#define AUDIO_DEC_FUNC			CUSTOM_OFF

//configure
#define DVP_V1_0				1
#define GPCV1248_JVIN_V1_0		2
#define GPCV1248_C6_V1_0		3
#define GPCV1248_C6_V2_0		4
#define GPCV1248_WKA_V1_0		5
#define GPCV1248_TOPFUTURE_V1_0 6
#define GPCV1237A_Aerial_Photo	10
#define GPDV_BOARD_VERSION		GPCV1237A_Aerial_Photo //DVP_V1_0

// CPU and system clock
#define INIT_MHZ				144//96   // MHz

//#define RAM_ADDR						0x00200000
#define RAM_ADDR				0x00200000///0x1E5B80///0x1FA100 //0x00200000

#define SUPPORT_JTAG			CUSTOM_OFF // ICE

// panel
#define PANEL_T27P05_ILI8961	0
#define PANEL_TPO_TD025THEA7	1	
#define PANEL_T43				2	
#define PANEL_400X240_I80		3	
#define PANEL_T40P00_ILI9342C	4

#define PANEL_SCREEN_PROPOTION_4_3 0
#define PANEL_SCREEN_PROPOTION_16_9 1

#define G_SENSOR_DA380			0
#define G_SENSOR_DMARD07		1

/*Sensor Type*/
#define SENSOR_OV7670			0	
#define SENSOR_OV9712			1
#define SENSOR_SOI_H22			2
#define SENSOR_BF3703			3
#define SENSOR_SOI_H22_MIPI 	4
#define SENSOR_OV3640			5
#define SENSOR_OV5642			6
#define SENSOR_GC1004			7
#define SENSOR_GC1004_MIPI		8


/*Sensor Format*/
//max resolution: 640*480				
#define OV7670_YUYV 			0x00
#define OV7670_BAYER_RAW		0x01
#define OV7670_RGRB 			0x04
#define OV7670_PROCESS_BAYER_RAW 0x05

//max resolution: 2048*1536
#define OV3640_YUYV 			0x00
#define OV3640_UYVY 			0x02
#define OV3640_BGGR 			0x18
#define OV3640_GBRG 			0x19
#define OV3640_GRBG 			0x1A
#define OV3640_RGGB 			0x1B

//max resolution: 2592*1944
#define OV5642_UYVY 			0x32
#define OV5642_BGGR 			0x00
#define OV5642_GBRG 			0x01
#define OV5642_GRBG 			0x02
#define OV5642_RGGB 			0x03
#define OV5642_MIPI_YUV 		0x4032
#define OV5642_MIPI_GRBG		0x4002

//#define OV5642_MIPI_2LANE_YUV		0x8032
//#define OV5642_MIPI_2LANE_GRGB		0x8002
//max resolution: 1296*800	
#define OV9712_RAW				0x01

//max resolution 1296*816
#define SOI_H22_RAW 			0x01
#define SOI_H22_MIPI			0x02

//max resolution 1296*742
#define GC1004_RAW				0x01
#define GC1004_MIPI 			0x02

//max resolution 1296*742
#define GC1004_RAW				0x01
#define GC1004_MIPI 			0x02

//max resolution 640*480
#define BF3703_RAW				0x01

//max resolution: 640*480				
#define OV7725_YUYV 			0x00
#define OV7725_BAYER_RAW		0x01
#define OV7725_RGRB 			0x04
#define OV7725_PROCESS_BAYER_RAW 0x05

#define COLOR_BAR				0x22


#if 0
#define SENSOR_WIDTH			640
#define SENSOR_HEIGHT			480
#elif 1
#define SENSOR_WIDTH			1280
#define SENSOR_HEIGHT			720
#elif 0
#define SENSOR_WIDTH			1280
#define SENSOR_HEIGHT			800	
#elif 0
#define SENSOR_WIDTH			1920
#define SENSOR_HEIGHT			1080
#elif 0
#define SENSOR_WIDTH			2048
#define SENSOR_HEIGHT			1536

#else

#define SENSOR_WIDTH			2592
#define SENSOR_HEIGHT			1944
#endif

#define SENSOR_BUF_FRAME_MODE	1		// Frame mode
#define SENSOR_BUF_FIFO_MODE	2		// FIFO mode
#define VIDEO_ENCODE_USE_MODE	SENSOR_BUF_FRAME_MODE

// AVI recording resolution: 640x480, 720x480, 1280x720
#define AVI_WIDTH_1080FHD		1920
#define AVI_HEIGHT_1080FHD		1080

#define AVI_WIDTH_1080P 		1440
#define AVI_HEIGHT_1080P		1080

#define AVI_WIDTH_720P			1280
#define AVI_HEIGHT_720P 		720

#define AVI_WIDTH_WVGA			848
#define AVI_HEIGHT_WVGA 		480

#define AVI_WIDTH_VGA			640
#define AVI_HEIGHT_VGA			480

#define AVI_WIDTH_QVGA			320
#define AVI_HEIGHT_QVGA 		240

#define AVI_Y_Q_VALUE_PC_CAM	50


#define AVI_Q_VALUE_CAPTURE_BEST 98
#define AVI_Q_VALUE_CAPTURE_NORNAL 90
#define AVI_Q_VALUE_CAPTURE_LOW 80


#define IndexBuffer_Size		(32*1024)	// Do not modify this
#define AVI_ENCODE_SHOW_TIME	0

// Photo resolution and quality
//#define PIC_WIDTH						640
//#define PIC_HEIGH						480
#define PIC_Q_VALUE 			50		// 30, 40, 50, 60, 70, 80, 90

// Motion detection
#define MD_STOP_TIME			10		// when MD_STOP_TIME seconds detect no motion then stop video record

//motion detect sensitive
#define LOW 					300
#define MEDIUM					150
#define HIGH					70
#define MD_SENSITIVE			HIGH

// Storage type
#define MINI_DVR_STORAGE_TYPE	2		// T_FLASH
#define RAMDISK_TYPE			3		// SDRAM



// key detection
#define KEY_ACTIVE				DATA_HIGH
#define KEY_TYPE0				0
#define KEY_TYPE1				1
#define KEY_TYPE2				2
#define KEY_TYPE3				3
#define KEY_TYPE4				4
#define KEY_TYPE5				5
#define KEY_TYPE10				10

#define LED_ON_TYPE 			1//0--IO output low,then LED light

#define READ_FROM_PIN			0
#define READ_FROM_GPIO			1
#define FUNCTION_KEY			AD_KEY_DETECT_PIN
#define NEXT_KEY				AD_KEY_DETECT_PIN
#define PREVIOUS_KEY			AD_KEY_DETECT_PIN
#define MENU_KEY				AD_KEY_DETECT_PIN

//#define	CAPTURE_KEY						AD_KEY_DETECT_PIN
#define OK_KEY					AD_KEY_DETECT_PIN
#define SOS_KEY 				AD_KEY_DETECT_PIN
#define C6_KEY					1
#define SAMPLE2 				2
#define KEY_FUNTION_TYPE		SAMPLE2

// GPIO
#define USE_PANEL_NAME			PANEL_TPO_TD025THEA7
#define USE_PANEL_SCREEN_PROPOTION PANEL_SCREEN_PROPOTION_4_3

#define USE_SENSOR_NAME 		SENSOR_GC1004 
#define sensor_format			GC1004_MIPI

#define SENSOR_FLIP 			0	
#define USE_G_SENSOR_NAME		G_SENSOR_DA380	
#define USE_G_SENSOR_ACTIVE 	0
#define CPU_DRAW_TIME_STAMP_BLACK_EDGE 0
#define USE_IOKEY_NO			1
#define USE_ADKEY_NO			1
#define KEY_TYPE				KEY_TYPE10
#define PWR_KEY_TYPE			READ_FROM_PIN
#define PW_KEY					PWR_KEY0
#define ADP_OUT_PIN 			PWR_KEY1
#define C_USBDEVICE_PIN 		ADP_OUT_PIN
#define AD_KEY_DETECT_PIN		ADC_LINE_0
#define AD_BAT_DETECT_PIN		AD_KEY_DETECT_PIN

///#if SUPPORT_JTAG
#define VIDEO_KEY				IO_F15//IO_B9
#define CAPTURE_KEY 			IO_F15//IO_C15//IO_C15//IO_C15//IO_C15

#define SCCB_SCL				IO_E1  // Sensor I2C clock pin
#define SCCB_SDA				IO_E2	// Sensor I2C data pin

#define LED1					IO_C12// IO_B2
#define LED2					IO_F15// IO_B3
#define FUN_KEY 				IO_F15


#define I2C_SCL 				IO_F15	// G_Sensor I2C clock pin
#define I2C_SDA 				IO_F15	// G_Sensor I2C data pin

#define TV_OUT_MENU 			1	//please change resource
#define TV_DET_ENABLE			0
#define TV_DET_ACTIVE			1	   //tv_detect active level
#define AV_IN_DET				IO_F15//IO_A14 

#define HDMI_IN_DET 			IO_F15//IO_A9
#define TFT_BL					IO_F15	
#define SPEAKER_EN				IO_F15//IO_A8

#define IR_CTRL 				IO_F15//IO_B11	 //ir light 
#define LED 					IO_F15
#define LED_FLOOD_LIGHT			IO_E3

#define POWER_EN				IO_C13
#define CHARGE_PIN				IO_D13

#define CDSP_IQ 				GPCV1248_JVIN_V1_0//GPCV1248_C6_V2_0	
#define PRODUCT_NUM 			7673
#define PRODUCT_DATA			20170911
#define PROGRAM_VERSION_NUM 	1



//+++
#define JPEG_REC_TIME			0
#define JPEG_AE_GAIN			1
#define JPEG_TIME_STAMP_SHOW	JPEG_REC_TIME

//---
// Function enable
#define C_LOGO					CUSTOM_OFF
#define C_UVC					CUSTOM_ON
#define C_USB_AUDIO 			CUSTOM_OFF
#define C_MOTION_DETECTION		CUSTOM_OFF
#define C_AUTO_DEL_FILE 		CUSTOM_OFF
#define C_CYCLIC_VIDEO_RECORD	CUSTOM_ON
#define C_SCREEN_SAVER			CUSTOM_OFF
#define C_POWER_OFF_LOGO		CUSTOM_OFF
#define C_BATTERY_DETECT		CUSTOM_ON
#define C_BATTERY_LOW_POWER_OFF CUSTOM_ON
#define SUPPORT_LONGKEY 		CUSTOM_ON
#define USING_EXT_RTC			CUSTOM_ON
#define USING_FILE_RTC			CUSTOM_OFF
#define USING_FILE_TIME 		CUSTOM_ON
#define AUD_RECORD_EN			CUSTOM_OFF


#if DBG_MESSAGE
#define DBG_PRINT				print_string
#define __msg					print_string

#else
#define DBG_PRINT				if (0) print_string
#define __msg					if (0) print_string
#endif

#define UART_USED_NUM			1  // 0 or 1
#define SD_USED_NUM 			1//0  // 0 or 1

#define MJ_DECODE_SCALER		0  // SCALER_0
#define UNDEFINE_SCALER 		0  // SCALER_0	

//#define PWM_CTR_LED
//+++
#define ENABLE_SAVE_SENSOR_RAW_DATA 0
#define CDSP_DEBUG				0

#define NEW_LENS				1

#define Resolution_1080P		0

#endif //__CUSTOMER_H__

