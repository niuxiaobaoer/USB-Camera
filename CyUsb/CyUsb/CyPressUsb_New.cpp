#include "stdafx.h"
#include "CyPressUsb_New.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CCyPressUsb_New::CCyPressUsb_New(void)
{

}

CCyPressUsb_New::~CCyPressUsb_New(void)
{
	CloseUsb();
}

int CCyPressUsb_New::OpenUsb()
{
	CloseUsb();
	for(int i=0;i<m_Usb.DeviceCount();++i)
	{
		m_Usb.Open(i);
		if(m_Usb.IsOpen())
		{
			break;
		}
	}
	if(m_Usb.IsOpen())
	{
		m_Usb.ControlEndPt->Target = TGT_DEVICE;
		m_Usb.ControlEndPt->ReqType = REQ_VENDOR;
		m_Usb.ControlEndPt->Value = 0;
		m_Usb.ControlEndPt->Index = 0;
		m_Usb.BulkInEndPt->TimeOut = 100;//added byq qbc
		
		return 0;
	}
	return -1;
}

int CCyPressUsb_New::CloseUsb()
{
	if(m_Usb.IsOpen())
	{
		m_Usb.Close();
		return 0;
	}
	return -1;
}

int CCyPressUsb_New::ReadData( char* pbuff,LONG &lBytes )
{
	
	if(m_Usb.IsOpen())
	{
		if(m_Usb.BulkInEndPt->GetXferSize()<lBytes)
		{
			m_Usb.BulkInEndPt->SetXferSize(lBytes);
		}
		if(m_Usb.BulkInEndPt->XferData((PUCHAR)pbuff,lBytes))
		{
			return 0;
		}
	}
	return -1;
}

int CCyPressUsb_New::WriteData( char* pbuff,LONG &lBytes )
{
	return 0;
}

int CCyPressUsb_New::SendOrder( PUSB_ORDER pOrder )
{
	if(m_Usb.IsOpen())
	{
		m_Usb.ControlEndPt->Target=(CTL_XFER_TGT_TYPE)pOrder->Target;
		m_Usb.ControlEndPt->ReqType=(CTL_XFER_REQ_TYPE)pOrder->ReqType;
		m_Usb.ControlEndPt->Direction=(CTL_XFER_DIR_TYPE)pOrder->Direction;
		m_Usb.ControlEndPt->ReqCode=pOrder->ReqCode;
		m_Usb.ControlEndPt->Value=pOrder->Value;
		m_Usb.ControlEndPt->Index=pOrder->Index;
		LONG lBytes=0;
		lBytes=pOrder->DataBytes;
		if(m_Usb.ControlEndPt->XferData((PUCHAR)(pOrder->pData),lBytes))
		{
			pOrder->DataBytes=lBytes;
			return 0;
		}
		pOrder->DataBytes=0;
	}
	return -1;
}




