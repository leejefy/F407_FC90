#include <Copyright.h>
/********************************************************************************
* ev96122mii.c
*
* DESCRIPTION:
*       SMI access routines for EV-96122 board
*
* DEPENDENCIES:   Platform.
*
* FILE REVISION NUMBER:
*
*******************************************************************************/

#include "stm32f4x7_eth.h"
#include <msSample.h>
#include "string.h"
#include "gtDrvSwRegs.h"


/*
 * For each platform, all we need is 
 * 1) Assigning functions into 
 *         fgtReadMii : to read MII registers, and
 *         fgtWriteMii : to write MII registers.
 *
 * 2) Register Interrupt (Not Defined Yet.)
*/

/* 
 *  EV-96122 Specific Definition
*/

#define SMI_OP_CODE_BIT_READ                    1
#define SMI_OP_CODE_BIT_WRITE                   0
//#define SMI_BUSY                                1<<28
#define READ_VALID                              0x0800


#ifdef FIREFOX
#define ETHER_SMI_REG                   0x10 
#define internalRegBaseAddr 0x80008000
#define NONE_CACHEABLE        0x00000000
#define CACHEABLE            0x00000000
#define SMI_RX_TIMEOUT        1000
#else
#define ETHER_SMI_REG                   0x080810 
#define internalRegBaseAddr 0x14000000
#define NONE_CACHEABLE        0xa0000000
#define CACHEABLE            0x80000000
#define SMI_RX_TIMEOUT        10000000
#endif

typedef unsigned int              SMI_REG;

#ifdef LE /* Little Endian */              
#define SHORT_SWAP(X) (X)
#define WORD_SWAP(X) (X)
#define LONG_SWAP(X) ((l64)(X))

#else    /* Big Endian */
#define SHORT_SWAP(X) ((X <<8 ) | (X >> 8))

#define WORD_SWAP(X) (((X)&0xff)<<24)+      \
                    (((X)&0xff00)<<8)+      \
                    (((X)&0xff0000)>>8)+    \
                    (((X)&0xff000000)>>24)

#define LONG_SWAP(X) ( (l64) (((X)&0xffULL)<<56)+               \
                            (((X)&0xff00ULL)<<40)+              \
                            (((X)&0xff0000ULL)<<24)+            \
                            (((X)&0xff000000ULL)<<8)+           \
                            (((X)&0xff00000000ULL)>>8)+         \
                            (((X)&0xff0000000000ULL)>>24)+      \
                            (((X)&0xff000000000000ULL)>>40)+    \
                            (((X)&0xff00000000000000ULL)>>56))   

#endif

#define GT_REG_READ(offset, pData)                                          \
*pData = ( (volatile unsigned int)*((unsigned int *)                        \
           (NONE_CACHEABLE | internalRegBaseAddr | (offset))) );            \
*pData = WORD_SWAP(*pData)

#define GT_REG_WRITE(offset, data)                                          \
(volatile unsigned int)*((unsigned int *)(NONE_CACHEABLE |                  \
          internalRegBaseAddr | (offset))) = WORD_SWAP(data)

#define IS_MV88E6XX_PHY_ADDRESS(ADDRESS) ((ADDRESS) <= 0xF)


typedef enum _bool{false,true} bool;

/*****************************************************************************
*
* bool etherReadMIIReg (unsigned int portNumber , unsigned int MIIReg,
* unsigned int* value)
*
* Description
* This function will access the MII registers and will read the value of
* the MII register , and will retrieve the value in the pointer.
* Inputs
* portNumber - one of the 2 possiable Ethernet ports (0-1).
* MIIReg - the MII register offset.
* Outputs
* value - pointer to unsigned int which will receive the value.
* Returns Value
* true if success.
* false if fail to make the assignment.
* Error types (and exceptions if exist)
*/

#if 0
GT_BOOL gtBspReadMii (GT_QD_DEV* dev, unsigned int portNumber , unsigned int MIIReg,
                        unsigned int* value)
{
SMI_REG smiReg;
unsigned int phyAddr;
unsigned int timeOut = 10; /* in 100MS units */
int i;

/* first check that it is not busy */
    GT_REG_READ (ETHER_SMI_REG,(unsigned int*)&smiReg);
    if(smiReg & SMI_BUSY) 
    {
        for(i = 0 ; i < SMI_RX_TIMEOUT ; i++);
        do {
            GT_REG_READ (ETHER_SMI_REG,(unsigned int*)&smiReg);
            if(timeOut-- < 1 ) {
                return false;
            }
        } while (smiReg & SMI_BUSY);
    }
/* not busy */

    phyAddr = portNumber;

    smiReg =  (phyAddr << 16) | (SMI_OP_CODE_BIT_READ << 26) | (MIIReg << 21) |
         SMI_OP_CODE_BIT_READ<<26;

    GT_REG_WRITE (ETHER_SMI_REG,*((unsigned int*)&smiReg));
    timeOut = 10; /* initialize the time out var again */
    GT_REG_READ (ETHER_SMI_REG,(unsigned int*)&smiReg);
    if(!(smiReg & READ_VALID)) 
        {
            i=0;
            while(i < SMI_RX_TIMEOUT)
            {
                i++;
            }
        {
        }
        do {
            GT_REG_READ (ETHER_SMI_REG,(unsigned int*)&smiReg);
            if(timeOut-- < 1 ) {
                return false;
            }
        } while (!(smiReg & READ_VALID));
     }
    *value = (unsigned int)(smiReg & 0xffff);
    
    return true;


}
#endif
/*
smiReg =  QD_SMI_BUSY 
		| (devAddr << QD_SMI_DEV_ADDR_BIT) 
		| (QD_SMI_WRITE << QD_SMI_OP_BIT) 
		| (regAddr << QD_SMI_REG_ADDR_BIT) 
		| (QD_SMI_CLAUSE22 << QD_SMI_MODE_BIT); 

	ret = ETH_WritePHYRegister(0x1C,QD_REG_SMI_PHY_CMD,smiReg);
	_eth_delay_(PHY_RESET_DELAY);

	while(ETH_ReadPHYRegister(0x1C, QD_REG_SMI_PHY_CMD) & QD_SMI_BUSY)
	{	
		timeout++;
		printf("\r\n Write_PHY_SMI busy time : %d \r\n",timeout);
	}
	if(timeout == PHY_WRITE_TO)
	{
	   return ETH_ERROR;
	}

	ret = ETH_WritePHYRegister(0x1C,QD_REG_SMI_PHY_DATA,data);

*/


GT_BOOL gtBspReadMii (GT_QD_DEV* dev, unsigned int devAddr , unsigned int regAddr,
                        unsigned int* value)
{

SMI_REG smiReg;
unsigned int phyAddr;
unsigned int timeOut = 10; /* in 100MS units */
int i;
int ret;

	if( IS_MV88E6XX_PHY_ADDRESS(devAddr) )
	{
		/* first check that it is not busy */
		smiReg =  QD_SMI_BUSY 
			| (1 << QD_SMI_MODE_BIT) /* 0 - clause 45, 1 - clause 22 */
			| (QD_SMI_READ_22 << QD_SMI_OP_BIT)
			| (devAddr << QD_SMI_DEV_ADDR_BIT) 
			| (regAddr << QD_SMI_REG_ADDR_BIT);
		
		ret = ETH_WritePHYRegister(0x1C,QD_REG_SMI_PHY_CMD,smiReg);
	
		if( ret && (ETH_ReadPHYRegister(0x1C,QD_REG_SMI_PHY_CMD) & QD_SMI_BUSY)) 
		{
			for(i = 0 ; i < SMI_RX_TIMEOUT ; i++);
			do {
				smiReg = ETH_ReadPHYRegister(0x1C,QD_REG_SMI_PHY_CMD);
				if(timeOut-- < 1 ) {
					return false;
				}
			} while (smiReg & QD_SMI_BUSY);
		}
		
		/* not busy */
	
		*value = ETH_ReadPHYRegister(0x1C,QD_REG_SMI_PHY_DATA);
		
	}else {
		*value = ETH_ReadPHYRegister(devAddr,regAddr);
	}

    return true;


}


/*****************************************************************************
* 
* bool etherWriteMIIReg (unsigned int portNumber , unsigned int MIIReg,
* unsigned int value)
* 
* Description
* This function will access the MII registers and will write the value
* to the MII register.
* Inputs
* portNumber - one of the 2 possiable Ethernet ports (0-1).
* MIIReg - the MII register offset.
* value -the value that will be written.
* Outputs
* Returns Value
* true if success.
* false if fail to make the assignment.
* Error types (and exceptions if exist)
*/
#if 0
GT_BOOL gtBspWriteMii (GT_QD_DEV* dev, unsigned int portNumber , unsigned int MIIReg,
                       unsigned int value)
{
SMI_REG smiReg;
unsigned int phyAddr;
unsigned int timeOut = 10; /* in 100MS units */
int i;

/* first check that it is not busy */
    GT_REG_READ (ETHER_SMI_REG,(unsigned int*)&smiReg);
    if(smiReg & SMI_BUSY) 
    {
        for(i = 0 ; i < SMI_RX_TIMEOUT ; i++);
        do {
            GT_REG_READ (ETHER_SMI_REG,(unsigned int*)&smiReg);
            if(timeOut-- < 1 ) {
                return false;
            }
        } while (smiReg & SMI_BUSY);
    }
/* not busy */

    phyAddr = portNumber;

    smiReg = 0; /* make sure no garbage value in reserved bits */
    smiReg = smiReg | (phyAddr << 16) | (SMI_OP_CODE_BIT_WRITE << 26) |
             (MIIReg << 21) | (value & 0xffff);

    GT_REG_WRITE (ETHER_SMI_REG,*((unsigned int*)&smiReg));

    return(true);
}
#endif

GT_BOOL gtBspWriteMii (GT_QD_DEV* dev, unsigned int devAddr , unsigned int regAddr,
                       unsigned int value)
{	
	SMI_REG smiReg;
	unsigned int timeOut = 10; /* in 100MS units */
	int i;
	int ret;

	if( IS_MV88E6XX_PHY_ADDRESS(devAddr) )
	{	
		/* first check that it is not busy */
		  
		smiReg =  QD_SMI_BUSY 
			| (devAddr << QD_SMI_DEV_ADDR_BIT) 
			| (QD_SMI_WRITE << QD_SMI_OP_BIT) 
			| (regAddr << QD_SMI_REG_ADDR_BIT) 
			| (QD_SMI_CLAUSE22 << QD_SMI_MODE_BIT); 
		
		
		ret = ETH_WritePHYRegister(0x1C,QD_REG_SMI_PHY_CMD,smiReg);

		if( ret && (ETH_ReadPHYRegister(0x1C,QD_REG_SMI_PHY_CMD) & QD_SMI_BUSY)) 
		{
			for(i = 0 ; i < SMI_RX_TIMEOUT ; i++);
			do {
				smiReg = ETH_ReadPHYRegister(0x1C,QD_REG_SMI_PHY_CMD);
				if(timeOut-- < 1 ) {
					return false;
				}
			} while (smiReg & QD_SMI_BUSY);
		}
		
	/* not busy */

		ret = ETH_WritePHYRegister(0x1C,QD_REG_SMI_PHY_DATA,value);
	}else{
		ret = ETH_WritePHYRegister(devAddr,regAddr,value);
	}
	return ret;

}


void gtBspMiiInit(GT_QD_DEV* dev)
{
    return;    
}
