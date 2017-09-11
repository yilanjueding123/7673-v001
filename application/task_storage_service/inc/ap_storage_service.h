#include "task_storage_service.h"
#include "stdio.h"

#define STORAGE_TIME_INTERVAL_MOUNT		128	//128 = 1s
#define STORAGE_TIME_INTERVAL_FREESIZE	128*15	//128 = 1s
#define STORAGE_TIME_INTERVAL_FORMAT	128*5	//128 = 1s
#define STORAGE_TIME_FIRST_CHECK        128*2

extern void ap_storage_service_init(void);
extern INT8S Time_file_get(void);
extern INT32S ap_storage_service_storage_mount(void);
#if C_AUTO_DEL_FILE == CUSTOM_ON
	extern void ap_storage_service_freesize_check_switch(INT8U type);
	extern INT32S ap_storage_service_freesize_check_and_del(void);
	extern void ap_storage_service_del_thread_mb_set(void);
	extern INT32U bkground_del_thread_size_get(void);
#endif
extern void ap_storage_service_freefile_size_check(INT16U usr_config);
extern void ap_storage_service_file_open_handle(INT32U type);
extern void ap_storage_service_timer_start(void);
extern void ap_storage_service_timer_stop(void);
extern void ap_storage_service_play_req(STOR_SERV_PLAYINFO *info_ptr, INT32U req_msg);
extern void JH_Message_Get(void);

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
	extern void ap_storage_service_cyclic_record_file_open_handle(void);
#endif
extern void ap_storage_service_usb_plug_in(void);
extern INT8U ap_state_config_date_stamp_get(void);
extern void ap_state_config_date_stamp_set(INT8U flag);
extern void ap_storage_service_file_del(INT32U idx);
extern void ap_storage_service_file_delete_all(void);
extern void ap_storage_service_file_lock_one(void);
extern void ap_storage_service_file_lock_all(void);
extern void ap_storage_service_file_unlock_one(void);
extern void ap_storage_service_file_unlock_all(void);
extern void ap_storage_service_format_req(void);
