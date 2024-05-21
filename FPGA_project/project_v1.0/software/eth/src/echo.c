#include <stdio.h>
#include <string.h>
#include "lwip/udp.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "xil_printf.h"

u8* TCP_Data;
u8* UDP_Data;

#define UDP_SERVER_PORT 5001
#define TCP_SERVER_PORT 5001

int transfer_data() {
	return 0;
}

err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	tcp_recved(tpcb, p->len);

	if (tcp_sndbuf(tpcb) > p->len) {
		err = tcp_write(tpcb, p->payload, p->len, 1);
		TCP_Data = p->payload;
		xil_printf("Received data: %s\n\r", (char *)p->payload);
	} else
		xil_printf("no space in tcp_sndbuf\n\r");

	pbuf_free(p);

	return ERR_OK;
}

err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	static int connection = 1;

	tcp_recv(newpcb, tcp_recv_callback);

	tcp_arg(newpcb, (void*)(UINTPTR)connection);

	connection++;

	return ERR_OK;
}


int start_tcp_application()
{
	struct tcp_pcb *tpcb;
	err_t err;

	tpcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!tpcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	err = tcp_bind(tpcb, IP_ANY_TYPE, TCP_SERVER_PORT);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", TCP_SERVER_PORT, err);
		return -2;
	}

	tcp_arg(tpcb, NULL);

	tpcb = tcp_listen(tpcb);
	if (!tpcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	tcp_accept(tpcb, tcp_accept_callback);

	xil_printf("TCP echo server started @ port %d\n\r", TCP_SERVER_PORT);

	return 0;
}

void udp_receive_callback(struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	u16_t len;
	len = p->len;
    if (len > 0) {
    	UDP_Data = (u8_t *)p->payload;
        xil_printf("Received data: %s\n\r", (char *)UDP_Data);
        pbuf_free(p);
    } else {
        xil_printf("Received empty UDP packet\n\r");
    }
}

int start_udp_server() {
    struct udp_pcb *upcb;
    err_t err;

    udp_init();
    upcb = udp_new();
    if (!upcb) {
        xil_printf("Error creating UDP PCB. Out of Memory\n\r");
        udp_remove(upcb);
        return ERR_MEM;
    }
    else {
    	xil_printf("UDP PCB'si basariyla olusturuldu. \n\r");
    }

    err = udp_bind(upcb, IP_ANY_TYPE, UDP_SERVER_PORT);
    if (err != ERR_OK) {
        xil_printf("UDP portu baglanamadi %d: err = %d\n\r", UDP_SERVER_PORT, err);
        return -2;
    }
    else{
    	xil_printf("UDP portu baglandi %d: \n\r", UDP_SERVER_PORT);
    }

    udp_recv(upcb, udp_receive_callback, NULL);

    xil_printf("UDP server started @ port %d\n\r", UDP_SERVER_PORT);

    return ERR_OK;
}
