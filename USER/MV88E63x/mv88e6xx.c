#include "mv88e6xx.h"
#include "stm32f4x7_eth.h"
#include "usart.h" 
#include "malloc.h" 
#include "string.h"

#include "gtDrvSwRegs.h"
#include "msApiTypes.h"
#include "msApiDefs.h"
#include "msApiInternal.h"


/* Private define ------------------------------------------------------------*/
#define		PHY_ADDRESS     0x3    // MV88E6321 PHY芯片地址.

ETH_DMADESCTypeDef *DMARxDscrTab;	//以太网DMA接收描述符数据结构体指针
ETH_DMADESCTypeDef *DMATxDscrTab;	//以太网DMA发送描述符数据结构体指针 
uint8_t *Rx_Buff; 					//以太网底层驱动接收buffers指针 
uint8_t *Tx_Buff; 					//以太网底层驱动发送buffers指针

extern GT_QD_DEV       *dev;


/****************************************************************************
* 名    称: u8 mv88e6xx_init(void)
* 功    能：ETH初始化
* 入口参数： 
* 返回参数：无
* 说    明：       
****************************************************************************/	
u8 mv88e6xx_init(void)
{
	u8 ret;
	RCC_ClocksTypeDef RCC_Clocks;   	// Configure the GPIO ports for ethernet pins  

	ETH_GPIO_Config();
	
	SFP_GPIO_Config();

	ETH_NVIC_Config();      // Config NVIC for Ethernet  

	RCC_GetClocksFreq(&RCC_Clocks);  // SystTick configuration: an interrupt every 10ms   
	SysTick_Config(RCC_Clocks.SYSCLK_Frequency / 100);

	/* Update the SysTick IRQ priority should be higher than the Ethernet IRQ */
	/* The Localtime should be updated during the Ethernet packets processing */
	NVIC_SetPriority (SysTick_IRQn, 1);  

	ret = ETH_MACDMA_Config();	// Configure the Ethernet MAC/DMA  
	return !ret;     //ETH的规则为:0,失败;1,成功;所以要取反一下 
  
}

/****************************************************************************
* 名    称: static u8 ETH_MACDMA_Config(void)
* 功    能：ETH DMA初始化
* 入口参数： 
* 返回参数：无
* 说    明：       
****************************************************************************/
static u8 ETH_MACDMA_Config(void)
{
  u8 ret;
  ETH_InitTypeDef ETH_InitStructure;

  /* Enable ETHERNET clock  */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
                         RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);                                             

  ETH_DeInit(); /* Reset ETHERNET on AHB Bus */

  ETH_SoftwareReset();   /* Software reset */

  while (ETH_GetSoftwareResetStatus() == SET);  /* Wait for software reset */
	
  /* ETHERNET Configuration --------------------------------------------------*/
  /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
  ETH_StructInit(&ETH_InitStructure);

  /* Fill ETH_InitStructure parametrs */
  /*------------------------   MAC   -----------------------------------*/
  ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
  //ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Disable; 
  //ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
  //ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;   

  ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
  ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
  ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
  ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
  ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
  ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
  ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
  ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
  ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

  /*------------------------   DMA   -----------------------------------*/  
  
  /* When we use the Checksum offload feature, we need to enable the Store and Forward mode: 
  the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum, 
  if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
  ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; 
  ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;         
  ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;     
 
  ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;       
  ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;   
  ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;
  ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;      
  ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;                
  ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;          
  ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
  ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

  /* Configure Ethernet */
  ret = ETH_Init(&ETH_InitStructure, PHY_ADDRESS);
  if( ret == ETH_SUCCESS ){
	printf("======PHY init ok!==== \r\n");	
	/* Enable the Ethernet Rx Interrupt */
  	ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
  }else{
	printf("======PHY init ERROR!==== \r\n");				
  }  

  return ret;
}

/****************************************************************************
* 名    称: void ETH_GPIO_Config(void)
* 功    能：ETH GPIO初始化
* 入口参数： 
* 返回参数：无
* 说    明：对接MV88E6321的IO初始化       
//output pins
MII_TXD3  PE.2
MDC  	  PC.1
MDIO      PA.2
MII TXD2  PC.2
MII TXCLK PC.3
MII TXEN  PB.11
MII TXD0  PB.12
MII TXD1  PB.13

//input pins
MII RXCLK PA.1
MII RXDV  PA.7
MII RXD0  PC.4
MII RXD1  PC.5
MII RXD2  PB.0
MII RXD3  PB.1

****************************************************************************/
void ETH_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	volatile int i;
	/* Enable GPIOs clocks */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
							 RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);

	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_MII); //MAC和PHY之间使用MII接口	
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2| GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_ETH);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource2, GPIO_AF_ETH);

}

/****************************************************************************
* 名    称: void SFP_GPIO_Config(void)
* 功    能：SFP GPIO初始化
* 入口参数： 
* 返回参数：无
* 说    明：1000M 光模块的IO初始化       

SFP TxFAU PD.5
SFP LOS   PD.6
SFP TxDIS PD.7
SFP MODE0 PB.5
SFP SCL   PB.6
SFP SDA   PB.7
****************************************************************************/
void SFP_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	volatile int i;
	/* Enable GPIOs clocks */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD, ENABLE);
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_OUT;          //普通输出模式  ???
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	PDout(5) = 1;
	PDout(7) = 0;
	
}

/****************************************************************************
* 名    称: void ETH_NVIC_Config(void)
* 功    能：ETH 中断初始化
* 入口参数： 
* 返回参数：无
* 说    明：对接MV88E6321的IO初始化       
****************************************************************************/
void ETH_NVIC_Config(void)
{
  NVIC_InitTypeDef   NVIC_InitStructure;

  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); 
  
  /* Enable the Ethernet global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);    
}

extern void lwip_pkt_handle(void);		//在lwip_comm.c里面定义
//以太网DMA接收中断服务函数
void ETH_IRQHandler(void)
{
	while(ETH_GetRxPktSize(DMARxDescToGet)!=0) 	//检测是否收到数据包
	{ 
		lwip_pkt_handle();		
	}
	ETH_DMAClearITPendingBit(ETH_DMA_IT_R); 	//清除DMA中断标志位
	ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);	//清除DMA接收中断标志位
}  
//接收一个网卡数据包
//返回值:网络数据包帧结构体
FrameTypeDef ETH_Rx_Packet(void)
{ 
	u32 framelength=0;
	FrameTypeDef frame={0,0};   
	//检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
	if((DMARxDescToGet->Status&ETH_DMARxDesc_OWN)!=(u32)RESET)
	{	
		frame.length=ETH_ERROR; 
		if ((ETH->DMASR&ETH_DMASR_RBUS)!=(u32)RESET)  
		{ 
			ETH->DMASR = ETH_DMASR_RBUS;//清除ETH DMA的RBUS位 
			ETH->DMARPDR=0;//恢复DMA接收
		}
		return frame;//错误,OWN位被设置了
	}  
	if(((DMARxDescToGet->Status&ETH_DMARxDesc_ES)==(u32)RESET)&& 
	((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET)&&  
	((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET))  
	{       
		framelength=((DMARxDescToGet->Status&ETH_DMARxDesc_FL)>>ETH_DMARxDesc_FrameLengthShift)-4;//得到接收包帧长度(不包含4字节CRC)
 		frame.buffer = DMARxDescToGet->Buffer1Addr;//得到包数据所在的位置
	}else framelength=ETH_ERROR;//错误  
	frame.length=framelength; 
	frame.descriptor=DMARxDescToGet;  
	//更新ETH DMA全局Rx描述符为下一个Rx描述符
	//为下一次buffer读取设置下一个DMA Rx描述符
	DMARxDescToGet=(ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);   
	return frame;  
}
//发送一个网卡数据包
//FrameLength:数据包长度
//返回值:ETH_ERROR,发送失败(0)
//		ETH_SUCCESS,发送成功(1)
u8 ETH_Tx_Packet(u16 FrameLength)
{   
	//检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
	if((DMATxDescToSet->Status&ETH_DMATxDesc_OWN)!=(u32)RESET)return ETH_ERROR;//错误,OWN位被设置了 
 	DMATxDescToSet->ControlBufferSize=(FrameLength&ETH_DMATxDesc_TBS1);//设置帧长度,bits[12:0]
	DMATxDescToSet->Status|=ETH_DMATxDesc_LS|ETH_DMATxDesc_FS;//设置最后一个和第一个位段置位(1个描述符传输一帧)
  	DMATxDescToSet->Status|=ETH_DMATxDesc_OWN;//设置Tx描述符的OWN位,buffer重归ETH DMA
	if((ETH->DMASR&ETH_DMASR_TBUS)!=(u32)RESET)//当Tx Buffer不可用位(TBUS)被设置的时候,重置它.恢复传输
	{ 
		ETH->DMASR=ETH_DMASR_TBUS;//重置ETH DMA TBUS位 
		ETH->DMATPDR=0;//恢复DMA发送
	} 
	//更新ETH DMA全局Tx描述符为下一个Tx描述符
	//为下一次buffer发送设置下一个DMA Tx描述符 
	DMATxDescToSet=(ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);    
	return ETH_SUCCESS;   
}
//得到当前描述符的Tx buffer地址
//返回值:Tx buffer地址
u32 ETH_GetCurrentTxBuffer(void)
{  
  return DMATxDescToSet->Buffer1Addr;//返回Tx buffer地址  
}

//为ETH底层驱动申请内存
//返回值:0,正常
//    其他,失败
u8 ETH_Mem_Malloc(void)
{ 
	DMARxDscrTab=mymalloc(SRAMIN,ETH_RXBUFNB*sizeof(ETH_DMADESCTypeDef));//申请内存
	DMATxDscrTab=mymalloc(SRAMIN,ETH_TXBUFNB*sizeof(ETH_DMADESCTypeDef));//申请内存  
	Rx_Buff=mymalloc(SRAMIN,ETH_RX_BUF_SIZE*ETH_RXBUFNB);	//申请内存
	Tx_Buff=mymalloc(SRAMIN,ETH_TX_BUF_SIZE*ETH_TXBUFNB);	//申请内存
	if(!DMARxDscrTab||!DMATxDscrTab||!Rx_Buff||!Tx_Buff)
	{
		ETH_Mem_Free();
		return 1;	//申请失败
	}	
	return 0;		//申请成功
}

//释放ETH 底层驱动申请的内存
void ETH_Mem_Free(void)
{ 
	myfree(SRAMIN,DMARxDscrTab);//释放内存
	myfree(SRAMIN,DMATxDscrTab);//释放内存
	myfree(SRAMIN,Rx_Buff);		//释放内存
	myfree(SRAMIN,Tx_Buff);		//释放内存  
}

void switch_default_config(void)
{
	#if 0
	uint16_t phy_status;
	uint16_t port_status;
	
	/* config port register status */	 
	if ( !( ETH_ReadPHYRegister(0x15,PHYSICAL_CR) & PORT_LinkValue) ) 
	{
		port_status = ETH_ReadPHYRegister(0x15,PHYSICAL_CR);
		ETH_WritePHYRegister(0x15,PHYSICAL_CR,port_status | PORT_ForcedLink); /* port 5 forced link */
		printf("\r\n=== cpu [%d] PORT_ForcedLink = %x \r\n",5,port_status | PORT_ForcedLink); 
	}

	if ( !( ETH_ReadPHYRegister(0x13,PORT_CR) & PORT_STATE) ) 
	{
		port_status = ETH_ReadPHYRegister(0x13,PORT_CR);
		ETH_WritePHYRegister(0x13,PORT_CR,port_status | PORT_STATE); /* port 3 forwarding */
		printf("\r\n=== port[%d] port status = %x \r\n",3,port_status | PORT_STATE); 
	}

	if ( !( ETH_ReadPHYRegister(0x15,PORT_CR) & PORT_STATE) ) 
	{
		port_status = ETH_ReadPHYRegister(0x15,PORT_CR);
		ETH_WritePHYRegister(0x15,PORT_CR,port_status | PORT_STATE); /* port 5 forwarding */
		printf("\r\n=== cpu [%d] port status = %x \r\n",5,port_status | PORT_STATE); 
	}
	
	if ( !( ETH_ReadPHYRegister(0x10,PHYSICAL_CR) & PORT_LinkValue) ) 
	{
		port_status = ETH_ReadPHYRegister(0x10,PHYSICAL_CR);
		ETH_WritePHYRegister(0x10,PHYSICAL_CR,port_status | PORT_ForcedLink); /* port 0 forced link */
		printf("\r\n=== port [%d] PORT_ForcedLink = %x \r\n",0,port_status | PORT_ForcedLink); 
	}
	
	if ( !( ETH_ReadPHYRegister(0x10,PORT_CR) & PORT_STATE) ) 
	{
		port_status = ETH_ReadPHYRegister(0x10,PORT_CR);
		ETH_WritePHYRegister(0x10,PORT_CR,port_status | PORT_STATE); /* port 0 forwarding */
		printf("\r\n=== port [%d] port status = %x \r\n",0,port_status | PORT_STATE); 
	}
	#endif

	/* config port register status */	 
	gprtSetCPUPort( dev, CPU_PORT ,5 );
	gprtSetForwardUnknown( dev, CPU_PORT, GT_TRUE ); /* set port forwarding */
	gpcsSetForcedLink( dev, CPU_PORT, GT_TRUE ); 		/* set port forced link */
	
	gprtSetForwardUnknown( dev, ETH1_PORT, GT_TRUE );
	gprtSetForwardUnknown( dev, UPLINK_PORT, GT_TRUE );

	/* phy register initial */
	/*
		PHY initial :
		step 1. software reset
		step 2. auto-negotiation enable
		step 3.	power down
		step 4. restart auto-negotiation
	*/
	//set phy 3
	gprtPhyReset( dev, PHY_2_LPORT_3 );
	gprtPortAutoNegEnable( dev, PHY_2_LPORT_3, GT_TRUE );
	gprtPortRestartAutoNeg( dev, PHY_2_LPORT_3 );
	gprtPortPowerDown( dev, PHY_2_LPORT_3, GT_FALSE );

	//set phy 0 reset,auto-negotiation enable,power down and restart auto-negotiation	
	gprtSerdesReset( dev, PHY_2_LPORT_0 );
	gprtSerdesAutoNegEnable( dev, PHY_2_LPORT_0, GT_TRUE );
	gprtSerdesRestartAutoNeg( dev, PHY_2_LPORT_0 );
	gprtSerdesPowerDown( dev, PHY_2_LPORT_0, GT_FALSE );
	
}




