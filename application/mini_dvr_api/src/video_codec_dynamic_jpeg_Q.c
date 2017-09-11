#include "my_avi_encoder_state.h"


extern INT32S current_Y_Q_value;
extern INT32S target_Y_Q_value;

extern INT32S current_UV_Q_value;
extern INT32S target_UV_Q_value;

extern INT32U current_VLC_size;
extern INT32U max_VLC_size;

/****************************************************************************/
/*
 *	Dynamic_Tune_Q
 */
void Dynamic_Tune_Q(INT32U jpeg_size, INT32U full_size_flag)
{

        if ( (my_pAviEncVidPara->encode_width == AVI_WIDTH_1080FHD) ||(my_pAviEncVidPara->encode_width == AVI_WIDTH_1080P) )
		{
 			if (full_size_flag)
			{
				current_Y_Q_value -= 20;
			}
			else if (jpeg_size > (max_VLC_size-(40*1024)))
			{
				current_Y_Q_value -= 5;
			}
			else if (jpeg_size < (max_VLC_size-(60*1024)))
			{
				current_Y_Q_value += 5;
			}

			if(current_Y_Q_value < 10)
			{
				current_Y_Q_value = 10;
			}
 		}
		else	 // other size(720p)
		{
			if (full_size_flag)
			{
				current_Y_Q_value -= 15;	
			}
			else if (jpeg_size > (max_VLC_size-(25*1024)))
			{
				current_Y_Q_value -= 5;
			}
			else if (jpeg_size < (max_VLC_size-(55*1024)))
			{
				current_Y_Q_value += 5;
			}

			if(current_Y_Q_value < 5)
			{
				current_Y_Q_value = 5;
			}
		}

		if(current_Y_Q_value >= 80)
		{
			current_Y_Q_value = 80;
		}
}


