#include "drv_l1_i2c.h"
#include "drv_l2_sensor.h"
#include "drv_l1_cdsp.h"
#include "gp_aeawb.h"
//#include "cdsp_cfg.h"

#include "sensor_gc1004_iqj.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (USE_SENSOR_NAME == SENSOR_GC1004)
//================================================================//
//Set Gain
#if 0//GC1004_GC_AE
#define ANALOG_GAIN_1 64 		// 1.00x
#define ANALOG_GAIN_2 90 		// 1.4x
#define ANALOG_GAIN_3 118 		// 1.8x
#define ANALOG_GAIN_4 163 		// 2.56x
#define ANALOG_GAIN_5 218 		// 3.40x
#define ANALOG_GAIN_6 304 		// 4.7x
#define ANALOG_GAIN_7 438 		// 6.84x
#define ANALOG_GAIN_8 602 		// 9.4x
#define ANALOG_GAIN_9 851 		// 13.2x
#endif


static sensor_exposure_t gc1004_seInfo;
static sensor_calibration_t gc1004_cdsp_calibration;
static INT32U gc1004_analog_gain = 0x100;
static int *p_expTime_table;

#define C_GC1004	0
#define C_GC1014	1
#define C_GC1064	2

static INT8U u32Ratio_last=0x20;		//for gc1004
static INT8U u32Ratio_last_gc1014=0x80; //for gc1014
static INT32U R_gc1004_version = C_GC1014;
//static int sensor_max_ev_idx;
static gpCdspWBGain_t gc1004_wbgain;
/**************************************************************************
 *                         SENSEOR FUNCTION                          *
 **************************************************************************/

sensor_exposure_t *gc1004_get_senInfo(void)
{
	return &gc1004_seInfo;
}

void gc1004_sensor_calibration_str(void)
{
	gc1004_cdsp_calibration.r_b_gain = g_GC1004_r_b_gain;
	/*
	gc1004_cdsp_calibration.gamma = g_GC1004_gamma_045_table_5;
	gc1004_cdsp_calibration.color_matrix = g_GC1004_color_matrix4gamma045_7;
	gc1004_cdsp_calibration.awb_thr = g_GC1004_awb_thr_2;
	*/
}

sensor_calibration_t *gc1004_get_calibration(void)
{
	return &gc1004_cdsp_calibration;
}

void gc1004_sensor_calibration(void)
{
	//OB
	gp_Cdsp_SetBadPixOb((INT16U *)g_GC1004_badpix_ob_table);
	//Linearity
	//hwIsp_LinCorr_Table((INT8U *)g_GC1004_LiTable_rgb);
	//Lenscmp
	hwIsp_luc_MaxTan8_Slop_CLP((INT16U *)g_GC1004_MaxTan8 ,(INT16U *)g_GC1004_Slope4 ,(INT16U *)g_GC1004_CLPoint);
	hwIsp_RadusF0((INT16U *)g_GC1004_Radius_File_0);
	hwIsp_RadusF1((INT16U *)g_GC1004_Radius_File_1);		
	//Gamma
	hwCdsp_InitGamma((INT32U *)g_GC1004_gamma_045_table);
	//Color Correction
	hwCdsp_SetColorMatrix_Str((INT16S *)g_GC1004_color_matrix4gamma045);
	//AWB
	gp_Cdsp_SetAWBYUV((INT16S *)g_GC1004_awb_thr);
}

int gc1004_get_night_ev_idx(void)
{
	return gc1004_seInfo.night_ev_idx;
}

int gc1004_get_max_ev_idx(void)
{
	return gc1004_seInfo.max_ev_idx;
}

gpCdspWBGain_t *gc1004_awb_r_b_gain_boundary(void)
{
	
	int i;
	int max_r_gain, max_b_gain, min_r_gain, min_b_gain;
	
	max_r_gain = max_b_gain = 0;
	min_r_gain = min_b_gain = 255;
	
	for(i = 10 ; i < 55 ; i++)
	{
		if(max_r_gain < g_GC1004_r_b_gain[i][0]) max_r_gain = g_GC1004_r_b_gain[i][0];
		if(max_b_gain < g_GC1004_r_b_gain[i][1]) max_b_gain = g_GC1004_r_b_gain[i][1];
		if(min_r_gain > g_GC1004_r_b_gain[i][0]) min_r_gain = g_GC1004_r_b_gain[i][0];
		if(min_b_gain > g_GC1004_r_b_gain[i][1]) min_b_gain = g_GC1004_r_b_gain[i][1];
	}
	
	gc1004_wbgain.max_rgain = max_r_gain;
	gc1004_wbgain.max_bgain = max_b_gain;
	gc1004_wbgain.min_rgain = min_r_gain;
	gc1004_wbgain.min_bgain = min_b_gain;
	
	return &gc1004_wbgain;
}

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void gc1004_cvt_analog_gain(INT32U analog_gain)
{
	INT32U Analog_Multiple[9]={1000, 1400, 1800, 2560, 3400,4800,6840,9400,13200}; 

	INT32U Analog_Index;
	INT32U Digital_Gain;
	INT32U Decimal;


  INT32U u32NCurrent,u32SCurrent,u32Ratio_this,u32Ratio_this_1;
  INT32U ReadReg1,ReadReg2,ReadReg3,ReadReg4;

	gc1004_analog_gain = analog_gain*10;	 	
	
    if(gc1004_analog_gain>7200)   gc1004_analog_gain=7200; // max_gain=7.2 
    //if(gc1004_analog_gain>5200)   gc1004_analog_gain=5200; // max_gain=5.2

	Analog_Index=0;
	
	while(Analog_Index<9)
	{
	  if(gc1004_analog_gain<Analog_Multiple[Analog_Index]) 
	  {
		break;
	  }
	  else
	  {
		Analog_Index++; 
	  }
	}

	Digital_Gain = gc1004_analog_gain*1000/Analog_Multiple[Analog_Index-1];
	Decimal=(Digital_Gain*64)/1000;

#if 1
	sccb_write(GC1004_SLAVE_ID, 0xb1,  Decimal>>6); 
	sccb_write(GC1004_SLAVE_ID, 0xb2,  (Decimal<<2)&0xfc);
	sccb_write(GC1004_SLAVE_ID, 0xb6,   Analog_Index-1);
#else
	sccb_write(GC1004_SLAVE_ID, 0xb1, 0x01); 
	sccb_write(GC1004_SLAVE_ID, 0xb2, 0x00);
	sccb_write(GC1004_SLAVE_ID, 0xb6, 0x06);
#endif

   ReadReg1 =sccb_read(GC1004_SLAVE_ID, 0x56);
   ReadReg2 =sccb_read(GC1004_SLAVE_ID, 0xd6);
   ReadReg3 =sccb_read(GC1004_SLAVE_ID, 0x46);
   ReadReg4 =sccb_read(GC1004_SLAVE_ID, 0xc6);
   
  
   u32NCurrent = ReadReg1 | (ReadReg2<<8);
   u32SCurrent = ReadReg3 | (ReadReg4<<8);

 //  DBG_PRINT("u32NCurrent = %d, u32SCurrent = %d\r\n", u32NCurrent, u32SCurrent);

   u32Ratio_this_1 =( (u32NCurrent * 10 + u32SCurrent*11) * 32*100 / (21*u32NCurrent)); 	//k=1.1
   u32Ratio_this=u32Ratio_this_1/100;
   
  // DBG_PRINT("u32Ratio_this_1 = %d, u32Ratio_this = %d\r\n", u32Ratio_this_1, u32Ratio_this);
   
   if(u32Ratio_this > 32)	 u32Ratio_this = 32;			
   
	  u32Ratio_this = (u32Ratio_last + u32Ratio_this) / 2 -1;	 //a + b = 10	
	  
   //DBG_PRINT("u32Ratio_this*******000000 = %d\r\n", u32Ratio_this);
	  
   if(u32Ratio_this != u32Ratio_last)
   {
	  sccb_write(GC1004_SLAVE_ID, 0x66, u32Ratio_this); 
   }	   
   
	u32Ratio_last = u32Ratio_this;
	
  
  if(gc1004_analog_gain>4000)
	{
		 if(u32Ratio_this>0x1c)  u32Ratio_this=0x1c;
     	sccb_write(GC1004_SLAVE_ID, 0x66,  u32Ratio_this); 
	}
	if(gc1004_analog_gain<3250)
  {
  	 if(u32Ratio_this>0x20) u32Ratio_this=0x20;
      sccb_write(GC1004_SLAVE_ID, 0x66,  u32Ratio_this); 
	}



}

static void gc1014_cvt_analog_gain(INT32U analog_gain)
{
    INT32U Analog_Multiple[9]={1000, 1706, 2000, 3414, 4026,6870,8127,13869,19610};

	INT32U Analog_Index;
	INT32U Digital_Gain;
	INT32U Decimal;


    INT32U u32NCurrent,u32SCurrent,u32Ratio_this,u32Ratio_this_1;
    INT32U ReadReg1,ReadReg2,ReadReg3,ReadReg4;
  

	gc1004_analog_gain = analog_gain*10;	 	
	
    if(gc1004_analog_gain>7200)   gc1004_analog_gain=7200; // max_gain=7.2 
    //if(gc1004_analog_gain>5200)   gc1004_analog_gain=5200; // max_gain=5.2

	Analog_Index=0;
	
	while(Analog_Index<9)
	{
	  if(gc1004_analog_gain<Analog_Multiple[Analog_Index]) 
	  {
		break;
	  }
	  else
	  {
		Analog_Index++; 
	  }
	}

	Digital_Gain = gc1004_analog_gain*1000/Analog_Multiple[Analog_Index-1];
	Decimal=(Digital_Gain*64)/1000;

#if 1
	sccb_write(GC1004_SLAVE_ID, 0xb1,  Decimal>>6); 
	sccb_write(GC1004_SLAVE_ID, 0xb2,  (Decimal<<2)&0xfc);
	sccb_write(GC1004_SLAVE_ID, 0xb6,   Analog_Index-1);
#else
	sccb_write(GC1004_SLAVE_ID, 0xb1, 0x01); 
	sccb_write(GC1004_SLAVE_ID, 0xb2, 0x00);
	sccb_write(GC1004_SLAVE_ID, 0xb6, 0x00);	
#endif

   ReadReg1 =sccb_read(GC1004_SLAVE_ID, 0x56);
   ReadReg2 =sccb_read(GC1004_SLAVE_ID, 0xd6);
   ReadReg3 =sccb_read(GC1004_SLAVE_ID, 0x46);
   ReadReg4 =sccb_read(GC1004_SLAVE_ID, 0xc6);
   
  
   u32NCurrent = ReadReg1 | (ReadReg2<<8);
   u32SCurrent = ReadReg3 | (ReadReg4<<8);

   //DBG_PRINT("u32NCurrent = %d, u32SCurrent = %d\r\n", u32NCurrent, u32SCurrent);

   u32Ratio_this_1 =( (u32NCurrent * 10 + u32SCurrent*11) * 32*100 / (21*u32NCurrent)); 	//k=1.1
   u32Ratio_this=u32Ratio_this_1*4/100;
  
   
  // DBG_PRINT("u32Ratio_this_1 = %d, u32Ratio_this = %d\r\n", u32Ratio_this_1, u32Ratio_this);
   
   if(u32Ratio_this > 0x80)	 u32Ratio_this = 0x80;			
   
	  u32Ratio_this = (u32Ratio_last_gc1014 + u32Ratio_this) / 2;	 //a + b = 10	
	  
   //DBG_PRINT("u32Ratio_this*******000000 = %d\r\n", u32Ratio_this);
	  
   if(u32Ratio_this != u32Ratio_last_gc1014)
   {
	  sccb_write(GC1004_SLAVE_ID, 0x66, u32Ratio_this+1); 
   }	   
   
	u32Ratio_last_gc1014 = u32Ratio_this;
	
  	
  



}

static void gc1064_cvt_analog_gain(INT32U analog_gain)  //GC1064
{
    INT32U Analog_Multiple[9]={1000, 1650, 1870, 3080, 3500,5820,6700,10700,15800};

	INT32U Analog_Index;
	INT32U Digital_Gain;
	INT32U Decimal;


  INT32U u32NCurrent,u32SCurrent,u32Ratio_this,u32Ratio_this_1;
  INT32U ReadReg1,ReadReg2,ReadReg3,ReadReg4;

	gc1004_analog_gain = analog_gain*10;	 	
	
    if(gc1004_analog_gain>7200)   gc1004_analog_gain=7200; // max_gain=7.2 
    //if(gc1004_analog_gain>5200)   gc1004_analog_gain=5200; // max_gain=5.2

	Analog_Index=0;
	
	while(Analog_Index<9)
	{
	  if(gc1004_analog_gain<Analog_Multiple[Analog_Index]) 
	  {
		break;
	  }
	  else
	  {
		Analog_Index++; 
	  }
	}

	Digital_Gain = gc1004_analog_gain*1000/Analog_Multiple[Analog_Index-1];
	Decimal=(Digital_Gain*64)/1000;

#if 1
	sccb_write(GC1004_SLAVE_ID, 0xb1,  Decimal>>6); 
	sccb_write(GC1004_SLAVE_ID, 0xb2,  (Decimal<<2)&0xfc);
	sccb_write(GC1004_SLAVE_ID, 0xb6,   Analog_Index-1);
#else
	sccb_write(GC1004_SLAVE_ID, 0xb1, 0x01); 
	sccb_write(GC1004_SLAVE_ID, 0xb2, 0x00);
	sccb_write(GC1004_SLAVE_ID, 0xb6, 0x06);
#endif

   ReadReg1 =sccb_read(GC1004_SLAVE_ID, 0x56);
   ReadReg2 =sccb_read(GC1004_SLAVE_ID, 0xd6);
   ReadReg3 =sccb_read(GC1004_SLAVE_ID, 0x46);
   ReadReg4 =sccb_read(GC1004_SLAVE_ID, 0xc6);
   
  
   u32NCurrent = ReadReg1 | (ReadReg2<<8);
   u32SCurrent = ReadReg3 | (ReadReg4<<8);

   //DBG_PRINT("u32NCurrent = %d, u32SCurrent = %d\r\n", u32NCurrent, u32SCurrent);

   u32Ratio_this_1 =( (u32NCurrent * 10 + u32SCurrent*11) * 32*100 / (21*u32NCurrent)); 	//k=1.1
   u32Ratio_this=u32Ratio_this_1*4/100;
  
   
  // DBG_PRINT("u32Ratio_this_1 = %d, u32Ratio_this = %d\r\n", u32Ratio_this_1, u32Ratio_this);
   
   if(u32Ratio_this > 0x80)	 u32Ratio_this = 0x80;			
   
	  u32Ratio_this = (u32Ratio_last_gc1014 + u32Ratio_this) / 2;	 //a + b = 10	
	  
   //DBG_PRINT("u32Ratio_this*******000000 = %d\r\n", u32Ratio_this);
	  
   if(u32Ratio_this != u32Ratio_last_gc1014)
   {
	  sccb_write(GC1004_SLAVE_ID, 0x66, u32Ratio_this+1); 
   }	   
   
	u32Ratio_last_gc1014 = u32Ratio_this;
	
}

int gc1004_set_exposure_time(sensor_exposure_t *si)
{
	//int ret=0;
	//unsigned short tmp;
	//int analog_gain;
	//unsigned char cvt_digital_gain;
	//int digital_gain;
	int lsb_time, msb_time;
	int idx;

	// From agoritham calc new data update to gc1004_seInfo.
	gc1004_seInfo.sensor_ev_idx += si->ae_ev_idx;
	if(gc1004_seInfo.sensor_ev_idx >= gc1004_seInfo.max_ev_idx) gc1004_seInfo.sensor_ev_idx = gc1004_seInfo.max_ev_idx;
	if(gc1004_seInfo.sensor_ev_idx < 0) gc1004_seInfo.sensor_ev_idx = 0;
	
	idx = gc1004_seInfo.sensor_ev_idx * 3;
	gc1004_seInfo.time = p_expTime_table[idx];
	gc1004_seInfo.analog_gain = p_expTime_table[idx+1];
	gc1004_seInfo.digital_gain = p_expTime_table[idx+2];
	
	gc1004_seInfo.userISO = si->userISO;

	//DBG_PRINT("T %d, ag %d, ev %d.\r\n", gc1004_seInfo.time, gc1004_seInfo.analog_gain, gc1004_seInfo.sensor_ev_idx );
	//DBG_PRINT("Time = %d, a gain = %d, d gain = %d, ev idx = %d [%d]\r\n", gc1004_seInfo.time, gc1004_seInfo.analog_gain, gc1004_seInfo.digital_gain, gc1004_seInfo.sensor_ev_idx, si->ae_ev_idx );

	// set exposure time
		lsb_time = (gc1004_seInfo.time & 0xFF);
		msb_time = ((gc1004_seInfo.time >>8 )& 0xFF);

		#if 1
			sccb_write(GC1004_SLAVE_ID, 0x04 , lsb_time );
			sccb_write(GC1004_SLAVE_ID, 0x03 , msb_time );
		#else
			sccb_write(GC1004_SLAVE_ID, 0x04 , 0x34 );
			sccb_write(GC1004_SLAVE_ID, 0x03 , 0x02 );
		#endif
		
		
	return 0;
}

void gc1004_BLK_Cal(void)
{

	INT32U u32NCurrent,u32SCurrent;
	INT32U u32Ratio_this,u32Ratio_last;

		
	u32NCurrent = sccb_read(GC1004_SLAVE_ID,0x56) | (sccb_read(GC1004_SLAVE_ID,0xd6)<<8);
	u32SCurrent = sccb_read(GC1004_SLAVE_ID,0x46) | (sccb_read(GC1004_SLAVE_ID,0xc6)<<8);//----------add 20141201
	
	if(u32NCurrent > 0)
	{
	    u32Ratio_this = (u32NCurrent * 10 + 11*u32SCurrent) * 32 /( 21*u32NCurrent);	   //k=1.1
	
		if(u32Ratio_this > 32)	 u32Ratio_this = 32;	
			
		u32Ratio_this = (u32Ratio_last + u32Ratio_this) / 2;	//a + b = 10
		
		  sccb_write(GC1004_SLAVE_ID,0x66,u32Ratio_this);
	 }
	    u32Ratio_last = u32Ratio_this;

}

void gc1004_set_exposure_gain(void)
{
	//int digital_gain_tmp;
	/*digital_gain_tmp =  0x60;((gc1004_seInfo.digital_gain  >> 2) & 0xFF);	//0x40:1x, 0xff:4x
	sccb_write(GC1004_SLAVE_ID, 0xb0, digital_gain_tmp);*/
	if (R_gc1004_version == C_GC1064)
	{
		gc1064_cvt_analog_gain(gc1004_seInfo.analog_gain);
	}
	else if (R_gc1004_version == C_GC1004)
	{
		gc1004_cvt_analog_gain(gc1004_seInfo.analog_gain);
	}
	else
	{
		gc1014_cvt_analog_gain(gc1004_seInfo.analog_gain);
	}
	//DBG_PRINT("G");
}


void gc1004_get_exposure_time(sensor_exposure_t *se)
{
	//int ret=0;
	gp_memcpy((INT8S *)se, (INT8S *)&gc1004_seInfo, sizeof(sensor_exposure_t));

}

void gc1004_set_exp_freq(int freq)
{
	if(freq == 50)
	{
		gc1004_seInfo.sensor_ev_idx = GC1004_50HZ_INIT_EV_IDX;
		gc1004_seInfo.ae_ev_idx = 0;
		gc1004_seInfo.daylight_ev_idx= GC1004_50HZ_DAY_EV_IDX;
		gc1004_seInfo.night_ev_idx= GC1004_50HZ_NIGHT_EV_IDX;
		
		gc1004_seInfo.max_ev_idx = GC1004_50HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_GC1004_exp_time_gain_50Hz;
	}
	else if(freq == 60)
	{
		gc1004_seInfo.sensor_ev_idx = GC1004_60HZ_INIT_EV_IDX;
		gc1004_seInfo.ae_ev_idx = 0;
		gc1004_seInfo.daylight_ev_idx= GC1004_60HZ_DAY_EV_IDX;
		gc1004_seInfo.night_ev_idx= GC1004_60HZ_NIGHT_EV_IDX;
		gc1004_seInfo.max_ev_idx = GC1004_60HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_GC1004_exp_time_gain_60Hz;
	}
}

static int gc1004_init(void)
{
	gc1004_seInfo.max_time = GC1004_MAX_EXPOSURE_TIME;
	gc1004_seInfo.min_time = GC1004_MIN_EXPOSURE_TIME;

	gc1004_seInfo.max_digital_gain = GC1004_MAX_DIGITAL_GAIN ;
	gc1004_seInfo.min_digital_gain = GC1004_MIN_DIGITAL_GAIN ;

	gc1004_seInfo.max_analog_gain = GC1004_MAX_ANALOG_GAIN;
	gc1004_seInfo.min_analog_gain = GC1004_MIN_ANALOG_GAIN;

	gc1004_seInfo.analog_gain = gc1004_seInfo.min_analog_gain;
	gc1004_seInfo.digital_gain = gc1004_seInfo.min_digital_gain;
	gc1004_seInfo.time = gc1004_seInfo.max_time;// >> 1;
	gc1004_seInfo.userISO = ISO_AUTO;

	gc1004_set_exp_freq(50);
	
	DBG_PRINT("gc1004_init\r\n");
	return 0;
}

void sensor_gc1004_init(INT32U WIDTH, INT32U HEIGHT)
{
	//i2c_bus_handle_t i2c_handle; 
	INT32U i;
	INT8U reg_tmp, data_tmp;
	/*
	  ?????3]-¡§¡§
	*/
	//i2c_handle.slaveAddr = 0x60;
	//i2c_handle.clkRate = 100;
	
	gc1004_init();
	gc1004_sensor_calibration_str();
	if (1)
	{//?D???¨´?¨®¦Ì?¨¦???¨ª¡¤¨º?GC1004, ?1¨º?GC1014.
		INT32U t;
		INT32U t1;
		
		t = sccb_read(GC1004_SLAVE_ID, 0xF0);
		t1 = sccb_read(GC1004_SLAVE_ID, 0xF1);
		
		if ((t==0x10)&&(t1==0x24))
		{
			R_gc1004_version = C_GC1064;
		}
		else
		{
			sccb_write(GC1004_SLAVE_ID, 0xFE, 0x00);
			sccb_write(GC1004_SLAVE_ID, 0xF9, 0x0F);
			t =sccb_read(GC1004_SLAVE_ID, 0x26);
			//DBG_PRINT("Sencer_id=%d\r\n",t);
			if (t == 0x00)
			{
				R_gc1004_version = C_GC1004;
			}
			else
			{
				R_gc1004_version = C_GC1014;
			}
		}
	}
	if(sensor_format == GC1004_RAW){
		if(WIDTH == 1280 && HEIGHT == 720)
		{
		   //+++++++++++++++++++++++++++++ 20151202
			if (R_gc1004_version == C_GC1064)
			{
				for (i=0; i<sizeof(GC1064_Para_720P_30)/2; i++) 
				{
					sccb_write(GC1004_SLAVE_ID, GC1064_Para_720P_30[i][0], GC1064_Para_720P_30[i][1]);
				}
			}
			else if (R_gc1004_version == C_GC1014)
			{
				for (i=0; i<sizeof(GC1014_Para_720P_30)/2; i++) 
				{
					sccb_write(GC1004_SLAVE_ID, GC1014_Para_720P_30[i][0], GC1014_Para_720P_30[i][1]);
				}
			}
			else
			{
		   //----------------------------- 20151202
			for (i=0; i<sizeof(GC1004_Para_720P_30)/2; i++) 
			{
				//reg_1byte_data_1byte_write(&i2c_handle,GC1004_Para_720P_30[i][0], GC1004_Para_720P_30[i][1]);
				sccb_write(GC1004_SLAVE_ID, GC1004_Para_720P_30[i][0], GC1004_Para_720P_30[i][1]);
			}
			}
		}			
		else 
		{
			while(1);
		}
	}else if(sensor_format == GC1004_MIPI){
		if(WIDTH == 1280 && HEIGHT == 800)
		{
			for (i=0; i<sizeof(GC1004_MIPI_1280_800_30)/2; i++) 
			{
				//reg_1byte_data_1byte_write(&i2c_handle,GC1004_MIPI_1280_800_30[i][0], GC1004_MIPI_1280_800_30[i][1]);
				sccb_write(GC1004_SLAVE_ID,GC1004_MIPI_1280_800_30[i][0], GC1004_MIPI_1280_800_30[i][1]);
			}
		}	
		else if	(WIDTH == 1280 && HEIGHT == 720)
		{
			if (R_gc1004_version == C_GC1064)
			{
				for (i=0; i<sizeof(GC1064_MIPI_720P)/2; i++) 
				{
					reg_tmp = GC1064_MIPI_720P[i][0];
					data_tmp =  GC1064_MIPI_720P[i][1];
					sccb_write(GC1004_SLAVE_ID,GC1064_MIPI_720P[i][0], GC1064_MIPI_720P[i][1]);
					#if 1
					if (reg_tmp == 0x10 && data_tmp == 0x80)
					{
						 drv_msec_wait(200);//90); //wait 1 is 1ms for CLKO stable
					}
					#endif				
				}
			}
			else if (R_gc1004_version == C_GC1004)
			{
				for (i=0; i<sizeof(GC1004_MIPI_720P)/2; i++) 
				{
					reg_tmp = GC1004_MIPI_720P[i][0];
					data_tmp =  GC1004_MIPI_720P[i][1];
					sccb_write(GC1004_SLAVE_ID,GC1004_MIPI_720P[i][0], GC1004_MIPI_720P[i][1]);
					#if 1
					if (reg_tmp == 0x10 && data_tmp == 0x80)
					{
						 drv_msec_wait(200);//90); //wait 1 is 1ms for CLKO stable
					}
					#endif				
				}
			}
			else
				{
				         for (i=0; i<sizeof(GC1014_MIPI_720P)/2; i++) 
			         {
				       reg_tmp = GC1014_MIPI_720P[i][0];
				       data_tmp =  GC1014_MIPI_720P[i][1];
				       sccb_write(GC1004_SLAVE_ID,GC1014_MIPI_720P[i][0], GC1014_MIPI_720P[i][1]);
         #if 1
				       if (reg_tmp == 0x10 && data_tmp == 0x80)
				        {
					       //gpio_write_io(IO_A14, 1);	//pull Low					
					       //sccb_delay(200);//120us
					       drv_msec_wait(200);//90); //wait 1 is 1ms for CLKO stable
					       //gpio_write_io(IO_A14, 0);	//pull high
				        }
         #endif				
			        }
				}
		}	
		else if(WIDTH == 640 && HEIGHT == 480)
		{
			for (i=0; i<sizeof(GC1004_MIPI_VGA_f60)/2; i++) 
			{
				//reg_1byte_data_1byte_write(&i2c_handle,GC1004_MIPI_VGA_f60[i][0], GC1004_MIPI_VGA_f60[i][1]);
				sccb_write(GC1004_SLAVE_ID,GC1004_MIPI_VGA_f60[i][0], GC1004_MIPI_VGA_f60[i][1]);
			}			
		}		
		else 
		{
			while(1);
		}
	}
}

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(USE_SENSOR_NAME == SENSOR_GC1004)     //
//================================================================//
