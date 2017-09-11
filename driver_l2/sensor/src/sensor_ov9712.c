#include "drv_l1_sensor.h"
#include "drv_l1_i2c.h"
#include "drv_l1_cdsp.h"

#if CDSP_IQ == 3 || CDSP_IQ == 4
#include "sensor_ov9712_iqp.h"
#elif CDSP_IQ == 5
#include "sensor_ov9712_iqw.h"
#elif CDSP_IQ == 2
#include "sensor_ov9712_iqj.h"
#else
#include "sensor_ov9712.h"
#endif 

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (USE_SENSOR_NAME == SENSOR_OV9712)
//================================================================//
static sensor_exposure_t ov9712_seInfo;
static sensor_calibration_t ov9712_cdsp_calibration;
static int *p_expTime_table;
extern void sensor_set_fps(INT32U fpsValue);
extern INT32U sensor_get_fps(void);

static gpCdspWBGain_t ov9712_wbgain;
/**************************************************************************
 *                         SENSEOR FUNCTION                          *
 **************************************************************************/
sensor_exposure_t *ov9712_get_senInfo(void)
{
	return &ov9712_seInfo;
}

void ov9712_sensor_calibration_str(void)
{
	ov9712_cdsp_calibration.r_b_gain = OV9712_r_b_gain;
	//ov9712_cdsp_calibration.gamma = OV9712_gamma_045_table;
	//ov9712_cdsp_calibration.color_matrix = OV9712_color_matrix4gamma045;
	//ov9712_cdsp_calibration.awb_thr = OV9712_awb_thr;
}


sensor_calibration_t *ov9712_get_calibration(void)
{
	return &ov9712_cdsp_calibration;
}

void ov9712_sensor_calibration(void)
{
	//OB
	gp_Cdsp_SetBadPixOb((INT16U *)OV9712_badpix_ob_table);
	/*
	//Linearity
	hwIsp_LinCorr_Table((INT8U *)g_OV9712_LiTable_rgb);
	
	//Lenscmp
	hwIsp_luc_MaxTan8_Slop_CLP((INT16U *)g_OV9712_MaxTan8 ,(INT16U *)g_OV9712_Slope4 ,(INT16U *)g_OV9712_CLPoint);
	hwIsp_RadusF0((INT16U *)g_OV9712_Radius_File_0);
	hwIsp_RadusF1((INT16U *)g_OV9712_Radius_File_1);		
	*/
	//Gamma
	hwCdsp_InitGamma((INT32U *)OV9712_gamma_045_table);
	//Color Correction
	hwCdsp_SetColorMatrix_Str((INT16S *)OV9712_color_matrix4gamma045);
	//AWB
	gp_Cdsp_SetAWBYUV((INT16S *)OV9712_awb_thr);
}

gpCdspWBGain_t *ov9712_awb_r_b_gain_boundary(void)
{
	int i;
	int max_r_gain, max_b_gain, min_r_gain, min_b_gain;
	
	max_r_gain = max_b_gain = 0;
	min_r_gain = min_b_gain = 255;
	
	for(i = 10 ; i < 55 ; i++)
	{
		if(max_r_gain < OV9712_r_b_gain[i][0]) max_r_gain = OV9712_r_b_gain[i][0];
		if(max_b_gain < OV9712_r_b_gain[i][1]) max_b_gain = OV9712_r_b_gain[i][1];
		if(min_r_gain > OV9712_r_b_gain[i][0]) min_r_gain = OV9712_r_b_gain[i][0];
		if(min_b_gain > OV9712_r_b_gain[i][1]) min_b_gain = OV9712_r_b_gain[i][1];
	}
	
	ov9712_wbgain.max_rgain = max_r_gain;
	ov9712_wbgain.max_bgain = max_b_gain;
	ov9712_wbgain.min_rgain = min_r_gain;
	ov9712_wbgain.min_bgain = min_b_gain;
	
	return &ov9712_wbgain;
}

static int OV9712_analog_gain_table[65] = 
{
	// coarse gain = 0
	(int)(1.00*256+0.5), (int)(1.06*256+0.5), (int)(1.13*256+0.5), (int)(1.19*256+0.5), 
	(int)(1.25*256+0.5), (int)(1.31*256+0.5), (int)(1.38*256+0.5), (int)(1.44*256+0.5), 
	(int)(1.50*256+0.5), (int)(1.56*256+0.5), (int)(1.63*256+0.5), (int)(1.69*256+0.5), 
	(int)(1.75*256+0.5), (int)(1.81*256+0.5), (int)(1.88*256+0.5), (int)(1.94*256+0.5),
	
	// coarse gain = 1
	(int)(2.00*256+0.5), (int)(2.13*256+0.5), (int)(2.25*256+0.5), (int)(2.38*256+0.5), 
	(int)(2.50*256+0.5), (int)(2.63*256+0.5), (int)(2.75*256+0.5), (int)(2.88*256+0.5),
	(int)(3.00*256+0.5), (int)(3.13*256+0.5), (int)(3.25*256+0.5), (int)(3.38*256+0.5),
	(int)(3.50*256+0.5), (int)(3.63*256+0.5), (int)(3.75*256+0.5), (int)(3.88*256+0.5),

	// coarse gain = 3
	(int)(4.00*256+0.5), (int)(4.25*256+0.5), (int)(4.50*256+0.5), (int)(4.75*256+0.5), 
	(int)(5.00*256+0.5), (int)(5.25*256+0.5), (int)(5.50*256+0.5), (int)(5.75*256+0.5), 
	(int)(6.00*256+0.5), (int)(6.25*256+0.5), (int)(6.50*256+0.5), (int)(6.75*256+0.5), 
	(int)(7.00*256+0.5), (int)(7.25*256+0.5), (int)(7.50*256+0.5), (int)(7.75*256+0.5), 
	
	// coarse gain = 7
	(int)(8.00*256+0.5), (int)(8.50*256+0.5), (int)(9.00*256+0.5), (int)(9.50*256+0.5), 
	(int)(10.00*256+0.5),(int)(10.50*256+0.5),(int)(11.00*256+0.5),(int)(11.50*256+0.5), 
	(int)(12.00*256+0.5),(int)(12.50*256+0.5),(int)(13.00*256+0.5),(int)(13.50*256+0.5), 
	(int)(14.00*256+0.5),(int)(14.50*256+0.5),(int)(15.00*256+0.5),(int)(15.50*256+0.5), 
	
	// coarse gain = 15
	(int)(16.00*256+0.5)
};

static int OV9712_cvt_analog_gain(int analog_gain)
{
	int i;
	int coarse_gain, fine_gain;
	int *p_OV9712_analog_gain_table = OV9712_analog_gain_table;

	for( i = 0 ; i < 65 ; i++)
	{
		if(analog_gain >= p_OV9712_analog_gain_table[i] && analog_gain < p_OV9712_analog_gain_table[i+1])
			break;
	} 

	if( i < 16 )
	{
		coarse_gain = 0;
		fine_gain = i;
	}
	else if(i < (16 + 16))
	{
		coarse_gain = 1;
		fine_gain = (i - 16); 
	}
	else if(i < (16 + 16 + 16))
	{
		coarse_gain = 3;
		fine_gain = (i - 16 - 16);
	}
	else if(i < (16 + 16 + 16 + 16))
	{
		coarse_gain = 7;
		fine_gain = (i - 16 - 16 -16);
	}
	else
	{
		coarse_gain = 15;
		fine_gain = 0;
	}

	return ((coarse_gain << 4) | fine_gain);
}


int ov9712_get_night_ev_idx(void)
{
	return ov9712_seInfo.night_ev_idx;
}

int ov9712_get_max_ev_idx(void)
{
	return ov9712_seInfo.max_ev_idx;
}

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

int ov9712_set_exposure_time(sensor_exposure_t *si)
{
	INT32U ret = 0;
	//unsigned short tmp;
	//int analog_gain, digital_gain;
	int lsb_time, msb_time;
	int idx;
	//unsigned char cvt_digital_gain;

#if 1	// From agoritham calc new data update to ov9712_seInfo.
	ov9712_seInfo.sensor_ev_idx += si->ae_ev_idx;
	if(ov9712_seInfo.sensor_ev_idx >= ov9712_seInfo.max_ev_idx) ov9712_seInfo.sensor_ev_idx = ov9712_seInfo.max_ev_idx;
	if(ov9712_seInfo.sensor_ev_idx < 0) ov9712_seInfo.sensor_ev_idx = 0;
	
	idx = ov9712_seInfo.sensor_ev_idx * 3;
	ov9712_seInfo.time = p_expTime_table[idx];
	ov9712_seInfo.analog_gain = p_expTime_table[idx+1];
	ov9712_seInfo.digital_gain = p_expTime_table[idx+2];
	
	ov9712_seInfo.userISO = si->userISO;
#endif	
	//DBG_PRINT("Time = %d, a gain = %d, d gain = %d, ev idx = %d\r\n", ov9712_seInfo.time, ov9712_seInfo.analog_gain, ov9712_seInfo.digital_gain, ov9712_seInfo.sensor_ev_idx);

	ret =(sccb_read(OV9712_ID, 0x04) & ~0x01);
	
	sccb_write(OV9712_ID, 0x04, 0x09|ret);			//group write on

	if (( 0x354 < ov9712_seInfo.time)&&(ov9712_seInfo.time <= 0x410))
	{
		if(sensor_get_fps() != 25)
		{
			sensor_set_fps(25);	
		}
	}
	else if (ov9712_seInfo.time > 0x410)//0x400)
	{
		if(sensor_get_fps() != 20)
		{
			sensor_set_fps(20);			
		}
	}
	else
	{
		if(sensor_get_fps() != 30)
		{
			sensor_set_fps(30);				
		}
	}

	lsb_time = (ov9712_seInfo.time & 0xFF);
	msb_time = ((ov9712_seInfo.time >>8 )& 0xFF);
	sccb_write(OV9712_ID, 0x10 , lsb_time );
	sccb_write(OV9712_ID, 0x16 , msb_time );
	
	// set Analog gain	
	//sccb_write(OV9712_ID, 0x00, analog_gain ); 
	//DBG_PRINT("<OV9712_set_exposure_time> Lib_analog_gain = 0x%x, cvt_analog_gain = 0x%x\r\n",ov9712_seInfo.analog_gain, analog_gain);
		
	// set Digital gain
	//with cdsp global gain
	//digital_gain =ov9712_seInfo.digital_gain >> 3;
	//hwCdsp_SetGlobalGain(digital_gain);	
	//DBG_PRINT("<OV9712_set_exposure_time> digital gain = 0x%x, cvt_digital_gain: 0x%x\r\n",ov9712_seInfo.digital_gain, digital_gain);


	//DBG_PRINT("\r\n<OV9712_set_exposure_time> time = %d, Lsb_time: 0x%x, Msb_time: 0x%x\r\n",ov9712_seInfo.time,lsb_time,msb_time);
	
	return 0;
}


void ov9712_set_exposure_gain(void)
{
	int analog_gain, digital_gain;
	
	analog_gain = OV9712_cvt_analog_gain(ov9712_seInfo.analog_gain);
	sccb_write(OV9712_ID, 0x00, analog_gain );
	sccb_write(OV9712_ID, 0xff, 0xff);			//group write trigger
	
	digital_gain =ov9712_seInfo.digital_gain >> 3;
	hwCdsp_SetGlobalGain(digital_gain);	
}

void ov9712_group_off(void)
{
	INT32U ret;
	ret =(sccb_read(OV9712_ID, 0x04) & ~0x01);
	sccb_write(OV9712_ID, 0x04, 0x09|ret);
	sccb_write(OV9712_ID, 0x04, 0x08|ret);			//group write off
	sccb_write(OV9712_ID, 0xff, 0xff);			//group write trigger
}

void ov9712_get_exposure_time(sensor_exposure_t *se)
{
	gp_memcpy((INT8S *)se, (INT8S *)&ov9712_seInfo, sizeof(sensor_exposure_t));

}

void ov9712_set_exp_freq(int freq)
{
	if(freq == 50)
	{
		ov9712_seInfo.sensor_ev_idx = OV9712_50HZ_INIT_EV_IDX;
		ov9712_seInfo.ae_ev_idx = 0;
		ov9712_seInfo.daylight_ev_idx= OV9712_50HZ_DAY_EV_IDX;
		ov9712_seInfo.night_ev_idx= OV9712_50HZ_NIGHT_EV_IDX;
		ov9712_seInfo.max_ev_idx = OV9712_50HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_OV9712_exp_time_gain_50Hz;
	}
	else if(freq == 60)
	{
		ov9712_seInfo.sensor_ev_idx = OV9712_60HZ_INIT_EV_IDX;
		ov9712_seInfo.ae_ev_idx = 0;
		ov9712_seInfo.daylight_ev_idx= OV9712_60HZ_DAY_EV_IDX;
		ov9712_seInfo.night_ev_idx= OV9712_60HZ_NIGHT_EV_IDX;
		ov9712_seInfo.max_ev_idx = OV9712_60HZ_MAX_EXP_IDX - 1;
		p_expTime_table = (int *)g_OV9712_exp_time_gain_60Hz;
	}
}

static int ov9712_init(void)
{
	ov9712_seInfo.max_time = OV9712_MAX_EXPOSURE_TIME;
	ov9712_seInfo.min_time = OV9712_MIN_EXPOSURE_TIME;

	ov9712_seInfo.max_digital_gain = OV9712_MAX_DIGITAL_GAIN ;
	ov9712_seInfo.min_digital_gain = OV9712_MIN_DIGITAL_GAIN ;

	ov9712_seInfo.max_analog_gain = OV9712_MAX_ANALOG_GAIN;
	ov9712_seInfo.min_analog_gain = OV9712_MIN_ANALOG_GAIN;

	ov9712_seInfo.analog_gain = ov9712_seInfo.min_analog_gain;
	ov9712_seInfo.digital_gain = ov9712_seInfo.min_digital_gain;
	ov9712_seInfo.time = ov9712_seInfo.max_time;// >> 1;

	ov9712_set_exp_freq(50);
	
	ov9712_seInfo.userISO = ISO_AUTO;
	
	DBG_PRINT("ov9712_init\r\n");
	return 0;
}

void sensor_ov9712_init(INT32U format, INT32U width, INT32U height)
{
	//i2c_bus_handle_t i2c_handle; 
	INT32U i;

	/*
	  �ѼƳ]��
	*/
	//i2c_handle.slaveAddr = 0x60;
	//i2c_handle.clkRate = 100;
	
	ov9712_init();
	ov9712_sensor_calibration_str();
	
	switch(format)
	{
		case OV9712_RAW:
		
			if(width == 1280 && height == 800)
			{
				for (i=0; i<sizeof(OV9712_1280_800_F30)/2; i++) 
				{
					sccb_write(OV9712_ID,OV9712_1280_800_F30[i][0], OV9712_1280_800_F30[i][1]);				
				}
				break;
			}	
			else if	(width == 1280 && height == 720)
			{
				/*		
				for (i=0; i<sizeof(OV9712_720P_F30)/2; i++) 
				{
					sccb_write(OV9712_ID,OV9712_720P_F30[i][0], OV9712_720P_F30[i][1]);
					//reg_1byte_data_1byte_write(&i2c_handle,OV9712_720P_F30[i][0], OV9712_720P_F30[i][1]);
				}
				*/
				
				for (i=0; i<sizeof(OV9712_720P_F30_A24)/2; i++) 
				//for (i=0; i<sizeof(OV9712_720P_F30)/2; i++) 
				{
					//sccb_write(OV9712_ID,OV9712_720P_F30[i][0], OV9712_720P_F30[i][1]);					
					sccb_write(OV9712_ID,OV9712_720P_F30_A24[i][0], OV9712_720P_F30_A24[i][1]);
				}
				
				break;
			}	
			else if(width == 640 && height == 480)
			{
				for (i=0; i<sizeof(OV9712_VGA_F30)/2; i++) 
				{
					sccb_write(OV9712_ID, OV9712_VGA_F30[i][0], OV9712_VGA_F30[i][1]);
					//reg_1byte_data_1byte_write(&i2c_handle,OV9712_VGA_F30[i][0], OV9712_VGA_F30[i][1]);
				}
				break;
			}		
			else 
			{
				while(1);
			}
	}
}

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //(USE_SENSOR_NAME == SENSOR_OV9712)     //
//================================================================//
