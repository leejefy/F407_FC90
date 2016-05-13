#include "main.h"
#include <stdio.h>
#include "usart.h"
#include "stm32f4x7_eth.h"
#include "lwip_comm.h"
#include "timer.h"
#include "malloc.h"
#include "lwip/netif.h"
#include "mv88e6xx.h"
#include "lwipopts.h"
#include "string.h"

#include "gtDrvSwRegs.h"
#include "msApiTypes.h"
#include "msApiDefs.h"
#include "msApiInternal.h"

extern GT_QD_DEV       *dev;

void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==1)
	{
		sprintf((char*)buf,"MAC    :%d.%d.%d.%d.%d.%d",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//打印MAC地址
		printf("\r\n %s \r\n",buf);
		sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		printf("\r\n %s \r\n",buf);
		sprintf((char*)buf,"DHCP GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		printf("\r\n %s \r\n",buf);
		sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		printf("\r\n %s \r\n",buf);

	}
	else 
	{
		sprintf((char*)buf,"MAC      :%d.%d.%d.%d.%d.%d",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//打印MAC地址
		printf("\r\n %s \r\n",buf);
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		printf("\r\n %s \r\n",buf);
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		printf("\r\n %s \r\n",buf);
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		printf("\r\n %s \r\n",buf);

	}	
}

int main(void)
{
    u16 i = 0;
	u8 str;
	u16 phyId;
	u16 regData;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
	delay_init();         //初始化延时函数
    uart_init(115200);
	my_mem_init(SRAMIN);		//初始化内部内存池
	//_mem_init(SRAMEX);		//初始化外部内存池	

	TIM3_Int_Init(999,839); //100khz的频率,计数1000为10ms	
	
 	while(lwip_comm_init()) //lwip初始化
	{
		printf("\r\n LWIP Init Falied! \r\n");
		delay_ms(1200);
		printf("\r\n Retrying...\r\n");
	}
#if LWIP_DHCP   //使用DHCP
	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//等待DHCP获取成功/超时溢出
	{	
		lwip_periodic_handle();	//LWIP内核需要定时处理的函数
	}
#endif

	show_address(lwipdev.dhcpstatus);	//显示地址信息

	if( qdStart(dev,5,0,0) == 0 )
	{		
		switch_default_config();
		printf("\r\n switch init OK! \r\n");	
	}else {
		printf("\r\n switch init Falied! \r\n");	
	}
	
	while (1)
	{   		
		lwip_periodic_handle();	//LWIP内核需要定时处理的函数
	}	  

}



