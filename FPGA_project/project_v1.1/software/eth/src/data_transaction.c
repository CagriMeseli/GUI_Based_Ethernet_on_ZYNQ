/*
 * dma.c
 *
 *  Created on: 13 May 2024
 *      Author: MONSTER
 */


#include "data_transaction.h"

int Status;

XDmaPs_Config *DmaCfg;
XDmaPs Dma;

XDmaPs_Cmd DmaCmd = {
    .ChanCtrl = {
        .SrcBurstSize = 4,
        .SrcBurstLen = 4,
        .SrcInc = 1,
        .DstBurstSize = 4,
        .DstBurstLen = 4,
        .DstInc = 1,
    },
};

XScuGic Intc;

XTime tStart, tEnd;

XUartPs UartPs;
XUartPs_Config *Config;

u8 Src[TRANSACTION_SIZE];

unsigned int Channel;
int Done;
int Error;

int dma(void) {
    Status = SetupInterruptSystem(&Intc, &Dma);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    XDmaPs_ResetManager(&Dma);
    XDmaPs_ResetChannel(&Dma, Channel);

    Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    if (Config == NULL) {
        xil_printf("uart lookupconfig failed\r\n");
        return XST_FAILURE;
    }

    Status = XUartPs_CfgInitialize(&UartPs, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("uart cfg_init failed\r\n");
        return XST_FAILURE;
    }

    DmaCfg = XDmaPs_LookupConfig(DMA_DEVICE_ID);
    if (!DmaCfg) {
        xil_printf("Lookup DMAC %d failed\r\n", DMA_DEVICE_ID);
        return XST_FAILURE;
    }

    Status = XDmaPs_CfgInitialize(&Dma, DmaCfg, DmaCfg->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("XDmaPs_CfgInitialize failed\r\n");
        return XST_FAILURE;
    }

    XDmaPs_SetDoneHandler(&Dma, 0, DmaDoneHandler, (void *)0);
    XDmaPs_SetFaultHandler(&Dma, DmaFaultHandler, (void *)0);

    recv_msg_from_uart();

    // Setup DMA transfer with dynamic length
    DmaCmd.BD.SrcAddr = (u32)uart_rx_data;
    DmaCmd.BD.DstAddr = (u32)Dst_DDR;
    DmaCmd.BD.Length = strlen((char *)uart_rx_data); // Dynamic length

    Done = 0;
    Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);
    XTime_GetTime(&tStart);
    if (Status == XST_SUCCESS) {
        while (Done == 0);
    }
    XTime_GetTime(&tEnd);

    //u64 et_dma_ddr = tEnd - tStart;

    xil_printf("Received message from DDR: %s\r\n", (char *)Dst_DDR);

    return XST_SUCCESS;
}


int SetupInterruptSystem(XScuGic *GicInstancePtr, XDmaPs *DmaPtr) {

    XScuGic_Config *IntcConfig;

    Xil_ExceptionInit();

    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
    Status = XScuGic_CfgInitialize(GicInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, GicInstancePtr);

    Status = XScuGic_Connect(GicInstancePtr, XPAR_XDMAPS_0_FAULT_INTR, (Xil_InterruptHandler)XDmaPs_FaultISR, (void *)DmaPtr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    Status = XScuGic_Connect(GicInstancePtr, XPAR_XDMAPS_0_DONE_INTR_0, (Xil_InterruptHandler)XDmaPs_DoneISR_0, (void *)DmaPtr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    XScuGic_Enable(GicInstancePtr, XPAR_XDMAPS_0_DONE_INTR_0);

    Xil_ExceptionEnable();
    Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

    return XST_SUCCESS;
}

void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef) {
    Done = 1;
}

void DmaFaultHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef) {
    Error = 1;
}

void recv_msg_from_uart(void) {
    u16 ReceivedCount = 0;
    u8 ReceivedChar;

    while (1) {
        if (!XUartPs_IsReceiveData(Config->BaseAddress)) continue;

        ReceivedChar = XUartPs_ReadReg(Config->BaseAddress, XUARTPS_FIFO_OFFSET);

        if (ReceivedChar == '\r' || ReceivedChar == '\n') {
            uart_rx_data[ReceivedCount] = '\0';
            break;
        }

        if (ReceivedCount < sizeof(uart_rx_data) - 1) {
            uart_rx_data[ReceivedCount++] = ReceivedChar;
        }
    }
    xil_printf("Received message from uart: %s\r\n", uart_rx_data);
}
