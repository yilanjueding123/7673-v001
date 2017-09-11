#ifndef __drv_l1_SYSTEM_H__
#define __drv_l1_SYSTEM_H__

#include "driver_l1.h"
#include "drv_l1_sfr.h"

#define   GPIO_CLK_SW  		       		0x00000004
#define   TIMERS_CLK_SW        			0x00000010
#define   GTE_CLK_SW           		 	0x00000020
#define   DAC_CLK_SW           		    0x00000040
#define   UART_EFUSE_CLK_SW    			0x00000080
#define   MP3_CLK_SW           			0x00000100
#define   SPI0_SPI1_CLK_SW     		 	0x00000200
#define   ADC_CLK_SW       		   		0x00000400
#define   PPU_LINEBASE_CLK_SW	   		0x00000800
#define   SPU_CLK_SW       		        0x00001000
#define   TFT_CLK_SW          		 	0x00002000
#define   SDC_CLK_SW       		     	0x00008000

#define   MS_CLK_SW        		   		0x00000001
#define   UHOST_CLK_SW     		        0x00000002
#define   UDEVICE_CLK_SW   	  		    0x00000004
#define   STN_CLK_SW   	    		    0x00000008
#define   TV_CLK_SW            			0x00000020
#define   PPU_FRAMEBASE_CLK_SW 			0x00000040
#define   CFC_CLK_SW               		0x00000100
#define   KEYSCAN_TOUCH_SENSOR_CLK_SW  	0x00000200
#define   JPEG_CLK_SW  		            0x00000400
#define   DEFLICKER_CLK_SW 				0x00000800
#define   SCALER_CLK_SW		            0x00001000
#define   NAND_CLK_SW		            0x00002000

#define   LDO_28_28v					0x00
#define   LDO_28_29v					0x20
#define   LDO_28_30v					0x40
#define   LDO_28_31v					0x60
#define   LDO_28_32v					0x80
#define   LDO_28_33v					0xA0

#define   LDO_CODEC_30v					0x000
#define   LDO_CODEC_31v					0x200
#define   LDO_CODEC_32v					0x400
#define   LDO_CODEC_33v					0x600

extern INT32U  MCLK;
extern INT32U  MHZ;

extern INT32S system_clk_init(void);
extern void system_enter_halt_mode(void);
extern void system_power_on_ctrl(void);
extern void system_da_ad_pll_en_set(BOOLEAN status);
extern void sys_weak_6M_set(BOOLEAN status);
extern void sys_reg_sleep_mode_set(BOOLEAN status);
extern void sys_ir_opcode_clear(void);
extern INT32U sys_ir_opcode_get(void);
extern INT32S sys_ldo28_ctrl(int flag, int mode);
extern INT32S sys_ldo_codec_ctrl(int flag, int mode);
extern INT32S sys_ldo33_off(void);
extern INT32S sys_pwr_key0_read(void);
extern INT32S sys_pwr_key1_read(void);

extern INT32S sys_msdc_clk_active(void);
extern INT32S sys_msdc_clk_restore(void);

#endif		// __drv_l1_SYSTEM_H__

