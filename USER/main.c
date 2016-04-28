#include "main.h"
#include <stdio.h>
#include "usart.h"
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"

#include "netconf.h"
#include "tcp.h"
#include "udp.h"
#include "string.h"
#include "UDP_SERVER.h"
#include "malloc.h"

#include "msApiTypes.h"
#include "msSample.h"
#include "msApiDefs.h"
#include "madInit.h"

//GT_QD_DEV		swDev = {0};
extern GT_QD_DEV		*dev;
unsigned short data1,data2;

/* Private define ------------------------------------------------------------*/
#define SYSTEMTICK_PERIOD_MS  10

/* Private variables ---------------------------------------------------------*/
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */

/**
  * @brief  Updates the system local time
  * @param  None
  * @retval None
  */
void Time_Update(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
}

u16 Show_ETH_PHY(u16 PHYRegAddr)
{
  u16 PHYRegData;   
  PHYRegData = ETH_ReadPHYRegister(0,0x02);
  printf("\n\rETH_ReadPHYRegister(%d,%02x):0x%X",1,0x02, PHYRegData); 
   PHYRegData = ETH_ReadPHYRegister(0,0x03);
  printf("\n\rETH_ReadPHYRegister(%d,%02x):0x%X",1,0x03, PHYRegData); 
  return PHYRegData;
}

u16 Read_ETH_PHY(u16 PHYAddress,u16 PHYRegAddr)
{
	u16 PHYRegData;   
	PHYRegData = ETH_ReadPHYRegister(PHYAddress,PHYRegAddr);
	printf("\n\rETH_ReadPHYRegister(%d,%02x):0x%X",PHYAddress,PHYRegAddr, PHYRegData); 
  	return PHYRegData;
}

void Check_ETH_PHY(void)
{
/*
    Show_ETH_PHY(0);
    Show_ETH_PHY(1);
    Show_ETH_PHY(2);
    Show_ETH_PHY(3);
    Show_ETH_PHY(4);
    Show_ETH_PHY(5);
    Show_ETH_PHY(6);
*/
	int i = 0;
	//for( i = 0; i < 16; i++ )
	//{
		Show_ETH_PHY(0);
	//}
	
	
    if(Show_ETH_PHY(17) & 0x3000)
    {  
      /* Set Ethernet speed to 10M following the autonegotiation */    
      printf("\n\r==>ETH_Speed_10M!");    
    }
    else
    {   
      /* Set Ethernet speed to 100M following the autonegotiation */ 
      printf("\n\r==>ETH_Speed_100M!");          
    } 
    /* Configure the MAC with the Duplex Mode fixed by the autonegotiation process */
    if((Show_ETH_PHY(17) & 0xA000) != (uint32_t)RESET)
    {
      /* Set Ethernet duplex mode to FullDuplex following the autonegotiation */
      printf("\n\r==>ETH_Mode_FullDuplex!");      
    }
    else
    {
      /* Set Ethernet duplex mode to HalfDuplex following the autonegotiation */
      printf("\n\r==>ETH_Mode_HalfDuplex!");      
    }
}

int main(void)
{
    u16 i = 0;
	u8_t str;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
	delay_init();         //初始化延时函数
    uart_init(115200);
	my_mem_init(SRAMIN);		//初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部内存池
	ETH_BSP_Config();     //PHY相关IO初始化与ETH相关初始化
	//LwIP_Init();			//LWIP初始化
	//UDP_server_init();	//初始化开发板为UDP服务器
	//Check_ETH_PHY();

	#if 0	

	if(qd_madInit(swdev,0) == 0)
	{
		printf("\r\n=== qd_madInit  OK!===\r\n");	
	}else{
		printf("\r\n=== qd_madInit ERROR!===\r\n");	
	}
	#endif
    #if 0
	if(qdStart(5,1,GT_88E6321) == 0){
		printf("\r\n=== mad driver init OK!===\r\n");	
	}else{
		printf("\r\n=== mad driver init ERROR!===\r\n");	
	}
	#if 0
	for( i = 0; i < 7; i++ )
	{
		if( gprtGetPagedPhyReg(dev,i, 0x03,0,&data1) == 0)
			{
				printf("\r\n===11 gprtGetPagedPhyReg OK! data = %x===\r\n",data1);	
			}else{
				printf("\r\n===11 gprtGetPagedPhyReg ERROR!===\r\n");	
			}
			
			
		if( gprtGetPhyReg(dev,i, 0x03,&data1) == 0)
		{
			printf("\r\n===22 gprtGetPhyReg OK! data = %x===\r\n",data1);	
		}else{
			printf("\r\n===22 gprtGetPhyReg ERROR!===\r\n"); 
		}

	}
	#endif
	#endif
	#if 0
	if(madInit(0) == 0)
	{
		printf("\r\n=== qd_madInit	OK!===\r\n");	
	}else{
		printf("\r\n=== qd_madInit ERROR!===\r\n"); 
	}
#endif	
	printf("==please input string:\r\n");
	while (1)
	{   		
     	scanf("%c",&str);
		if( str == '0' ){
			printf("==[%s]-%d-input string:%c OK\r\n",__FILE__,__LINE__,str);	
			if(qdStart(5,1,GT_88E6321) == 0){
				printf("\r\n=== mad driver init OK!===\r\n");	
			}else{
				printf("\r\n=== mad driver init ERROR!===\r\n");	
			}
		}
		//LwIP_Periodic_Handle(LocalTime);		  /* handle periodic timers for LwIP */ 
	}	  

}



