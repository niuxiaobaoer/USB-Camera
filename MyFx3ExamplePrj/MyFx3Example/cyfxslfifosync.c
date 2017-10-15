/*
 ## Cypress USB 3.0 Platform source file (cyfxslfifosync.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2011,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  WHICH IS THE PROPERTY OF CYPRESS.
 ##
 ##  Use of this file is governed
 ##  by the license agreement included in the file
 ##
 ##     <install>/license/license.txt
 ##
 ##  where <install> is the Cypress software
 ##  installation root directory path.
 ##
 ## ===========================
*/

/* This file illustrates the Slave FIFO Synchronous mode example */

/*
   This example comprises of two USB bulk endpoints. A bulk OUT endpoint acts as the
   producer of data from the host. A bulk IN endpoint acts as the consumer of data to
   the host. Appropriate vendor class USB enumeration descriptors with these two bulk
   endpoints are implemented.

   The GPIF configuration data for the Synchronous Slave FIFO operation is loaded onto
   the appropriate GPIF registers. The p-port data transfers are done via the producer
   p-port socket and the consumer p-port socket.

   This example implements two DMA Channels in MANUAL mode one for P to U data transfer
   and one for U to P data transfer.

   The U to P DMA channel connects the USB producer (OUT) endpoint to the consumer p-port
   socket. And the P to U DMA channel connects the producer p-port socket to the USB 
   consumer (IN) endpoint.

   Upon every reception of data in the DMA buffer from the host or from the p-port, the
   CPU is signalled using DMA callbacks. There are two DMA callback functions implemented
   each for U to P and P to U data paths. The CPU then commits the DMA buffer received so
   that the data is transferred to the consumer.

   The DMA buffer size for each channel is defined based on the USB speed. 64 for full
   speed, 512 for high speed and 1024 for super speed. CY_FX_SLFIFO_DMA_BUF_COUNT in the
   header file defines the number of DMA buffers per channel.

   The constant CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT in the header file is used to
   select 16bit or 32bit GPIF data bus configuration.
 */

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyfxslfifosync.h"
#include "cyu3gpif.h"
#include "cyu3pib.h"
#include "pib_regs.h"
/*added by qbc 2017-07-01*/
#include "cyu3i2c.h"
#include "cyu3spi.h"
#include "cyfxusbi2cregmode.h"

/* Firmware ID variable that may be used to verify I2C firmware. */
const uint8_t glFirmwareID[32] __attribute__ ((aligned (32))) = { 'F', 'X', '3', ' ', 'I', '2', 'C', '\0' };
uint8_t glEp0Buffer[4096] __attribute__ ((aligned (32)));
uint16_t glI2cPageSize = 0x40;   /* I2C Page size to be used for transfers. */




/* This file should be included only once as it contains
 * structure definitions. Including it in multiple places
 * can result in linker error. */
#include "cyfxgpif_syncsf.h"

CyU3PThread slFifoAppThread;	        /* Slave FIFO application thread structure */
CyU3PDmaChannel glChHandleSlFifoUtoP;   /* DMA Channel handle for U2P transfer. */
CyU3PDmaChannel glChHandleSlFifoPtoU;   /* DMA Channel handle for P2U transfer. */

uint32_t glDMARxCount = 0;               /* Counter to track the number of buffers received from USB. */
uint32_t glDMATxCount = 0;               /* Counter to track the number of buffers sent to USB. */
CyBool_t glIsApplnActive = CyFalse;      /* Whether the loopback application is active or not. */

/* ///////////////////////////////////////// */
uint8_t glUartRxp = 0;
uint8_t glUartRxBuffer[CY_FX_UART_MAX_BUFFER_SIZE]={0,1,2,3,4,5,6,7,8,9,10,12,13,14,15};
uint8_t glUartTxBuffer[CY_FX_UART_MAX_BUFFER_SIZE]={0,1,2,3,4,5,6,7,8,9,10,12,13,14,15};
/* ///////////////////////////////////////// */

/* Application Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* This function initializes the UART module */
void
CyFxUartApplnInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyTrue;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma = CyFalse;

    /* Set the UART configuration */
    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
    glUartRxp = 0;
}

/* This function starts the slave FIFO loop application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void
CyFxSlFifoApplnStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    /* Based on the Bus Speed configure the endpoint packet size */
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 512;
            break;

        case  CY_U3P_SUPER_SPEED:
            size = 1024;
            break;

        default:
            CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ? (CY_FX_EP_BURST_LENGTH) : 1;
    epCfg.streams = 0;
    epCfg.pcktSize = size;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Create a DMA MANUAL channel for U2P transfer.
     * DMA size is set based on the USB speed. */
    dmaCfg.size  = (usbSpeed == CY_U3P_SUPER_SPEED) ? (CY_FX_EP_BURST_LENGTH*size) : size;
    dmaCfg.count = CY_FX_SLFIFO_DMA_BUF_COUNT;
    dmaCfg.prodSckId = CY_FX_PRODUCER_USB_SOCKET;
    dmaCfg.consSckId = CY_FX_CONSUMER_PPORT_SOCKET;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    /* Enabling the callback for produce event. */
    dmaCfg.notification = 0;
    dmaCfg.cb = NULL;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleSlFifoUtoP,
    		CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Create a DMA MANUAL channel for P2U transfer. */
    dmaCfg.prodSckId = CY_FX_PRODUCER_PPORT_SOCKET;
    dmaCfg.consSckId = CY_FX_CONSUMER_USB_SOCKET;
    dmaCfg.cb = NULL;
    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleSlFifoPtoU,
    		CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Flush the Endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Set DMA channel transfer size. */
    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleSlFifoUtoP, CY_FX_SLFIFO_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleSlFifoPtoU, CY_FX_SLFIFO_DMA_RX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Update the status flag. */
    glIsApplnActive = CyTrue;
}

/* This function stops the slave FIFO loop application. This shall be called
 * whenever a RESET or DISCONNECT event is received from the USB host. The
 * endpoints are disabled and the DMA pipe is destroyed by this function. */
void
CyFxSlFifoApplnStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag. */
    glIsApplnActive = CyFalse;

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Destroy the channel */
    CyU3PDmaChannelDestroy (&glChHandleSlFifoUtoP);
    CyU3PDmaChannelDestroy (&glChHandleSlFifoPtoU);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }
}
/* I2C read / write for programmer application. */
CyU3PReturnStatus_t
CyFxUsbI2cTransfer (
        uint16_t  byteAddress,
        uint8_t   devAddr,
        uint16_t  byteCount,
        uint8_t  *buffer,
        CyBool_t  isRead)
{
    CyU3PI2cPreamble_t preamble;
    uint16_t pageCount = (byteCount / glI2cPageSize);
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint16_t resCount = glI2cPageSize;

    if (byteCount == 0)
    {
        return CY_U3P_SUCCESS;
    }

    if ((byteCount % glI2cPageSize) != 0)
    {
        pageCount ++;
        resCount = byteCount % glI2cPageSize;
    }

    CyU3PDebugPrint (2, "I2C access - dev: 0x%x, address: 0x%x, size: 0x%x, pages: 0x%x.\r\n",
            devAddr, byteAddress, byteCount, pageCount);

   // while (pageCount != 0)
    {
        if (isRead)
        {
            /* Update the preamble information. */
            preamble.length    = 4;
            preamble.buffer[0] = devAddr;
            preamble.buffer[1] = (uint8_t)(byteAddress >> 8);
            preamble.buffer[2] = (uint8_t)(byteAddress & 0xFF);
            preamble.buffer[3] = (devAddr | 0x01);
            preamble.ctrlMask  = 0x0004;

            status = CyU3PI2cReceiveBytes (&preamble, buffer, (pageCount == 1) ? resCount : glI2cPageSize, 0);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }
        }
        else /* Write */
        {
            /* Update the preamble information. */
            preamble.length    = 3;
            preamble.buffer[0] = devAddr;
            preamble.buffer[1] = (uint8_t)(byteAddress >> 8);
            preamble.buffer[2] = (uint8_t)(byteAddress & 0xFF);
            preamble.ctrlMask  = 0x0000;

            status = CyU3PI2cTransmitBytes (&preamble, buffer, (pageCount == 1) ? resCount : glI2cPageSize, 0);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }


            /* Wait for the write to complete. */
            preamble.length = 1;
            status = CyU3PI2cWaitForAck(&preamble, 200);
            if (status != CY_U3P_SUCCESS)
            {
                return status;
            }
        }

        /* An additional delay seems to be required after receiving an ACK. */
        CyU3PThreadSleep (1);

        /* Update the parameters */
        byteAddress  += glI2cPageSize;
        buffer += glI2cPageSize;
        pageCount --;
    }

    return CY_U3P_SUCCESS;
}

/* I2C write sensor reg. */
CyU3PReturnStatus_t
WrSensorReg (
		uint8_t   devAddr,
        uint16_t  byteAddress,
        uint16_t  wData)
{
    CyU3PI2cPreamble_t preamble;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    /* Update the preamble information. */
	preamble.length = 5;
	preamble.buffer[0] = devAddr;
	preamble.buffer[1] = (uint8_t)(byteAddress >> 8);
	preamble.buffer[2] = (uint8_t)(byteAddress & 0xFF);
	preamble.buffer[3] = (uint8_t)(wData >> 8);
	preamble.buffer[4] = (uint8_t)(wData & 0xFF);
	preamble.ctrlMask  = 0x0000;

	status = CyU3PI2cSendCommand (&preamble, 0, 0);
	if (status != CY_U3P_SUCCESS)
	{
		return status;
	}
	/* Wait for the write to complete. */
	preamble.length = 1;
	status = CyU3PI2cWaitForAck(&preamble, 200);
	if (status != CY_U3P_SUCCESS)
	{
		return status;
	}
    /* An additional delay seems to be required after receiving an ACK. */
   CyU3PThreadSleep (1);

	return CY_U3P_SUCCESS;
}
/* I2C read sensor reg. */
CyU3PReturnStatus_t
RdSensorReg (
		uint8_t   devAddr,
        uint16_t  byteAddresss)
{
    CyU3PI2cPreamble_t preamble;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    /* Update the preamble information. */
	preamble.length    = 4;
	preamble.buffer[0] = devAddr;
	preamble.buffer[1] = (uint8_t)(byteAddresss >>8);
	preamble.buffer[2] = (uint8_t)(byteAddresss & 0xFF);
	preamble.buffer[3] = (devAddr | 0x01);
	preamble.ctrlMask  = 0x0004;

	status = CyU3PI2cReceiveBytes (&preamble, glEp0Buffer, 2, 0);
	if (status != CY_U3P_SUCCESS)
	{
	 return status;
	}

	/* Wait for the write to complete. */
	preamble.length = 1;
	status = CyU3PI2cWaitForAck(&preamble, 200);
	if (status != CY_U3P_SUCCESS)
	{
		return status;
	}
    /* An additional delay seems to be required after receiving an ACK. */
    CyU3PThreadSleep (1);


	return CY_U3P_SUCCESS;
}

/* I2C write FPGA reg. */
CyU3PReturnStatus_t
WrFpgaReg (
		uint8_t   devAddr,
		uint8_t  byteAddress,
		uint8_t  Data)
{
    CyU3PI2cPreamble_t preamble;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    /* Update the preamble information. */
	preamble.length = 3;
	preamble.buffer[0] = devAddr;
	preamble.buffer[1] = byteAddress;
	preamble.buffer[2] = Data;
	preamble.ctrlMask  = 0x0000;

	status = CyU3PI2cSendCommand (&preamble, 0, 0);
	if (status != CY_U3P_SUCCESS)
	{
		return status;
	}
	/* Wait for the write to complete. */
	preamble.length = 1;
	status = CyU3PI2cWaitForAck(&preamble, 200);
	if (status != CY_U3P_SUCCESS)
	{
		return status;
	}
    /* An additional delay seems to be required after receiving an ACK. */
   CyU3PThreadSleep (1);

	return CY_U3P_SUCCESS;
}

/* I2C read FPGA reg. */
CyU3PReturnStatus_t
RdFpgaReg (
		uint8_t   devAddr,
		uint8_t  byteAddresss)
{
    CyU3PI2cPreamble_t preamble;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    /* Update the preamble information. */
	preamble.length    = 3;
	preamble.buffer[0] = devAddr;
	preamble.buffer[1] = byteAddresss;
	preamble.buffer[2] = (devAddr | 0x01);
	preamble.ctrlMask  = 0x0002;

	status = CyU3PI2cReceiveBytes (&preamble, glEp0Buffer, 1, 0);
	if (status != CY_U3P_SUCCESS)
	{
	 return status;
	}

	/* Wait for the write to complete. */
	preamble.length = 1;
	status = CyU3PI2cWaitForAck(&preamble, 200);
	if (status != CY_U3P_SUCCESS)
	{
		return status;
	}
    /* An additional delay seems to be required after receiving an ACK. */
    CyU3PThreadSleep (1);


	return CY_U3P_SUCCESS;
}
/* Callback to handle the USB setup requests. */
CyBool_t
CyFxSlFifoApplnUSBSetupCB (
        uint32_t setupdat0,
        uint32_t setupdat1
    )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function. */
	CyU3PI2cPreamble_t preamble;


    uint8_t  i2cAddr;
    uint8_t	 bFpgaAddr;
    uint8_t	 bFpgaValue;
    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex, wLength;
    CyBool_t isHandled = CyFalse;

    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);
    wLength   = ((setupdat1 & CY_U3P_USB_LENGTH_MASK)   >> CY_U3P_USB_LENGTH_POS);
    //注意，必须要有下面这个对CY_U3P_USB_STANDARD_RQT的处理，否则在contro center中对I2C读写会失败。
    //目前原因未知，可能是USB在起始阶段需要。
    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsApplnActive)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }
    }

    /* Handle supported vendor requests. */
    if (bType == CY_U3P_USB_VENDOR_RQT)
    {
        isHandled = CyTrue;

        switch (bRequest)
        {
            case CY_FX_RQT_ID_CHECK:
                CyU3PUsbSendEP0Data (8, (uint8_t *)glFirmwareID);
                break;

            case CY_FX_RQT_I2C_EEPROM_WRITE:
                //i2cAddr = 0xAE | ((wValue & 0x0007) << 1);
                i2cAddr = 0XAE;
                status  = CyU3PUsbGetEP0Data(((wLength + 15) & 0xFFF0), glEp0Buffer, NULL);
                //if (status == CY_U3P_SUCCESS)
                //{
                glEp0Buffer[0]=0x01;glEp0Buffer[1]=0x02;
                CyFxUsbI2cTransfer (0x00, i2cAddr, 0x02,
                            glEp0Buffer, CyFalse);
                //}
                break;

            case CY_FX_RQT_I2C_EEPROM_READ:
                i2cAddr = 0xA0 | ((wValue & 0x0007) << 1);
                CyU3PMemSet (glEp0Buffer, 0, sizeof (glEp0Buffer));
                status = CyFxUsbI2cTransfer (wIndex, i2cAddr, wLength,
                        glEp0Buffer, CyTrue);
                if (status == CY_U3P_SUCCESS)
                {
                    status = CyU3PUsbSendEP0Data(wLength, glEp0Buffer);
                }
                break;

            case 0xf0://sensor initialization
            	status  = CyU3PUsbGetEP0Data(((wLength + 15) & 0xFFF0), glEp0Buffer, NULL);

            	/*
            	WrSensorReg(0x20,0x3028,0x0010);//0		ROW_SPEED = 16
            	WrSensorReg(0x20,0x302A,0x000C);//1		VT_PIX_CLK_DIV = 12   P2   4<=P2<=16
            	WrSensorReg(0x20,0x302C,0x0001);//2		VT_SYS_CLK_DIV = 1    P1   1<=P1<=16
            	WrSensorReg(0x20,0x302E,0x0001);//3		PRE_PLL_CLK_DIV = 2   N    1<=N<=63
            	WrSensorReg(0x20,0x3030,0x0020);//4		PLL_MULTIPLIER = 40   M   32<=M<=255

            	WrSensorReg(0x20,0x3032,0x0000);//5		DIGITAL_BINNING = 0   _BINNING  帧率降一半
            	WrSensorReg(0x20,0x30B0,0x0080);//6		DIGITAL_TEST = 128

            	WrSensorReg(0x20,0x301A,0x00D8);//8		RESET_REGISTER = 216
            	WrSensorReg(0x20,0x301A,0x10DC);//9		RESET_REGISTER = 4316  h10DC    关键寄存器

            	WrSensorReg(0x20,0x3002,0x007C);//10	Y_ADDR_START = 124
            	WrSensorReg(0x20,0x3004,0x0002);//11	X_ADDR_START = 2
            	WrSensorReg(0x20,0x3006,0x034B);//12	Y_ADDR_END = 843
            	WrSensorReg(0x20,0x3008,0x0501);//13	X_ADDR_END = 1281

            	WrSensorReg(0x20,0x300A,0x02FD);//14	FRAME_LENGTH_LINES = 837
            	WrSensorReg(0x20,0x300C,0x056C);//15	LINE_LENGTH_PCK = 1388

            	WrSensorReg(0x20,0x3012,0x0080);//16	COARSE_INTEGRATION_TIME = 252	 h00FC	曝光时间
            	WrSensorReg(0x20,0x3014,0x008D);//17	FINE_INTEGRATION_TIME = 233

            	WrSensorReg(0x20,0x30A6,0x0001);//18	Y_ODD_INC = 1			    SKIP模式

            	WrSensorReg(0x20,0x3040,0x0000);//27	READ_MODE = 0			镜像等

            	WrSensorReg(0x20,0x3064,0x1982);//28	EMBEDDED_DATA_CTRL = 6530    开启输出 两行寄存器值 和  EMBEDDED_DATA  ，如果用AE模式 ，必须得开。图像输出时前两行不读

            	WrSensorReg(0x20,0x3100,0x0003);//30	AE;AG

            	WrSensorReg(0x20,0x305E,0x003C);//29	Total gain

            	WrSensorReg(0x20,0x3046,0x0100);
            	*/
                break;
            case 0xf1:
               	status  = CyU3PUsbGetEP0Data(((wLength + 15) & 0xFFF0), glEp0Buffer, NULL);
               	WrSensorReg(0x20,wIndex,wValue);
               	break;
            case 0xf2:
            	RdSensorReg(0x20,wIndex);
            	status = CyU3PUsbSendEP0Data(2, glEp0Buffer);
            	break;

            case 0xf3:
            	status  = CyU3PUsbGetEP0Data(((wLength + 15) & 0xFFF0), glEp0Buffer, NULL);
            	bFpgaAddr=wIndex&0xff;
            	bFpgaValue=wValue&0xff;
            	WrFpgaReg(0x78,bFpgaAddr,bFpgaValue);
            	break;
            case 0xf4:
            	bFpgaAddr=wIndex&0xff;
            	RdFpgaReg(0x78,bFpgaAddr);
            	status = CyU3PUsbSendEP0Data(1, glEp0Buffer);
            	break;

            default:
                /* This is unknown request. */
                isHandled = CyFalse;
                break;
        }

        /* If there was any error, return not handled so that the library will
         * stall the request. Alternatively EP0 can be stalled here and return
         * CyTrue. */
        if (status != CY_U3P_SUCCESS)
        {
            isHandled = CyFalse;
        }
    }
    return isHandled;
}


//The USB events of interest are: Set Configuration, Reset and Disconnect. The slave FIFO loop is
//started on receiving a SETCONF event and is stopped on a USB reset or USB disconnect.
/* This is the callback function to handle the USB events. */
void
CyFxSlFifoApplnUSBEventCB (
    CyU3PUsbEventType_t evtype,
    uint16_t            evdata
    )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            /* Stop the application before re-starting. */
            if (glIsApplnActive)
            {
                CyFxSlFifoApplnStop ();
            }
			CyU3PUsbLPMDisable();
            /* Start the loop back function. */
            CyFxSlFifoApplnStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the loop back function. */
            if (glIsApplnActive)
            {
                CyFxSlFifoApplnStop ();
            }
            break;

        default:
            break;
    }
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
 */
CyBool_t
CyFxApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* I2c initialization for EEPROM programming. */
CyU3PReturnStatus_t
CyFxI2cInit (uint16_t pageLen)
{
    CyU3PI2cConfig_t i2cConfig;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize and configure the I2C master module. */
    status = CyU3PI2cInit ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Start the I2C master block. The bit rate is set at 100KHz.
     * The data transfer is done via DMA. */
    CyU3PMemSet ((uint8_t *)&i2cConfig, 0, sizeof(i2cConfig));
    i2cConfig.bitRate    = CY_FX_USBI2C_I2C_BITRATE;
    i2cConfig.busTimeout = 0xFFFFFFFF;
    i2cConfig.dmaTimeout = 0xFFFF;
    i2cConfig.isDma      = CyFalse;

    status = CyU3PI2cSetConfig (&i2cConfig, NULL);
    if (status == CY_U3P_SUCCESS)
    {
        glI2cPageSize = pageLen;
    }

    return status;
}

/* Initialize all interfaces for the application. */
CyU3PReturnStatus_t
CyFxUsbI2cInit (
        void)
{
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the I2C interface for the EEPROM of page size 64 bytes. */
    status = CyFxI2cInit (CY_FX_USBI2C_I2C_PAGE_SIZE);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }
}
/* This function initializes the GPIF interface and initializes
 * the USB interface. */
void
CyFxSlFifoApplnInit (void)
{
    CyU3PPibClock_t pibClock;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the p-port block. */
    pibClock.clkDiv = 2;
    pibClock.clkSrc = CY_U3P_SYS_CLK;
    pibClock.isHalfDiv = CyFalse;
    /* Disable DLL for sync GPIF */
    pibClock.isDllEnable = CyFalse;
    apiRetStatus = CyU3PPibInit(CyTrue, &pibClock);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Load the GPIF configuration for Slave FIFO sync mode. */
    apiRetStatus = CyU3PGpifLoad (&Sync_Slave_Fifo_2Bit_CyFxGpifConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Start the state machine. */
    apiRetStatus = CyU3PGpifSMStart (SYNC_SLAVE_FIFO_2BIT_RESET, SYNC_SLAVE_FIFO_2BIT_ALPHA_RESET);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback(CyFxSlFifoApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(CyFxSlFifoApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(CyFxApplnLPMRqtCB);    

    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
	//The USB pins are connected. The FX3 USB device is visible to the host only after this action.
	//Hence it is important that all setup is completed before the USB pins are connected
    /* Connect the USB Pins with super speed operation enabled. */
    apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Entry function for the slFifoAppThread. */
void
SlFifoAppThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint8_t actualCount = 0;

	/* Initialize the UART example application */
	CyFxUartApplnInit();



	//CyU3PUartTransmitBytes(glUartTxBuffer, 10, &apiRetStatus );


    /* Initialize the slave FIFO application */
    CyFxSlFifoApplnInit();

    //added by qbc
    /* Initialize the application. */
    apiRetStatus = CyFxUsbI2cInit ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        //goto handle_error;
    	int a=0;
    	a=1;
    }


    for (;;)
    {
        actualCount = CyU3PUartReceiveBytes(glUartRxBuffer, 1, &apiRetStatus);
        if (actualCount != 0)
        {
            /* Check status */
            if (apiRetStatus != CY_U3P_SUCCESS)
            {
                /* Error handling */
                //CyFxAppErrorHandler(apiRetStatus);
            }

            glUartRxp=1;
        }
        else
        {
        	CyU3PThreadSleep (10);
        }
        if (glIsApplnActive)
        {
            /* Print the number of buffers received so far from the USB host. */
			//CyU3PDebugPrint (6, "Data tracker: buffers received: %d, buffers sent: %d.\n", glDMARxCount, glDMATxCount);
        }
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the thread */
    ptr = CyU3PMemAlloc (CY_FX_SLFIFO_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&slFifoAppThread,           /* Slave FIFO app thread structure */
                          "21:Slave_FIFO_sync",                    /* Thread ID and thread name */
                          SlFifoAppThread_Entry,                   /* Slave FIFO app thread entry function */
                          0,                                       /* No input parameter to thread */
                          ptr,                                     /* Pointer to the allocated thread stack */
                          CY_FX_SLFIFO_THREAD_STACK,               /* App Thread stack size */
                          CY_FX_SLFIFO_THREAD_PRIORITY,            /* App Thread priority */
                          CY_FX_SLFIFO_THREAD_PRIORITY,            /* App Thread pre-emption threshold */
                          CYU3P_NO_TIME_SLICE,                     /* No time slice for the application thread */
                          CYU3P_AUTO_START                         /* Start the thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread Creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
}

/*
 * Main function
 */
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    //////////////////////////////////////////////////////////////
    CyU3PSysClockConfig_t clkCfg = {
            CyTrue,
            2, 2, 2,
            CyFalse,
            CY_U3P_SYS_CLK
    };
    status = CyU3PDeviceInit (&clkCfg);
    //////////////////////////////////////////////////////////////
    /* Initialize the device */
    //status = CyU3PDeviceInit (NULL);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable instruction cache and keep data cache disabled.
     * The data cache is useful only when there is a large amount of CPU based memory
     * accesses. When used in simple cases, it can decrease performance due to large 
     * number of cache flushes and cleans and also it adds to the complexity of the
     * code. */
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port 
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. Here we are choosing
     * UART_ONLY configuration for 16 bit slave FIFO configuration and setting
     * isDQ32Bit for 32-bit slave FIFO configuration. */
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyTrue;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
#if (CY_FX_SLFIFO_GPIF_16_32BIT_CONF_SELECT == 0)
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
#else
    io_cfg.isDQ32Bit = CyTrue;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;//默认使能所有外设   CY_U3P_IO_MATRIX_LPP_UART_ONLY ?
#endif
    /* No GPIOs are enabled. */
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }
    CyU3PUsbLPMDisable();
    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:

    /* Cannot recover from this error. */
    while (1);
}

/* [ ] */

