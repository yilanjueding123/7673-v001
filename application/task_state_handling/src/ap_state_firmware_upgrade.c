/*
* Description: This file provides APIs to upgrade firmware from SD card to SPI
*
* Author: Tristan Yang
*
* Date: 2008/02/17
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
*/
#include "string.h"
#include "ap_state_firmware_upgrade.h"
#include "ap_state_resource.h"
#include "drv_l2_spifc.h"
#include "ap_state_handling.h"
#include "drv_l1_gpio.h"
#include "application.h"
#include "math.h"
#include "stdio.h"

#define DRV_Reg32_(addr)               (*(volatile INT32U *)(addr))
#define SPIFC_RESOURCE_OFFSET	0
extern spifc_apis_ops* pSpifc_apis_ops;
extern INT32S SPIFC_Flash_erase_block(INT32U addr, INT8U eraseArgs);
extern INT32S SPIFC_Flash_read_page(INT32U addr, INT8U *buf);
extern INT32S SPIFC_Flash_write_page(INT32U addr, INT8U *buf);

extern INT32S spifc_quad_mode_set(INT8U ctrlValue);
extern void led_green_on(void);

#define SPIFC_RESOURCE_OFFSET	0
#define UPGRADE_POS_X 10
#define UPGRADE_POS_Y 100
extern void led_red_on(void);
extern void led_green_on(void);
extern void led_all_off(void);
extern void led_green_off(void);
extern void led_red_off(void);

static INT32S memcmp_buf(void *SRC, void *DST, INT32U len)
{
    INT32U i, ret = 0;
    INT8U *p_src = SRC;
    INT8U *p_dst = DST;

    for (i=0; i<len; ++i)
    {
        if (p_src[i] != p_dst[i])
        {
            ret = i;
            break;
        }
    }
    return ret;
}
#if 0
static INT32S memcpy_buf(void *SRC, void *DST, INT32U len)
{
    INT32U i;
    INT32U *p_src = (INT32U*)SRC;
    INT32U *p_dst = (INT32U*)DST;

    len >>= 2;
    for (i=0; i<len; ++i)
    {
        p_dst[i] = p_src[i];
    }
    return 0;
}
#endif
/*static INT32S load_resource(void)
{
    memcpy_buf(sd_upgrade_320x15, (void*)iRAM_sd_upgrade_320x15, 320*15*2);
    memcpy_buf(sd_upgrade_ok_320x33, (void*)iRAM_sd_upgrade_ok_320x33, 320*33*2);
    memcpy_buf(sd_upgrade_fail_320x15, (void*)iRAM_sd_upgrade_fail_320x15, 320*15*2);
    return 0;
}*/

/*static INT32S upgrade_frame(INT32U frame_buf, INT32U resource_buf, INT32U line)
{
    INT16U ((*p_res)[TFT_WIDTH]) = (INT16U ((*)[TFT_WIDTH]))resource_buf;

    INT32U i, j;
    INT32U buff_size = TFT_WIDTH * TFT_HEIGHT;
    INT16U *p_buf = (INT16U*)	frame_buf;

    // clear buffer
    for (i=0; i<buff_size; ++i)
    {
        p_buf[i] = 0;
    }

    // put text
    p_buf = (INT16U*)(frame_buf + ((TFT_HEIGHT/2)*TFT_WIDTH));
    for (i=0; i<line; ++i)
    {
        for (j=0; j<TFT_WIDTH; ++j)
        {
            *p_buf++ = p_res[i][j];
        }
    }

    return 0;
}*/

static INT32S erase_block(INT32U addr, INT8U eraseArgs)
{
    INT32S retErr;

    retErr = pSpifc_apis_ops->erase(addr, eraseArgs);

    return retErr;
}

static INT32S read_page(INT32U addr, INT8U *buf)
{
    INT32S ret = STATUS_OK;
    TX_RX_ARGS txrxArgs = {0};

    txrxArgs.addrs = addr;
    txrxArgs.buff = buf;
    txrxArgs.buff_len = 256;

    ret = pSpifc_apis_ops->read(&txrxArgs);

    return ret;
}

static INT32S write_page(INT32U addr, INT8U *buf)
{
    INT32S ret = STATUS_OK;
    TX_RX_ARGS txrxArgs = {0};

    txrxArgs.addrs = addr;
    txrxArgs.buff = buf;
    txrxArgs.buff_len = 256;

    ret = pSpifc_apis_ops->program(&txrxArgs);

    return ret;
}
#if 0
static void firmware_upgrade_delay(INT32U cnt)
{
    INT32U i;
    for (i=0; i<cnt; ++i)
        R_RANDOM0 = i;
}
#endif
static INT32U *read_upgrade_file(INT32U *buf_start, INT32S *p_size, INT32U blk_len, INT32U file_total_size)
{
    static INT32U pos = 0;
    INT32U buf_next = (INT32U)buf_start + pos;
    INT32U next_pos = pos + blk_len;

    if (pos>=file_total_size)
        *p_size = 0;
    else if (next_pos<=file_total_size)
    {
        *p_size = file_total_size - next_pos;
    }
    else *p_size = next_pos - file_total_size;
    if (*p_size>blk_len)
    {
        *p_size = blk_len;
    }

    pos = next_pos;
    return (INT32U*)buf_next;
}

/*static void cpu_draw_burn_percent(INT32U val, INT32U target_buffer)
{
	STRING_INFO str_info = {0};
	INT16U temp;

	str_info.language = LCD_EN;
	str_info.font_color = 0xFFFF;	//white
	str_info.pos_x = UPGRADE_POS_X;
	str_info.pos_y = UPGRADE_POS_Y+40;
	str_info.buff_w = TFT_WIDTH;
	str_info.buff_h = TFT_HEIGHT;

	temp = val/10;
	ap_state_resource_char_draw(temp+0x30, (INT16U *) target_buffer, &str_info, RGB565_DRAW, 0);
	temp = val - temp * 10;
	ap_state_resource_char_draw(temp+0x30, (INT16U *) target_buffer, &str_info, RGB565_DRAW, 0);
}*/

static INT32U spifc_rewrite(int idx, INT8U *frame_buf, INT8U* verify_buf)
{
    INT32U i = 0;
    INT32U ret = -1;
    INT32U time = 5;

    do
    {
        for (i=0; i<C_UPGRADE_SPI_WRITE_SIZE; ++i)
        {
            if (verify_buf[i]!=0xFF)
            {
                ///DBG_PRINT("0x%x page is not empty\r\n",idx);
                return -1;				// Erase 64KB，整個 PAGE 重新寫過
            }
        }
        if (write_page(idx, frame_buf)!=0)
        {
            ///DBG_PRINT("Rewrite 0x%x Error!!\r\n");
            return -1;
        }

        if (read_page(idx, verify_buf)!=0)
        {
            ///DBG_PRINT("Reread 0x%x Error!!\r\n");
        }

        if (memcmp_buf(verify_buf,frame_buf,C_UPGRADE_SPI_WRITE_SIZE)==0)
        {
            ///DBG_PRINT("0x%x rewrite OK\r\n",idx);
            ret = 0;
            break;
        }
    }
    while(--time != 0);

    return ret;
}

static INT32U STRING_Convert_HEX(CHAR *str)
{
    INT32U sum = 0;
    INT8U len;
    INT16S i;

    len = strlen(str);
    for(i = 0; i<len; i++)
    {
        if (str[i]>='0'&&str[i]<='9') sum += (str[i]-'0')*(pow(16,len-1-i));
        else if(str[i]>='a'&&str[i]<='f') sum += (str[i]-'a'+10)*(pow(16,len-1-i));
        else if(str[i]>='A'&&str[i]<='F') sum += (str[i]-'A'+10)*(pow(16,len-1-i));
    }
    return sum; 
}
static void Send_Upfail_Message(void)
{
	//INT32U type;
   // type = LED_UPDATE_FAIL; //LED_WAITING_AUDIO_RECORD;
   // msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT32U), MSG_PRI_NORMAL);
    led_all_off();
    led_green_on();
	OSTimeDly(5);
	led_all_off();
	//led_red_on();
}

void ap_state_firmware_upgrade(void)
{
	INT32U i, j, k, total_size, complete_size,type;
	INT32U *upgrade_file, *firmware_buffer, *verify_buffer;
	///INT32U display_buffer;
	///INT32U buff_size, *buff_ptr;	
	INT32S verify_size;
	///INT16U *ptr; 
	struct stat_t statetest;
	INT16S fd,nRet;
	///STRING_ASCII_INFO ascii_str;
	//INT8S  prog_str[6];
	INT8U retry=0;
	///OS_Q_DATA OSQData;
	char   p[4];
	INT8U  FileName_Path[3+5+8+4+1+4];
	INT8U  checksum_temp[8+1];
	INT32U *checksum_buffer; 
	INT32U checksum_addr = 0;
	INT8U  *q;
    INT16U checksum_cnt;   
	INT32U CheckSum = 0; 
	INT32U CheckSum_BIN;  
	static INT8U flash_cnt=0;
	static INT8U time_temp=0;
	struct f_info	file_info;
	
//	#if C_AUTO_DEL_FILE == CUSTOM_ON
	if (storage_sd_upgrade_file_flag_get() != 2) {
		return;
	}
//	#endif
#if 0
	fd = open("C:\\gp_cardvr_upgrade.bin", O_RDONLY);
	if (fd < 0) {
		return;
	}
#endif
	   sprintf((char*)p,(const char*)"%04d",PRODUCT_NUM);
	   gp_strcpy((INT8S*)FileName_Path,(INT8S*)"C:\\JH_");
	   gp_strcat((INT8S*)FileName_Path,(INT8S*)p);
	   gp_strcat((INT8S*)FileName_Path,(INT8S*)"*.bin");

	   nRet = _findfirst((CHAR*)FileName_Path, &file_info ,D_ALL); 
	   if (nRet < 0) 
	   {
	     
		   return ;
	   }
      DBG_PRINT("FIND UPDATAFILE TYPE\r\n");
	  DBG_PRINT("FileName = %s\r\n", file_info.f_name);
	  strncpy((CHAR*)checksum_temp,(CHAR*)file_info.f_name+8,8);    
      checksum_temp[8] = NULL;                         
      gp_strcpy((INT8S*)FileName_Path,(INT8S*)"C:\\");
      gp_strcat((INT8S*)FileName_Path,(INT8S*)file_info.f_name);
	  fd = open((CHAR*)FileName_Path, O_RDONLY);
	  if (fd < 0)
	  	{
             Send_Upfail_Message();
			 return  ;
	    }
	  if (fstat(fd, &statetest)) 
		{
		     close(fd);
			 Send_Upfail_Message();
		     return;
	    }
	  OSTaskDel(STORAGE_SERVICE_PRIORITY);
	  OSTaskDel(USB_DEVICE_PRIORITY);
	  OSTaskDel(PERIPHERAL_HANDLING_PRIORITY);

	///ap_state_handling_tv_uninit();

	total_size = statetest.st_size;
	verify_size = (INT32S)total_size;

	checksum_buffer = (INT32U *) gp_malloc(256);
	if (!checksum_buffer)
		{
			gp_free((void*)checksum_buffer);
			Send_Upfail_Message();
			return ;
		}
		DBG_PRINT("Total_size=%d\r\n",total_size);
	for(k=0; k<total_size/256; k++)
	   {
		   lseek(fd, checksum_addr, SEEK_SET);	 
		   if (read(fd,(INT32U)checksum_buffer,256)<=0)
		   {
			   gp_free((void*)checksum_buffer);
			    Send_Upfail_Message();
			   return ;
		   }
		   q = (INT8U*)checksum_buffer;
		   for(checksum_cnt=0; checksum_cnt<256; checksum_cnt++)
		   {
			   CheckSum += *q++;
		   }
		   checksum_addr += 256;
	   }
	if((total_size%256)!= 0)
		{
	      lseek(fd, checksum_addr, SEEK_SET);
	     // read(fd,(INT32U)checksum_buffer,total_size%256);
		  if (read(fd,(INT32U)checksum_buffer,total_size%256)<=0)
		   {
			   gp_free((void*)checksum_buffer);
			    Send_Upfail_Message();
			   return ;
		   }
	      q = (INT8U*)checksum_buffer;
	      for(checksum_cnt=0; checksum_cnt<(total_size%256); checksum_cnt++)
	          {
				   CheckSum += *q++;
	          }
		}
	 
     DBG_PRINT("Check_sum=%d\r\n",CheckSum);
	 gp_free((void*)checksum_buffer);                               
     CheckSum_BIN = STRING_Convert_HEX((CHAR*)checksum_temp);       
     DBG_PRINT("Checksum_bin=%d\r\n",CheckSum_BIN);
    if (CheckSum_BIN == CheckSum)  
    {
     //  type = LED_UPDATE_PROGRAM;
	  // msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT32U), MSG_PRI_NORMAL);
       DBG_PRINT("Checksum ok\r\n");
    }
    else 
    {
		Send_Upfail_Message();
       DBG_PRINT("Checksum fail\r\n");
       return ;
    }

	upgrade_file = (INT32U *) gp_malloc(total_size);
	if (!upgrade_file) {
		///DBG_PRINT("firmware upgrade allocate firmware_buffer fail\r\n");
		close(fd);
		return;
	}
	complete_size = 0;
    lseek(fd, complete_size, SEEK_SET);
	read(fd, (INT32U) upgrade_file, total_size);

	verify_buffer = (INT32U *) gp_malloc(C_UPGRADE_SPI_WRITE_SIZE);
	if (!verify_buffer) {
		///DBG_PRINT("firmware upgrade allocate verify_buffer fail\r\n");
		close(fd);
		Send_Upfail_Message();
		return;
	}

//	OSTaskDel(STORAGE_SERVICE_PRIORITY);
//	OSTaskDel(USB_DEVICE_PRIORITY);
	//OSTaskDel(PERIPHERAL_HANDLING_PRIORITY);
	R_INT_GMASK = 1;	// avoid produre enter SPI area
	
	//led_all_off();
	//led_red_on();
	//DBG_PRINT("fire_up-start\r\n");
#if 1
      pSpifc_apis_ops = spifc_attach();
      pSpifc_apis_ops->init();
      ///spifc_quad_mode_set(0);  // Erase function must use 1-bit mode
      nvmemory_init();

	  R_SPIFC_TIMING = 0;		// SPI delay
	  R_IOD_DRV = 0x0;			// SPI_CLK driving

      R_SYSTEM_PLLEN  |= 0xC00;	// 把 SPI clock 降低，讀寫比較穩
      complete_size = 0;
	  led_all_off();
      while (complete_size < total_size)
      {
          INT32U buffer_left;
          INT32S block_size;
 
          firmware_buffer = read_upgrade_file(upgrade_file, &block_size, C_UPGRADE_BUFFER_SIZE, total_size);
          if (block_size <= 0)
          {
              break;
          }

          buffer_left = (total_size - complete_size + (C_UPGRADE_SPI_BLOCK_SIZE-1)) & ~(C_UPGRADE_SPI_BLOCK_SIZE-1);
          if (buffer_left > C_UPGRADE_BUFFER_SIZE)
          {
              buffer_left = C_UPGRADE_BUFFER_SIZE;
          }
          retry = 0;  // 每個 64KB 有 20 次機會
          while (buffer_left && retry<C_UPGRADE_FAIL_RETRY)
          {
            INT32U complete_size_bak;
            complete_size &= ~(C_UPGRADE_SPI_BLOCK_SIZE-1);
            complete_size_bak = complete_size;
            if (erase_block(SPIFC_RESOURCE_OFFSET+complete_size_bak, ERASE_BLOCK_64K))
            {
                retry++;
                continue;
            }
            for (j=C_UPGRADE_SPI_BLOCK_SIZE; j; j-=C_UPGRADE_SPI_WRITE_SIZE)
            { 
            if(time_temp>25)
            	{
            	time_temp=0;
            if(flash_cnt ==0)
		  	  {
                led_green_on();
		        flash_cnt=1;
		  	  }
		    else
		  	  {
		        led_green_off();
		        flash_cnt=0;
		  	  }
            	}
			time_temp++;
			
                if (write_page(SPIFC_RESOURCE_OFFSET+complete_size_bak, (INT8U *) (firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE-1))>>2))))
                {
                    break;
                }
                complete_size_bak += C_UPGRADE_SPI_WRITE_SIZE;
            }

            if ((j == 0)&&(verify_size>0))  	// verify stage
            {
                complete_size_bak = complete_size;

                for (j=C_UPGRADE_SPI_BLOCK_SIZE; j; j-=C_UPGRADE_SPI_WRITE_SIZE)
                {
                    INT32U i;
                    INT32U flag = 0;
					 if(time_temp>25)
            	{
            	time_temp=0;
            if(flash_cnt ==0)
		  	  {
                led_green_on();
		        flash_cnt=1;
		  	  }
		    else
		  	  {
		        led_green_off();
		        flash_cnt=0;
		  	  }
            	}
			time_temp++;
                    for (i=0; i<5; ++i)
                    {
                        if ((read_page(SPIFC_RESOURCE_OFFSET+complete_size_bak, (INT8U*)verify_buffer) )==0 )
                            break;
                    }

                    flag = 0;
                    if (memcmp_buf( (void*)verify_buffer  ,(void *)(firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE-1))>>2)), C_UPGRADE_SPI_WRITE_SIZE)!=0)
                    {
                        ///DBG_PRINT("Verify 0x%x (0x%x) Error !!\r\n",complete_size_bak,C_UPGRADE_SPI_WRITE_SIZE);
                        flag = 1;
                    }
                    //else DBG_PRINT("Verify 0x%x (0x%x) OK !!\r\n",complete_size_bak,verify_size);

                    if (flag == 1)
                    {
                        if ( spifc_rewrite(SPIFC_RESOURCE_OFFSET+complete_size_bak, (INT8U*)(firmware_buffer + ((complete_size_bak & (C_UPGRADE_BUFFER_SIZE-1))>>2)),  (INT8U*)verify_buffer)!=0 )
                        {
                            break;
                        }
                    }

                    complete_size_bak += C_UPGRADE_SPI_WRITE_SIZE;
                    verify_size-= C_UPGRADE_SPI_WRITE_SIZE;
                    if (verify_size <=0) // 檔案結束，沒必要再比下去
                    {
                        j = 0;
                        break;
                    }
                }
            }

            complete_size = complete_size_bak;
            if (j == 0)
            {
                buffer_left -= C_UPGRADE_SPI_BLOCK_SIZE;
            }
            else
            {
                retry++;
            }
			  led_green_off();
        }
        if (retry == C_UPGRADE_FAIL_RETRY)
        {
            break;
        }
    }
#endif


     if(retry != C_UPGRADE_FAIL_RETRY) {
	//	led_all_off();
	    led_red_off();
	    led_green_on();
		DBG_PRINT("sucess!\r\n");
	//type = LED_UPDATE_FINISH;
    //   	msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT32U), MSG_PRI_NORMAL);
		while(1);
     	}
     else
     {
     DBG_PRINT("fail!\r\n");
       //  led_red_on();
	//  led_green_on();
	  Send_Upfail_Message();
	  while(1);
     }
}
