#include "drv_l1_system.h"

extern INT32S spifc_quad_mode_set(INT8U ctrlValue);

static INT32S iRAM_Delay(INT32U cnt)
{
	INT32U i;
	
	for (i=0; i<cnt;++i)
	{
		__asm {NOP};
	}
	return 0;
}

#if 0
static void system_clk_set(void )
{   	  

	// SDRAM Timing
	R_MEM_SDRAM_CTRL0 = 0x0711;	// SDRAM 把计
   	R_MEM_SDRAM_CTRL1 = 0x2000;	// SDRAM 把计
   	R_MEM_SDRAM_CBRCYC = 0x8CA;	// SDRAM 把计

    /*PLL setting*/
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting stable

    R_SYSTEM_CLK_CTRL &= ~(0x8008); // Fast PLL disable, Entry Slow Mode

    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP
    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP    
    
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting slow mode stable

    {	// Reset XCKGEN
   		(*((volatile INT32U *) 0xD0000058)) &= ~0x1;
   		iRAM_Delay(0);
   		(*((volatile INT32U *) 0xD0000058)) |=  0x1;
   		// Wait stable ...
   		while ( ( (*((volatile INT32U *) 0xD0000058))&0x8000 ) == 0   )
   		{
   			__asm {NOP};
   		}
    		iRAM_Delay(100);		
    }   
    {		// change clk
	       R_SYSTEM_PLLEN = 0x14;		// CPU = 144MHx,  SPI = 72MHz
		iRAM_Delay(50);
		R_SYSTEM_CLK_CTRL |= 0x8008; // Fast PLL ENABLE
    }
    
    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP
    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP    
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting fast mode stable

}
#else
void system_clk_set(INT8U CPUCLK )
{   	  

	// SDRAM Timing
	if(CPUCLK == 48)
	    R_MEM_SDRAM_CTRL0 = 0x0011;	// SDRAM 把计
	else
	    R_MEM_SDRAM_CTRL0 = 0x0711;	// SDRAM 把计
   	R_MEM_SDRAM_CTRL1 = 0x2000;	// SDRAM 把计
   	R_MEM_SDRAM_CBRCYC = 0x8CA;	// SDRAM 把计

    /*PLL setting*/
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting stable

    R_SYSTEM_CLK_CTRL &= ~(0x8008); // Fast PLL disable, Entry Slow Mode

    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP
    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP    
    
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting slow mode stable

    {	// Reset XCKGEN
   		(*((volatile INT32U *) 0xD0000058)) &= ~0x1;
   		iRAM_Delay(0);
   		(*((volatile INT32U *) 0xD0000058)) |=  0x1;
   		// Wait stable ...
   		while ( ( (*((volatile INT32U *) 0xD0000058))&0x8000 ) == 0   )
   		{
   			__asm {NOP};
   		}
    		iRAM_Delay(100);		
    }   
    {		// change clk
        if(CPUCLK == 48)
	       R_SYSTEM_PLLEN = 0x02;		// CPU = 48MHz
	    else if(CPUCLK == 96)
	       R_SYSTEM_PLLEN = 0x0e;		// CPU = 96MHz
	    else if(CPUCLK == 120)
	       R_SYSTEM_PLLEN = 0x14;		// CPU = 120MHz
	    else
	       R_SYSTEM_PLLEN = 0x1a;		// CPU = 144MHz
		iRAM_Delay(50);
		R_SYSTEM_CLK_CTRL |= 0x8008; // Fast PLL ENABLE
    }
    
    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP
    (*((volatile INT32U *) 0xF8500000)) = (*((volatile INT32U *) 0xFF000001));//DUMMY OP    
    while (R_SYSTEM_POWER_STATE == 0) {;;} //waiting fast mode stable

}
#endif

////////////////
// ICE  clock ㄓ方琌 SYSCLK
// ICE 璶铆﹚ SYSCLK / ICE_CLK > 7
// ㄓ SYSCLK=144MHz,  ICE LCK=4MHz
// 秈 SLOW mode SYSCLK=12MHz┮ ICE clk ぃ禬筁 1.7M Hz
// ICE 1.3M Hz ち繵奔
// ち场 XTAL 12M
void system_clk_ext_XLAT_12M(void)
{
	R_SYSTEM_MISC_CTRL1 |= 0x01;
	R_SYSTEM_CKGEN_CTRL |= 0x14C;

	{
		INT32U i;
		for (i=0;i<0xF000;++i) {
			R_RANDOM0 = i;		// delay for XLAT clock stable
		}
		
	}
	
	R_SYSTEM_CLK_CTRL &= 0x3fff;		//enter SLOW mode

	while ( (R_SYSTEM_POWER_STATE&0xF)!=1 ) {
		//DBG_PRINT("wait goint to SLOW mode\r\n");
		;		
	}
	
	R_SYSTEM_CKGEN_CTRL |= 0x21;		
	R_SYSTEM_CLK_CTRL |= 0x8000;		//enter FAST mode again

	
	while ( (R_SYSTEM_POWER_STATE&0xF)!=2 ) {
		//DBG_PRINT("wait coming back to FAST mode\r\n");
		;		
	}

	R_SYSTEM_CTRL |= 0x00000902;  // josephhsieh@140519 sensor 玡狠㏕﹚48MHz
	R_SYSTEM_CTRL &= ~0x1000;	//clear SEL30K
	R_SYSTEM_CKGEN_CTRL |= 0x1F;
}


void system_clk_alter(INT32U SysClk, INT32U SpiClk, INT32U SpiDelay, INT32U SDramPara)
{	// switch system clock in TFT Vblanking  (ISR)
	INT32U REG_SYSCLK = (R_SYSTEM_PLLEN & (~0x3C1F));
	INT32U REG_SPIDELAY = ( (*((volatile INT32U *) 0xC0010024)) & (~0x7) );
	R_SYSTEM_PLLEN =  REG_SYSCLK | SysClk | SpiClk;
	R_MEM_SDRAM_CTRL0 = SDramPara;	
	(*((volatile INT32U *) 0xC0010024)) = REG_SPIDELAY | SpiDelay;
	
	iRAM_Delay(0x80);
}

// =========================================================
#include "drv_l1_spifc.h"
#include "drv_l2_spifc.h"

/*
// because of system clock no change, we can use global variable
spifc_apis_ops* pSpifc_apis_ops;
void SPI_MODE_SET(INT32U mode)
{
	TX_RX_ARGS args = {0};

	 R_IOD_DRV |= 2;			// SPI_CLK driving
	 (*((volatile INT32U *) 0xC0010024)) |= 2;		// SPI delay
	 R_SYSTEM_PLLEN &= (~0x3C00);	// SYS_CLK/2   (SYS_CLK=144MHz)

	pSpifc_apis_ops = spifc_attach();
	
	if ((mode!=FLAG_READ_PROGRAM_2IO)&&(mode!=FLAG_READ_PROGRAM_4IO))
	{
		DBG_PRINT("SPI 1-bit mode\r\n");
		return ;
	}
	
	pSpifc_apis_ops->init();
	if (mode==FLAG_READ_PROGRAM_4IO)
	{
		spifc_quad_mode_set(1);
	}
	{
		args.flag_args = ((FLAG_MEM_ACCESS<<3)|mode);  // memory access & 2-bit 	
		args.addrs = (INT32U)0;
		args.buff = (INT8U*)DUMMY_SPI_ADDRS;
		args.buff_len = 256;
		pSpifc_apis_ops->read(&args);
	}
	

	if (mode==FLAG_READ_PROGRAM_4IO) {
		DBG_PRINT("SPI_CLK=144MHz/2, SPI 4-bit mode\r\n");
	}
	else {
		DBG_PRINT("SPI_CLK=144MHz/2, SPI 2-bit mode\r\n");		
	}
	if  ((R_SYSTEM_PLLEN&0x1F)!=0x1A) {
		DBG_PRINT("Warning, SYS_CLK error!!\r\n");
	}
}
*/

spifc_apis_ops* pSpifc_apis_ops;
void SYSTEM_CLK_MODE(INT32U mode)
{
	TX_RX_ARGS args = {0};

	// CPU setting
	system_clk_set(INIT_MHZ);		// CPU=144MHz, SPI=72MHz

	// SPI setting
	 R_SPIFC_TIMING = 2;		// SPI delay
	 R_IOD_DRV = 0x2;			// SPI_CLK driving
	 
	pSpifc_apis_ops = spifc_attach();	
	if ((mode!=FLAG_READ_PROGRAM_2IO)&&(mode!=FLAG_READ_PROGRAM_4IO))
	{
		return ;
	}

	pSpifc_apis_ops->init();
	if (mode==FLAG_READ_PROGRAM_4IO)
	{
		spifc_quad_mode_set(1);
	}
	{
		args.flag_args = ((FLAG_MEM_ACCESS<<3)|mode);  // memory access & 2-bit 	
		args.addrs = (INT32U)0;
		args.buff = (INT8U*)DUMMY_SPI_ADDRS;
		args.buff_len = 256;
		pSpifc_apis_ops->read(&args);
	}
}
