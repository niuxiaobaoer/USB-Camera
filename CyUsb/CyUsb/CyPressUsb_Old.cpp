#include "StdAfx.h"
#include "CyPressUsb_Old.h"
#include "stdio.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CCyPressUsb_Old::CCyPressUsb_Old(void)
{
	m_hUsb=INVALID_HANDLE_VALUE;
}

CCyPressUsb_Old::~CCyPressUsb_Old(void)
{
}

int CCyPressUsb_Old::OpenUsb()
{
	CloseUsb();
	char   pcDriverName[64] = ""; 
	for(int i = 0; i < 64; i++) 
	{ 
		sprintf_s(pcDriverName, "\\\\.\\ezusb-%d",i);

		m_hUsb = CreateFileA(pcDriverName,GENERIC_WRITE|GENERIC_READ,	
			FILE_SHARE_WRITE|FILE_SHARE_READ ,NULL,OPEN_EXISTING,0, NULL) ;

		if (m_hUsb != INVALID_HANDLE_VALUE)		
		{
			return 0 ;
		}
	}
	return -1;
}

int CCyPressUsb_Old::CloseUsb()
{
	if(CloseHandle(m_hUsb))
	{
		m_hUsb=INVALID_HANDLE_VALUE;
		return 0;
	}
	return -1;
}

int CCyPressUsb_Old::ReadData( char* pbuff,LONG &lBytes )
{
	DWORD   dwBytesReturned=0;
	BULK_TRANSFER_CONTROL InData  ;
	InData.pipeNum = 1 ;
	DWORD dwInSize =sizeof(BULK_TRANSFER_CONTROL);
	BOOL bRet = DeviceIoControl(m_hUsb, IOCTL_EZUSB_BULK_READ, &InData, dwInSize,(LPVOID)pbuff, (DWORD)lBytes,&dwBytesReturned , NULL);
	if(!bRet)
	{
		lBytes=0;
		return -1;
	}
	lBytes=dwBytesReturned;
	return 0;
}

int CCyPressUsb_Old::WriteData( char* pbuff,LONG &lBytes )
{
	return 0;
}

int CCyPressUsb_Old::SendOrder( PUSB_ORDER pOrder )
{
	VENDOR_OR_CLASS_REQUEST_CONTROL InData;
	InData.direction=pOrder->Direction;
	InData.requestType =pOrder->ReqType ;
	InData.recepient = pOrder->Target ;
	InData.request = pOrder->ReqCode;
	InData.value = pOrder->Value;
	InData.index = pOrder->Index;
	DWORD dwInSize=sizeof(VENDOR_OR_CLASS_REQUEST_CONTROL);
	DWORD dwBytesReturned=0;
	BOOL bRet = DeviceIoControl(m_hUsb, IOCTL_EZUSB_VENDOR_OR_CLASS_REQUEST, &InData, dwInSize ,pOrder->pData,pOrder->DataBytes ,&dwBytesReturned , NULL);
	if(!bRet)
		return -1;
	return 0;
}
