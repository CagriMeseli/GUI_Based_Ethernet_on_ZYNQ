/*
 * Copyright (C) 2018 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include "xparameters.h"
#include "xstatus.h"
#include "netif/xadapter.h"
#include "platform.h"
#include "platform_config.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "sleep.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/init.h"
#include "lwip/inet.h"
#include "xuartps_hw.h"

#if LWIP_IPV6==1
#include "lwip/ip6_addr.h"
#include "lwip/ip6.h"
#else

#if LWIP_DHCP==0
#include "lwip/dhcp.h"
extern volatile int dhcp_timoutcntr;
#endif

#define DEFAULT_IP_ADDRESS	"192.168.200.102"
#define DEFAULT_IP_MASK		"255.255.255.0"
#define DEFAULT_GW_ADDRESS	"192.168.200.100"
#endif /* LWIP_IPV6 */

#define WAITING_FOR_START 0
#define WAITING_FOR_CHANNEL_SELECTION 1
#define TRANSMIT 2
#define UART_DEVICE_ID XPAR_PS7_UART_1_DEVICE_ID
#define XPAR_XUARTPS_0_BASEADDR 0xE0001000

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;


void platform_enable_interrupts(void);
void start_application_udp(void);
void start_application_udp_send(void);
void start_application_tcp(void);
void start_application_tcp_send(void);
void print_app_header_udp(void);
void print_app_header_udp_send(void);
void print_app_header_tcp(void);
void print_app_header_tcp_send(void);
void transfer_data_udp(void);
void transfer_data_tcp(void);

#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || \
		 XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
int ProgramSi5324(void);
int ProgramSfpPhy(void);
#endif
#endif

#ifdef XPS_BOARD_ZCU102
#ifdef XPAR_XIICPS_0_DEVICE_ID
int IicPhyReset(void);
#endif
#endif

struct netif server_netif;

#if LWIP_IPV6==1
static void print_ipv6(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf(" %s\n\r", inet6_ntoa(*ip));
}
#else
static void print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

static void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	print_ip("Board IP:       ", ip);
	print_ip("Netmask :       ", mask);
	print_ip("Gateway :       ", gw);
}

static void assign_default_ip(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	int err;

	xil_printf("Configuring default IP %s \r\n", DEFAULT_IP_ADDRESS);

	err = inet_aton(DEFAULT_IP_ADDRESS, ip);
	if (!err)
		xil_printf("Invalid default IP address: %d\r\n", err);

	err = inet_aton(DEFAULT_IP_MASK, mask);
	if (!err)
		xil_printf("Invalid default IP MASK: %d\r\n", err);

	err = inet_aton(DEFAULT_GW_ADDRESS, gw);
	if (!err)
		xil_printf("Invalid default gateway address: %d\r\n", err);
}
#endif /* LWIP_IPV6 */

#define CHAR_ESC		0x1b	/* 'ESC' character is used as terminator */

u32 UART_BASE_ADDR = XPAR_XUARTPS_0_BASEADDR;

int UartPs_mssg(u32 UART_BASE_ADDR,u8 *message);
int UartPs_ch(u32 UART_BASE_ADDR,u8 *channel);

u8 TCP_channel ;
u8 UDP_channel ;
u8 Tx_Count;
u32 Rx_Start;
u32 Tx_Start;
u8 message;
u8 channel;

int main(void)
{
	struct netif *netif;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] = {
		0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	netif = &server_netif;
#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || \
		XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
	ProgramSi5324();
	ProgramSfpPhy();
#endif
#endif

	/* Define this board specific macro in order perform PHY reset
	 * on ZCU102
	 */
#ifdef XPS_BOARD_ZCU102
	IicPhyReset();
#endif

	init_platform();

	xil_printf("\r\n\r\n");
	xil_printf("-----lwIP RAW Mode TCP Server Application-----\r\n");

	/* initialize lwIP */
	lwip_init();

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(netif, NULL, NULL, NULL, mac_ethernet_address,
				PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\r\n");
		return -1;
	}

#if LWIP_IPV6==1
	netif->ip6_autoconfig_enabled = 1;
	netif_create_ip6_linklocal_address(netif, 1);
	netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
	print_ipv6("\n\rlink local IPv6 address is:", &netif->ip6_addr[0]);
#endif /* LWIP_IPV6 */

	netif_set_default(netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	/* specify that the network if is up */
	netif_set_up(netif);

#if (LWIP_IPV6==0)
#if (LWIP_DHCP==0)
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	dhcp_start(netif);
	dhcp_timoutcntr = 24;
	while (((netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
		xemacif_input(netif);

	if (dhcp_timoutcntr <= 0) {
		if ((netif->ip_addr.addr) == 0) {
			xil_printf("ERROR: DHCP request timed out\r\n");
			assign_default_ip(&(netif->ip_addr),
					&(netif->netmask), &(netif->gw));
		}
	}

	/* print IP address, netmask and gateway */
#else
	assign_default_ip(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
#endif
	print_ip_settings(&(netif->ip_addr), &(netif->netmask), &(netif->gw));
#endif /* LWIP_IPV6 */

u8 CurrentState;
extern u32 UDP_Tx_Flag;
extern u32 TCP_Tx_Flag;
extern u32 UDP_Rx_Finish;
extern u32 TCP_Rx_Finish;

xil_printf("\r\n");

print_app_header_udp();
start_application_udp();
xil_printf("\r\n");

print_app_header_tcp();
start_application_tcp();
xil_printf("\r\n");


while (1) {
	while (Tx_Start == 0 && Rx_Start == 0){
		 switch (CurrentState) {
			 	 case WAITING_FOR_START:
					 TCP_channel = 0;
					 UDP_channel = 0;
					 message = 0;
					 channel = 0;
					 xil_printf("Transmit moda gecmek istiyor musun? Transmit baslatmak icin [Y], Receive baslatmak icin [N] . \r\n");
					 UartPs_mssg(UART_BASE_ADDR, &message);
					 if (message == 'Y' || message == 'y') {
						xil_printf("Yes mesaji alindi, transmit moda geciliyor.\r\n");
						CurrentState = WAITING_FOR_CHANNEL_SELECTION;
					 }
					 else if (message == 'N' || message == 'n'){
						Rx_Start = 1;
						xil_printf("No mesaji alindi verici moda geciliyor.\r\n");
						break;
					 }
					 else {
						xil_printf("Veri iletimi baslatilamamistir, lutfen tekrar mesaj giriniz.\r\n");
						CurrentState = WAITING_FOR_START;
					 }

			 	 case WAITING_FOR_CHANNEL_SELECTION:
			 		xil_printf("Veri kanalýný seçiniz; TCP = [T], UDP = [U] . \r\n");
					UartPs_ch(UART_BASE_ADDR, &channel);
					if (channel == 'T' || channel == 't') {
						xil_printf("TCP kanali seçildi.\r\n");
						TCP_channel = 1;
						print_app_header_tcp_send();
						start_application_tcp_send();
						xil_printf("\r\n");
						Tx_Start = 1;
						CurrentState = WAITING_FOR_START;
						break;
					}
					else if (channel == 'U' || channel == 'u') {
						xil_printf("UDP kanali seçildi.\r\n");
						UDP_channel = 1;
						print_app_header_udp_send();
						start_application_udp_send();
						xil_printf("\r\n");
						Tx_Start = 1;
						CurrentState = WAITING_FOR_START;
						break;
					}
					else {
						xil_printf("Hatalý giriþ. Lütfen T veya U giriniz.\r\n");
						CurrentState = WAITING_FOR_CHANNEL_SELECTION;
					}

				default:
					CurrentState = WAITING_FOR_START;
					break;
				}
		 }
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		if (UDP_channel) {
			transfer_data_udp();
		}
		else if (TCP_channel){
			transfer_data_tcp();
		}
		if (UDP_Tx_Flag || TCP_Tx_Flag || UDP_Rx_Finish || TCP_Rx_Finish) {
			Tx_Start 		= 0;
			Rx_Start 		= 0;
			UDP_Rx_Finish	= 0;
			UDP_Tx_Flag		= 0;
			TCP_Tx_Flag		= 0;
			TCP_Rx_Finish	= 0;
		}
		xemacif_input(netif);
    }
		cleanup_platform();

		return 0;
}


int UartPs_mssg(u32 UART_BASE_ADDR, u8 *message) {
    u8 RecvChar1;

    XUartPs_WriteReg(UART_BASE_ADDR, XUARTPS_CR_OFFSET,(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN));
    /* Mesajý bekleyin*/
    while (((*message != 'y') && (*message != 'Y') && (*message != 'N') && (*message != 'n'))) {
        while (!XUartPs_IsReceiveData(UART_BASE_ADDR));
        RecvChar1 = XUartPs_ReadReg(UART_BASE_ADDR, XUARTPS_FIFO_OFFSET);
        *message = RecvChar1;
        XUartPs_WriteReg(UART_BASE_ADDR, XUARTPS_FIFO_OFFSET, RecvChar1);
        if (((*message != 'y') && (*message != 'Y') && (*message != 'N') && (*message != 'n'))){
        	xil_printf("Yanlis giris lutfen [Y] veya [N] giriniz. \r\n");
        }

        xil_printf("\r\n");
    }

    return RecvChar1;
}

int UartPs_ch(u32 UART_BASE_ADDR,u8 *channel) {
    u8 RecvChar2;

    XUartPs_WriteReg(UART_BASE_ADDR, XUARTPS_CR_OFFSET,(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN));
    /* Kanalý bekleyin */
    while (((*channel != 'U') && (*channel != 'u') && (*channel != 'T') && (*channel != 't'))) {
        while (!XUartPs_IsReceiveData(UART_BASE_ADDR));
        RecvChar2 = XUartPs_ReadReg(UART_BASE_ADDR, XUARTPS_FIFO_OFFSET);
        *channel = RecvChar2;
        XUartPs_WriteReg(UART_BASE_ADDR, XUARTPS_FIFO_OFFSET, RecvChar2);
        if (((*channel != 'U') && (*channel != 'u') && (*channel != 'T') && (*channel != 't'))){
			xil_printf("Yanlis giris lutfen [U] veya [T] giriniz. \r\n");
		}
        xil_printf("\r\n");
    }

    return RecvChar2;
}





