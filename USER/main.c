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

#include "gtDrvSwRegs.h"
#include "msApiTypes.h"

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
  PHYRegData = ETH_ReadPHYRegister(0x10,0x0);
  printf("\n\rETH_ReadPHYRegister(%02x,%02x):0x%X",0x10,0x0, PHYRegData); 
  PHYRegData = ETH_ReadPHYRegister(0x11,0x0);
  printf("\n\rETH_ReadPHYRegister(%02x,%02x):0x%X",0x11,0x0, PHYRegData); 
  return PHYRegData;
}

u16 Read_ETH_PHY(u16 PHYAddress,u16 PHYRegAddr)
{
	u16 PHYRegData;   
	PHYRegData = ETH_ReadPHYRegister(PHYAddress,PHYRegAddr);
	printf("\n\rETH_ReadPHYRegister(%02x,%02x):0x%X\n",PHYAddress,PHYRegAddr, PHYRegData); 
  	return PHYRegData;
}

u16 Write_ETH_PHY(u16 PHYAddress,u16 PHYRegAddr,u16 PHYRegData)
{	
	u16 ret;

	printf("\n\r=======Write_ETH_PHY=======\n"); 

	ret = ETH_WritePHYRegister(PHYAddress,PHYRegAddr,PHYRegData);
		
	return ret;
}

u16 Read_GLOBAL_REG(u16 SmiAddr, u16 RegAddr)
{
  u16 PHYRegData; 
  printf("\r\n@@@@@@@@@Read_GLOBAL_REG called!\r\n");
  
  PHYRegData = ETH_ReadPHYRegister(SmiAddr,RegAddr);
  printf("\n\r11 Read_GLOBAL_REG(%02x,%02x):0x%X",SmiAddr,RegAddr, PHYRegData); 
  return PHYRegData;
}

u16 Write_GLOBAL_REG(u16 SmiAddr, u16 RegAddr,u16 data)
{
  u16 ret; 
  printf("\r\n@@@@@@@@@Write_GLOBAL_REG called!\r\n"); 
  
  ret = Write_ETH_PHY(SmiAddr,RegAddr,data);
  printf("\n\Write_GLOBAL_REG(%02x,%02x):0x%X",SmiAddr,RegAddr,data); 

  return ret;
}


u16 Read_PHY_SMI( u16 devAddr , u16 regAddr )
{
	u16 smiReg;		
	u16 busy;
	u16 ret;
	u16 data = 0;
	
	smiReg =  QD_SMI_BUSY 
		| (1 << QD_SMI_MODE_BIT) /* 0 - clause 45, 1 - clause 22 */
		| (QD_SMI_READ_22 << QD_SMI_OP_BIT)
		| (devAddr << QD_SMI_DEV_ADDR_BIT) 
		| (regAddr << QD_SMI_REG_ADDR_BIT);
	
	printf("\r\n @@@@@@@@@Read_PHY_SMI called! smiReg= %x\r\n",smiReg); 

	ret = Write_ETH_PHY(0x1C, QD_REG_SMI_PHY_CMD, smiReg);

	while(ETH_ReadPHYRegister(0x1C, QD_REG_SMI_PHY_CMD) & QD_SMI_BUSY)
    {	
    	busy++;
		printf("\r\n ==== busy time : %d \r\n",busy);
	}

	data = ETH_ReadPHYRegister(0x1C,QD_REG_SMI_PHY_DATA);
	
	printf("\n\r Read_PHY_SMI(%02x,%02x):0x%X", 0x1C,QD_REG_SMI_PHY_DATA,data); 
	return data;	
}

u16 Write_PHY_SMI( u16 SmiAddr , u16 PHYRegAddr,  u16 data )
{
	u16 smiReg;
	u8 ret;
	smiReg =  QD_SMI_BUSY | (SmiAddr << QD_SMI_DEV_ADDR_BIT) | (QD_SMI_WRITE << QD_SMI_OP_BIT) | (PHYRegAddr << QD_SMI_REG_ADDR_BIT) | (QD_SMI_CLAUSE22 << QD_SMI_MODE_BIT);
	printf("\r\n @@@@@@@@@Write_PHY_SMI called! smiReg=%x\r\n",smiReg); 	
	
	ret = Write_ETH_PHY(SmiAddr,QD_REG_SMI_PHY_CMD,smiReg);

	if( ret ){
		ret = Write_ETH_PHY(SmiAddr,QD_REG_SMI_PHY_DATA,data);
	}
	
	printf("\n\r Write_PHY_SMI(%02x,%02x):0x%X",SmiAddr,QD_REG_SMI_PHY_DATA,data); 
	return data;	

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


	//Read_GLOBAL_REG(0x1c,0x18);

	//Read_GLOBAL_REG(0x1c,0x19);

#if 0  // SMI get phy info OK 
	data = Read_PHY_SMI(3,0x01);

	data = Read_PHY_SMI(3,0x02);

	data = Read_PHY_SMI(3,0x03);
#endif	
	//Write_GLOBAL_REG(0x1b,0x04,0x4000);
	//data = Read_PHY_SMI(0x1c,0x02);

	Read_ETH_PHY(0x10,0);
	Read_ETH_PHY(0x11,0);
	Read_ETH_PHY(0x12,0);
	Read_ETH_PHY(0x13,0);
	Read_ETH_PHY(0x14,0);
	Read_ETH_PHY(0x15,0);
	Read_ETH_PHY(0x16,0);
	

	//printf("==please input string:\r\n");
	while (1)
	{   		
     //	scanf("%c",&str);
	//	if( str == '0' ){
	//		printf("==[%s]-%d-input string:%c OK\r\n",__FILE__,__LINE__,str);	
 
	//	}
		//LwIP_Periodic_Handle(LocalTime);		  /* handle periodic timers for LwIP */ 
	}	  

}



