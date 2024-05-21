/*
 * dma.h
 *
 *  Created on: 13 May 2024
 *      Author: MONSTER
 */

#ifndef DMA_H
#define DMA_H

#include <stdio.h>
#include "xdmaps.h"
#include "xscugic.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "xil_printf.h"
#include "xuartps.h"

#define PS_UART_DEVICE_ID XPAR_XUARTPS_0_DEVICE_ID
#define TRANSACTION_SIZE 1500
#define UART_DEVICE_ID XPAR_XUARTPS_0_DEVICE_ID

#define INTC_DEVICE_ID XPAR_SCUGIC_0_DEVICE_ID
#define INTC_BASEADDR XPAR_PS7_SCUGIC_0_BASEADDR
#define DMA_DEVICE_ID XPAR_XDMAPS_1_DEVICE_ID

#define INTC_DEVICE_ID XPAR_SCUGIC_0_DEVICE_ID
#define INTC_BASEADDR XPAR_PS7_SCUGIC_0_BASEADDR

#define DMA0_ID XPAR_XDMAPS_1_DEVICE_ID

u8 Dst_DDR[TRANSACTION_SIZE];

int uart(void);
void recv_msg_from_uart(void);
int dma(void);
int SetupInterruptSystem(XScuGic *GicInstancePtr, XDmaPs *DmaPtr);
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);
void DmaFaultHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);

u8 uart_rx_data[TRANSACTION_SIZE];
u8 ReceivedChar;

int msg_len;

#endif /* DMA_H */
