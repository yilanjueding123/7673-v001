#ifndef __FS_H__
#define __FS_H__
 
#define C_FS_VERSION	0x0100		// version 1.00

//============================================================================
#define _FAT_CLUSTER_SIZE_4K		0       //ʹ�ù̶��Ĵش�С
#define FAT_OS						0		//�����Ƿ�ʹ��OS

#define MALLOC_USE					0		//�����Ƿ�ʹ��BUDDY
#define USE_GLOBAL_VAR				1

#define _USE_EXTERNAL_MEMORY		0		//ʹ���ⲿRAM,����buddy����
#define _PART_FILE_OPERATE			1		//�˶����ʾ֧��ɾ�������ļ��Ͳ������

#define _READ_WRITE_SPEED_UP		1		//��ʾ����дһ��sectorʱ����ͨ��bufferֱ�Ӷ�disk���ж�д

#define _SEEK_SPEED_UP				1		//�˶����ʾ�Ƿ��seek���٣���Ҫ������ڴ�ռ�Ͷ�ջ

#define _CACHE_FAT_INFLEXION		1		//���ڹյ��seek���ٷ���

#define SUPPORT_GET_FOLDER_NAME		1

#define CHEZAI_MP3					0
#define MAX_FILE_NUM				20		//ָ�������Դ򿪵��ļ�����Ŀ

#define _DIR_UPDATA					1		//��ʾ��д�Ĺ����п��Ը���dir����Ϣ

//֧��С�����Ĵ��̵ĸ�ʽ�����̶���ʽ��ΪFAT16������MBR����Ҫָ�����̵����sector����ʵ��sector��
//��Ϊ���ǵ��ļ�ϵͳ��֧��FAT12��������������С��16MBʱ��PC��Ѵ�������FAT12�����������������������
//��С��16MB�Ĵ��̸�ʽ��Ϊ����16Mb����ָ����С����������ָ������ʵ�ʵĿռ��С
#define _SMALL_DISK_FORMAT			1

#define DISK_DEFRAGMENT_EN			0		//��ʾ֧�ִ���ɨ��

#define ADD_ENTRY					1		//���浱ǰ������Ŀ¼���ƫ����

#define FIND_EXT					1		//֧����չ�Ĳ����㷨

#define  ALIGN_16                   32     //modify by wangzhq
#define SUPPORT_VOLUME_FUNCTION		1
#if CHEZAI_MP3 == 1
	#define READ_ONLY				1	
#endif

#define FS_FORMAT_SPEEDUP			1		// support fs format speedup, the file system will malloc 8kB buffer


//ָ��֧�ֵĴ�����,ΪSD+NAND FLASH�ķ�����


//============================================================================

//#define LFN_API				1
#define LFN_FLAG			1
#define WITHFAT32			1
#define WITHFAT12			1
#ifndef READ_ONLY
#define WITH_CDS_PATHSTR	1				// Remember current position
#else
 #define WITH_CDS_PATHSTR	0				// Remember current position
#endif 

#define NUMBUFF				4

#define FSDRV_MAX_RETRY		1

#define C_MAX_DIR_LEVEL		16				// Max directory deepth level
#define	C_FILENODEMARKDISTANCE	100

#define FD32_LFNPMAX 		260				// Max length for a long file name path, including the NULL terminator
#define FD32_LFNMAX  		255				// Max length for a long file name, including the NULL terminator

#if FS_FORMAT_SPEEDUP == 1
	#define C_FS_FORMAT_BUFFER_SECTOR_SIZE		32
	#define C_FS_FORMAT_BUFFER_SIZE				(512 * C_FS_FORMAT_BUFFER_SECTOR_SIZE)	// 8192 Byte

	#define GLOBAL_BUFFER						1
	#define ALLOCATE_BUFFER						2
	#define FS_FORMAT_SPEEDUP_BUFFER_SOURCE		ALLOCATE_BUFFER
#else
	#define GLOBAL_BUFFER						1
	#define ALLOCATE_BUFFER						2
	#define FS_FORMAT_SPEEDUP_BUFFER_SOURCE		GLOBAL_BUFFER
#endif

#endif		// __FS_H__