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
#define		PHY_ADDRESS     0x3    // MV88E6321 PHYоƬ��ַ.

ETH_DMADESCTypeDef *DMARxDscrTab;	//��̫��DMA�������������ݽṹ��ָ��
ETH_DMADESCTypeDef *DMATxDscrTab;	//��̫��DMA�������������ݽṹ��ָ�� 
uint8_t *Rx_Buff; 					//��̫���ײ���������buffersָ�� 
uint8_t *Tx_Buff; 					//��̫���ײ���������buffersָ��

extern GT_QD_DEV       *dev;


/****************************************************************************
* ��    ��: u8 mv88e6xx_init(void)
* ��    �ܣ�ETH��ʼ��
* ��ڲ����� 
* ���ز�������
* ˵    ����       
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
	return !ret;     //ETH�Ĺ���Ϊ:0,ʧ��;1,�ɹ�;����Ҫȡ��һ�� 
  
}

/****************************************************************************
* ��    ��: static u8 ETH_MACDMA_Config(void)
* ��    �ܣ�ETH DMA��ʼ��
* ��ڲ����� 
* ���ز�������
* ˵    ����       
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
* ��    ��: void ETH_GPIO_Config(void)
* ��    �ܣ�ETH GPIO��ʼ��
* ��ڲ����� 
* ���ز�������
* ˵    �����Խ�MV88E6321��IO��ʼ��       
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

	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_MII); //MAC��PHY֮��ʹ��MII�ӿ�	
	
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
* ��    ��: void SFP_GPIO_Config(void)
* ��    �ܣ�SFP GPIO��ʼ��
* ��ڲ����� 
* ���ز�������
* ˵    ����1000M ��ģ���IO��ʼ��       

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
	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_OUT;          //��ͨ���ģʽ  ???
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
* ��    ��: void ETH_NVIC_Config(void)
* ��    �ܣ�ETH �жϳ�ʼ��
* ��ڲ����� 
* ���ز�������
* ˵    �����Խ�MV88E6321��IO��ʼ��       
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

extern void lwip_pkt_handle(void);		//��lwip_comm.c���涨��
//��̫��DMA�����жϷ�����
void ETH_IRQHandler(void)
{
	while(ETH_GetRxPktSize(DMARxDescToGet)!=0) 	//����Ƿ��յ����ݰ�
	{ 
		lwip_pkt_handle();		
	}
	ETH_DMAClearITPendingBit(ETH_DMA_IT_R); 	//���DMA�жϱ�־λ
	ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);	//���DMA�����жϱ�־λ
}  
//����һ���������ݰ�
//����ֵ:�������ݰ�֡�ṹ��
FrameTypeDef ETH_Rx_Packet(void)
{ 
	u32 framelength=0;
	FrameTypeDef frame={0,0};   
	//��鵱ǰ������,�Ƿ�����ETHERNET DMA(���õ�ʱ��)/CPU(��λ��ʱ��)
	if((DMARxDescToGet->Status&ETH_DMARxDesc_OWN)!=(u32)RESET)
	{	
		frame.length=ETH_ERROR; 
		if ((ETH->DMASR&ETH_DMASR_RBUS)!=(u32)RESET)  
		{ 
			ETH->DMASR = ETH_DMASR_RBUS;//���ETH DMA��RBUSλ 
			ETH->DMARPDR=0;//�ָ�DMA����
		}
		return frame;//����,OWNλ��������
	}  
	if(((DMARxDescToGet->Status&ETH_DMARxDesc_ES)==(u32)RESET)&& 
	((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET)&&  
	((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET))  
	{       
		framelength=((DMARxDescToGet->Status&ETH_DMARxDesc_FL)>>ETH_DMARxDesc_FrameLengthShift)-4;//�õ����հ�֡����(������4�ֽ�CRC)
 		frame.buffer = DMARxDescToGet->Buffer1Addr;//�õ����������ڵ�λ��
	}else framelength=ETH_ERROR;//����  
	frame.length=framelength; 
	frame.descriptor=DMARxDescToGet;  
	//����ETH DMAȫ��Rx������Ϊ��һ��Rx������
	//Ϊ��һ��buffer��ȡ������һ��DMA Rx������
	DMARxDescToGet=(ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);   
	return frame;  
}
//����һ���������ݰ�
//FrameLength:���ݰ�����
//����ֵ:ETH_ERROR,����ʧ��(0)
//		ETH_SUCCESS,���ͳɹ�(1)
u8 ETH_Tx_Packet(u16 FrameLength)
{   
	//��鵱ǰ������,�Ƿ�����ETHERNET DMA(���õ�ʱ��)/CPU(��λ��ʱ��)
	if((DMATxDescToSet->Status&ETH_DMATxDesc_OWN)!=(u32)RESET)return ETH_ERROR;//����,OWNλ�������� 
 	DMATxDescToSet->ControlBufferSize=(FrameLength&ETH_DMATxDesc_TBS1);//����֡����,bits[12:0]
	DMATxDescToSet->Status|=ETH_DMATxDesc_LS|ETH_DMATxDesc_FS;//�������һ���͵�һ��λ����λ(1������������һ֡)
  	DMATxDescToSet->Status|=ETH_DMATxDesc_OWN;//����Tx��������OWNλ,buffer�ع�ETH DMA
	if((ETH->DMASR&ETH_DMASR_TBUS)!=(u32)RESET)//��Tx Buffer������λ(TBUS)�����õ�ʱ��,������.�ָ�����
	{ 
		ETH->DMASR=ETH_DMASR_TBUS;//����ETH DMA TBUSλ 
		ETH->DMATPDR=0;//�ָ�DMA����
	} 
	//����ETH DMAȫ��Tx������Ϊ��һ��Tx������
	//Ϊ��һ��buffer����������һ��DMA Tx������ 
	DMATxDescToSet=(ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);    
	return ETH_SUCCESS;   
}
//�õ���ǰ��������Tx buffer��ַ
//����ֵ:Tx buffer��ַ
u32 ETH_GetCurrentTxBuffer(void)
{  
  return DMATxDescToSet->Buffer1Addr;//����Tx buffer��ַ  
}

//ΪETH�ײ����������ڴ�
//����ֵ:0,����
//    ����,ʧ��
u8 ETH_Mem_Malloc(void)
{ 
	DMARxDscrTab=mymalloc(SRAMIN,ETH_RXBUFNB*sizeof(ETH_DMADESCTypeDef));//�����ڴ�
	DMATxDscrTab=mymalloc(SRAMIN,ETH_TXBUFNB*sizeof(ETH_DMADESCTypeDef));//�����ڴ�  
	Rx_Buff=mymalloc(SRAMIN,ETH_RX_BUF_SIZE*ETH_RXBUFNB);	//�����ڴ�
	Tx_Buff=mymalloc(SRAMIN,ETH_TX_BUF_SIZE*ETH_TXBUFNB);	//�����ڴ�
	if(!DMARxDscrTab||!DMATxDscrTab||!Rx_Buff||!Tx_Buff)
	{
		ETH_Mem_Free();
		return 1;	//����ʧ��
	}	
	return 0;		//����ɹ�
}

//�ͷ�ETH �ײ�����������ڴ�
void ETH_Mem_Free(void)
{ 
	myfree(SRAMIN,DMARxDscrTab);//�ͷ��ڴ�
	myfree(SRAMIN,DMATxDscrTab);//�ͷ��ڴ�
	myfree(SRAMIN,Rx_Buff);		//�ͷ��ڴ�
	myfree(SRAMIN,Tx_Buff);		//�ͷ��ڴ�  
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




