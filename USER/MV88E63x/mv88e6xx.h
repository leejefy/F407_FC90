#ifndef __MV88E6XX_H
#define __MV88E6XX_H
#include "common.h"
#include "stm32f4x7_eth.h"

#ifdef __cplusplus
 extern "C" {
#endif

extern ETH_DMADESCTypeDef *DMARxDscrTab;			//以太网DMA接收描述符数据结构体指针
extern ETH_DMADESCTypeDef *DMATxDscrTab;			//以太网DMA发送描述符数据结构体指针 
extern uint8_t *Rx_Buff; 							//以太网底层驱动接收buffers指针 
extern uint8_t *Tx_Buff; 							//以太网底层驱动发送buffers指针
extern ETH_DMADESCTypeDef  *DMATxDescToSet;			//DMA发送描述符追踪指针
extern ETH_DMADESCTypeDef  *DMARxDescToGet; 		//DMA接收描述符追踪指针 
extern ETH_DMA_Rx_Frame_infos *DMA_RX_FRAME_infos;	//DMA最后接收到的帧信息指针


#define PORT_SR	    0	/* port status register */	
#define PHYSICAL_CR	    1	/* physical control register */
#define PORT_CR			4/* port control register */


#define PORT_LinkValue             ((uint16_t)0x0200)      /*!< set port register link value */
#define PORT_ForcedLink            ((uint16_t)0x0100)      /*!< set port register forced link mode */
#define PORT_DplexValue     		((uint16_t)0x0080)      /*!< set port register dplex value */
#define PORT_ForcedDpx             ((uint16_t)0x0040)      /*!< set port register forced dplex mode */
#define PORT_STATE 				((uint16_t)0x0003)      /*!< set port register state */

#define PORT_Duplex_Mode           	  ((uint16_t)0x0400)      /*!< port duplex mode */
#define PORT_Linked_Status               ((uint16_t)0x0800)      /*!< port link status */
#define PORT_Speed_Mode                 ((uint16_t)0x0300)      /*!< port speed mode */

/* The internal SERDES on port 0 is mapped at SMI device address 0x0C respectively */
#define PHY_2_LPORT_0				0x0C  

#define PHY_2_LPORT_3 				0x03

#define CPU_PORT					0x15    // port number 5

#define ETH1_PORT					0x13    // port number 3

#define UPLINK_PORT				0x10   // Fiber port, port number 0

void ETH_GPIO_Config(void);
void SFP_GPIO_Config(void);
static void ETH_NVIC_Config(void);
static u8 ETH_MACDMA_Config(void);
	 
u8 mv88e6xx_init(void);

FrameTypeDef ETH_Rx_Packet(void);
u8 ETH_Tx_Packet(u16 FrameLength);
u32 ETH_GetCurrentTxBuffer(void);
u8 ETH_Mem_Malloc(void);
void ETH_Mem_Free(void);

void switch_default_config(void);

#ifdef __cplusplus
}
#endif

#endif  /* __MV88E6XX_H */



