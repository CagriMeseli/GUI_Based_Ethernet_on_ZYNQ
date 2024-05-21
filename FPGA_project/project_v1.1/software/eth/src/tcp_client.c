#include "tcp_perf_client.h"
#include <stdio.h>
#include <string.h>
#include "lwip/udp.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "xil_printf.h"
#include "lwip/inet.h"
#include "data_transaction.h"

static struct tcp_pcb *c_pcb;
static char send_buf[TCP_SEND_BUFSIZE];
static struct perf_stats client;
char message[51] = "fpga gomsis fpga to gui tcp gonderim testi basarili";

static void tcp_client_close(struct tcp_pcb *pcb)
{
	err_t err;

	if (pcb != NULL) {
		tcp_sent(pcb, NULL);
		tcp_err(pcb, NULL);
		err = tcp_close(pcb);
		if (err != ERR_OK) {
			tcp_abort(pcb);
		}
	}
}

static err_t tcp_send_traffic(void)
{
	err_t err;
	u8_t apiflags = TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE;

	if (c_pcb == NULL) {
		return ERR_CONN;
	}

	while (tcp_sndbuf(c_pcb) > TCP_SEND_BUFSIZE) {
		err = tcp_write(c_pcb, send_buf, TCP_SEND_BUFSIZE, apiflags);
		if (err != ERR_OK) {
			xil_printf("TCP client: Error on tcp_write: %d\r\n",
					err);
			return err;
		}

		err = tcp_output(c_pcb);
		if (err != ERR_OK) {
			xil_printf("TCP client: Error on tcp_output: %d\r\n",
					err);
			return err;
		}
	}
	return ERR_OK;
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	return tcp_send_traffic();
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if (err != ERR_OK) {
		tcp_client_close(tpcb);
		xil_printf("Connection error\n\r");
		return err;
	}
	/* store state */
	c_pcb = tpcb;

	/* set callback values & functions */
	tcp_arg(c_pcb, NULL);
	tcp_sent(c_pcb, tcp_client_sent);

	/* initiate data transfer */
	return ERR_OK;
}

void transfer_data(void)
{
	tcp_send_traffic();
}

void start_client_application(void)
{
	err_t err;
	struct tcp_pcb* pcb;
	ip_addr_t remote_addr;

	err = inet_aton(TCP_SERVER_IP_ADDRESS, &remote_addr);

	if (!err) {
		xil_printf("Invalid Server IP address: %d\r\n", err);
		return;
	}

	/* Create Client PCB */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error in PCB creation. out of memory\r\n");
		return;
	}

	err = tcp_connect(pcb, &remote_addr, TCP_CONN_PORT,
			tcp_client_connected);
	if (err) {
		xil_printf("Error on tcp_connect: %d\r\n", err);
		tcp_client_close(pcb);
		return;
	}

	memset(send_buf, 0, sizeof(send_buf));
	strncpy(send_buf, message, sizeof(send_buf) - 1);

	return;
}
