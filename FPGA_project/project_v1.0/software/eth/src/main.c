#include <stdio.h>
#include "xparameters.h"
#include "netif/xadapter.h"
#include "lwip/udp.h"
#include "platform.h"
#include "platform_config.h"
#include "xil_printf.h"
#include "lwip/tcp.h"
#include "xil_cache.h"
#include "xGpio.h"

#define SW_ADDR XPAR_AXI_GPIO_0_BASEADDR
#define LED_ADDR XPAR_AXI_GPIO_1_BASEADDR

void print_app_header();
int start_tcp_application();
int start_udp_server();
int transfer_data();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

void lwip_init();

extern u8* TCP_Data;
extern u8* UDP_Data;
extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
static struct netif server_netif;
struct netif *echo_netif;

void
print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void
print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

int main()
{
XGpio sw, led;
int sw_val;
int led_val;

XGpio_Initialize(&sw, XPAR_AXI_GPIO_0_DEVICE_ID); //sw
XGpio_SetDataDirection(&sw, 1, 0xFFFFFFFF);
XGpio_Initialize(&led, XPAR_AXI_GPIO_1_DEVICE_ID); //led
XGpio_SetDataDirection(&led, 1, 0x00000000);

#if LWIP_IPV6==0
	ip_addr_t ipaddr, netmask, gw;

#endif
	unsigned char mac_ethernet_address[] =
	{ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	echo_netif = &server_netif;

	init_platform();

	IP4_ADDR(&ipaddr,  192, 168,   1, 10);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);

	lwip_init();

#if (LWIP_IPV6 == 0)
	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
#endif
	netif_set_default(echo_netif);

	platform_enable_interrupts();

	netif_set_up(echo_netif);

#if (LWIP_IPV6 == 0)

	print_ip_settings(&ipaddr, &netmask, &gw);

#endif
	start_tcp_application();
	start_udp_server();
	xil_printf(" lwIP UDP ve TCP sunucusu basladi \r \n");
	while (1) {
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		xemacif_input(echo_netif);
		transfer_data();
		if ((TCP_Data || UDP_Data) != NULL) {
			switch (TCP_Data || UDP_Data) {
				case 0:
					led_val |= 0x00 ;
					break;
				case 1:
					led_val |= 0x01 ;
					break;
				case 2:
					led_val |= 0x02 ;
					break;
				case 3:
					led_val |= 0x04 ;
					break;
				default:
					led_val |= 0x00 ;
					break;
			}
		}
		else {
			led_val = 0x00;
		}
		XGpio_DiscreteWrite(&led, 1, led_val);
	}

	cleanup_platform();

	return 0;
}
