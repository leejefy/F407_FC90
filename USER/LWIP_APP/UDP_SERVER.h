#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_



/***************������ip��MAC����*************************/
#define IMT407G_IP  			 	192,168,1,6   		//������ip 
#define IMT407G_NETMASK   	255,255,255,0   		//��������������
#define IMT407G_WG		   	 	192,168,1,1   			//������������
#define IMT407G_MAC_ADDR    00,11,22,33,44,1				  //������MAC��ַ

#define UDP_LOCAL_PORT     	2040

void UDP_server_init(void);


#endif

