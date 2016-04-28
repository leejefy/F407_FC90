#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"
//#include "netconf.h"
//#include "tcp.h"
//#include "udp.h"
#include "string.h"
//#include "UDP_SERVER.h"


/* Private define ------------------------------------------------------------*/
#define		PHY_ADDRESS     0x00    // MV88E6321 PHY芯片地址.


/****************************************************************************
* 名    称: void ETH_BSP_Config(void)
* 功    能：ETH初始化
* 入口参数： 
* 返回参数：无
* 说    明：       
****************************************************************************/	
void ETH_BSP_Config(void)
{
  RCC_ClocksTypeDef RCC_Clocks;   	// Configure the GPIO ports for ethernet pins  

  ETH_GPIO_Config();
  
  ETH_NVIC_Config();      // Config NVIC for Ethernet  

  ETH_MACDMA_Config();    // Configure the Ethernet MAC/DMA  

  RCC_GetClocksFreq(&RCC_Clocks);  // SystTick configuration: an interrupt every 10ms   
  SysTick_Config(RCC_Clocks.SYSCLK_Frequency / 100);

  /* Update the SysTick IRQ priority should be higher than the Ethernet IRQ */
  /* The Localtime should be updated during the Ethernet packets processing */
  NVIC_SetPriority (SysTick_IRQn, 1);  
}

/****************************************************************************
* 名    称: static void ETH_MACDMA_Config(void)
* 功    能：ETH DMA初始化
* 入口参数： 
* 返回参数：无
* 说    明：       
****************************************************************************/
static void ETH_MACDMA_Config(void)
{
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
  //ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
  ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Disable; 
  ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
  ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;   

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
  if(ETH_Init(&ETH_InitStructure, PHY_ADDRESS)){
	printf("======PHY init ok!==== \r\n");				
  }else{
	printf("======PHY init ERROR!==== \r\n");				
  }  

  /* Enable the Ethernet Rx Interrupt */
  ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
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
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;  
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


