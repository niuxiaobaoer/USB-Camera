// CCTAPI.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "CCTAPI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//added by qbc
long g_lBytePerSecond;
//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CCCTAPIApp

//BEGIN_MESSAGE_MAP(CCCTAPIApp, CWinApp)
//END_MESSAGE_MAP()


// CCCTAPIApp construction

CCCTAPIApp::CCCTAPIApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	//m_bUsbOpen=FALSE;
	m_pDataProcess=NULL;
	m_pDataCapture=NULL;
	m_bOpened=false;
	m_bClosed=true;

	//added by qbc
	g_lBytePerSecond=0;
}


int  CCCTAPIApp::startCap(int height,int width,LPMV_CALLBACK2 CallBackFunc)
{
	if(m_bOpened)
		return -1;
	m_bOpened=true;//全局变量
	m_bClosed=false;//全局变量
	int m_bUsbOpen=0;
	m_bUsbOpen=OpenUsb()==0?TRUE:FALSE;
	if(m_bUsbOpen==0)
		return -1;
	m_pDataProcess=new CDataProcess();
	m_pDataCapture=new CDataCapture();
	m_pDataProcess->Open(height,width,CallBackFunc);
	m_pDataCapture->Open(m_pDataProcess,height,width);
	return 0;
}
int  CCCTAPIApp::startCap2(int height,int width,LPMV_CALLBACK2 CallBackFunc)
{
	if(m_bOpened)
		return -1;
	//WrFpgaReg(0x09,0x00);//停止采集
	WrFpgaReg(0x09,0x01);//开启数据传输
	Sleep(1);

	m_bOpened=true;//全局变量
	m_bClosed=false;//全局变量

	m_pDataProcess=new CDataProcess();
	m_pDataCapture=new CDataCapture();

	m_pDataProcess->Open(height,width,CallBackFunc);
	m_pDataCapture->Open(m_pDataProcess,height,width);
	return 0;
}
int  CCCTAPIApp::stopCap()
{
	if(m_bClosed)
		return -1;
	m_bOpened=false;
	m_bClosed=true;
	m_pDataCapture->Close();
	m_pDataProcess->Close();

	CloseUsb();

	delete m_pDataCapture;
	delete m_pDataProcess;

	return 0;
}
int  CCCTAPIApp::stopCap2()
{
	WrFpgaReg(0x09,0x00);//停止采集

	if(m_bClosed)
		return -1;

	m_bOpened=false;
	m_bClosed=true;
	
	m_pDataProcess->Close();
	m_pDataCapture->Close();

	
	delete m_pDataCapture;
	delete m_pDataProcess;

	return 0;
}
void  CCCTAPIApp::WrSensorReg(unsigned short iAddr, unsigned short iValue)
{
	USB_ORDER     m_sUsbOrder;
	BYTE  m_byData[64];
	m_sUsbOrder.pData=m_byData;

	m_sUsbOrder.ReqCode = 0xF1;
	m_sUsbOrder.DataBytes = 2;
	m_sUsbOrder.Direction = 0;
	m_sUsbOrder.Index = iAddr;
	m_sUsbOrder.Value = iValue;
	SendOrder(&m_sUsbOrder);
	return;

}
unsigned short  CCCTAPIApp::RdSensorReg(unsigned short iAddr)
{
	USB_ORDER     m_sUsbOrder;
	BYTE  m_byData[64];
	m_sUsbOrder.pData=m_byData;

	m_byData[0] = '0';
	m_byData[1] = '0';
	m_sUsbOrder.ReqCode = 0xF2;
	m_sUsbOrder.DataBytes = 2;
	m_sUsbOrder.Direction = 1;
	m_sUsbOrder.Index = iAddr;
	SendOrder(&m_sUsbOrder);
	UINT8 rxval[2];
	memcpy(rxval, m_byData, 2);
	unsigned short irxval = rxval[0] << 8;
	irxval += rxval[1];

	return irxval;

}
void  CCCTAPIApp::WrFpgaReg(unsigned char iAddr, unsigned char iValue)
{
	USB_ORDER     m_sUsbOrder;
	BYTE  m_byData[64];
	m_sUsbOrder.pData=m_byData;


	m_sUsbOrder.ReqCode = 0xF3; 
	m_sUsbOrder.DataBytes = 1;
	m_sUsbOrder.Direction = 0;
	m_sUsbOrder.Index = iAddr;
	m_sUsbOrder.Value = iValue;
	SendOrder(&m_sUsbOrder);
	return;

}
unsigned char  CCCTAPIApp::RdFpgaReg(unsigned char iAddr)
{
	USB_ORDER     m_sUsbOrder;
	BYTE  m_byData[64];
	m_sUsbOrder.pData=m_byData;

	m_byData[0] = '0';
	m_sUsbOrder.ReqCode = 0xF4;
	m_sUsbOrder.DataBytes = 1;
	m_sUsbOrder.Direction = 1;
	m_sUsbOrder.Index = iAddr;
	SendOrder(&m_sUsbOrder);
	UINT8 rxval[1];
	memcpy(rxval, m_byData, 1);
	UINT8 irxval = rxval[0];
	return irxval;
}
void  CCCTAPIApp::InitSensor(void)
{
	USB_ORDER m_sUsbOrder;
	BYTE  m_byData[64];
	m_sUsbOrder.pData=m_byData;

	m_sUsbOrder.ReqCode = 0xF0;
	m_sUsbOrder.DataBytes = 2;
	m_sUsbOrder.Direction = 0;

	SendOrder(&m_sUsbOrder);
	return;

}
void  CCCTAPIApp::GetFpsMbs(int &fCount,long &lBytePerSecond )
{
	if(m_pDataProcess!=NULL)
	{
		m_pDataProcess->GetFrameCount(fCount);
		lBytePerSecond=g_lBytePerSecond;
		//str.Format(L"%d Fps     %0.4f MBs",FCount,float(g_lBytePerSecond)/1024.0/1024.0);
		g_lBytePerSecond=0;
	}

}

void  CCCTAPIApp::SetTrigModAuto(void )
{
		WrSensorReg(0x30B0, 0x0080);
		WrSensorReg(0x301A, 0x10DC);
		WrFpgaReg(0x00, 0x00);
}
void  CCCTAPIApp::SetTrigModFpga(unsigned char fpgafreq)
{
		WrSensorReg(0x30B0, 0x0480);
		WrSensorReg(0x301A, 0x19D8);
		WrFpgaReg(0x00, 0x01);
		WrFpgaReg(0x05, fpgafreq);
}
void  CCCTAPIApp::setFpgaFreq(unsigned char fpgafreq)
{
	WrFpgaReg(0x05, fpgafreq & 0xff);
}
void  CCCTAPIApp::setGainValue(unsigned short GainValue)
{
	WrSensorReg(0x305E, GainValue);
}

void  CCCTAPIApp::setAutoGainExpo(bool isAutoGain, bool isAutoExpo)
{
	if((isAutoGain==true)&&(isAutoExpo==true))
		WrSensorReg(0x3100, 0x0003);
	else if ((isAutoGain==true)&&(isAutoExpo==false))
		WrSensorReg(0x3100, 0x0002);
	else if ((isAutoGain==false)&&(isAutoExpo==true))
		WrSensorReg(0x3100, 0x0001);
	else if ((isAutoGain==false)&&(isAutoExpo==false))
		WrSensorReg(0x3100, 0x0000);
	else;
}
void   CCCTAPIApp::setExpoValue(unsigned short ExpoValue)
{
	WrSensorReg(0x3012, ExpoValue);
}
void   CCCTAPIApp::setResolution(unsigned char resNo)
{
	if(resNo==0)//1280*720
	{
		WrSensorReg(0x3030, 0x0020);
		WrSensorReg(0x3002, 0x007C);
		WrSensorReg(0x3004, 0x0002);
		WrSensorReg(0x3006, 0x034B);
		WrSensorReg(0x3008, 0x0501);
		WrSensorReg(0x300A, 0x02FD);
		WrSensorReg(0x300C, 0x056C);
		WrSensorReg(0x30A6, 0x0001);
		WrSensorReg(0x3032, 0x0000);// 关闭bining模式

		WrFpgaReg(0x01, 0x05);
		WrFpgaReg(0x02, 0x00);
		WrFpgaReg(0x03, 0x02);
		WrFpgaReg(0x04, 0xd0);
		WrFpgaReg(0x06, 0x00);
	}
	else if(resNo==1)//1280*960
	{
		WrSensorReg(0x3030, 0x0020);
		WrSensorReg(0x3002, 0x0004);
		WrSensorReg(0x3004, 0x0002);
		WrSensorReg(0x3006, 0x03C3);
		WrSensorReg(0x3008, 0x0501);
		WrSensorReg(0x300A, 0x03FD);
		WrSensorReg(0x300C, 0x056C);
		WrSensorReg(0x30A6, 0x0001);
		WrSensorReg(0x3032, 0x0000);// 关闭bining模式

		WrFpgaReg(0x01, 0x05);
		WrFpgaReg(0x02, 0x00);
		WrFpgaReg(0x03, 0x03);
		WrFpgaReg(0x04, 0xC0);
		WrFpgaReg(0x06, 0x00);
	}
	else if(resNo==2)//640*480 skip
	{
		WrSensorReg(0x3030, 0x002A);
		WrSensorReg(0x3002, 0x0004);
		WrSensorReg(0x3004, 0x0002);
		WrSensorReg(0x3006, 0x03C3);
		WrSensorReg(0x3008, 0x0501);
		WrSensorReg(0x300A, 0x01FB);
		WrSensorReg(0x300C, 0x056C);
		WrSensorReg(0x30A6, 0x0003);
		WrSensorReg(0x3032, 0x0000);// 关闭bining模式

		WrFpgaReg(0x01, 0x02);
		WrFpgaReg(0x02, 0x80);
		WrFpgaReg(0x03, 0x01);
		WrFpgaReg(0x04, 0xE0);
		WrFpgaReg(0x06, 0x01);
	}
	else if(resNo==3)//640*480 binning
	{
		WrSensorReg(0x3030, 0x0020);
		WrSensorReg(0x3002, 0x0004);
		WrSensorReg(0x3004, 0x0002);
		WrSensorReg(0x3006, 0x03C3);
		WrSensorReg(0x3008, 0x0501);
		WrSensorReg(0x300A, 0x03FB);
		WrSensorReg(0x300C, 0x056C);
		WrSensorReg(0x30A6, 0x0001);
		WrSensorReg(0x3032, 0x0002);// 开启binning模式

		WrFpgaReg(0x01, 0x02);
		WrFpgaReg(0x02, 0x80);
		WrFpgaReg(0x03, 0x01);
		WrFpgaReg(0x04, 0xE0);
		WrFpgaReg(0x06, 0x00);
	}
	else;

}
void  CCCTAPIApp::setNormal(void)
{
	WrSensorReg(0x3040, 0x0000);
}
void  CCCTAPIApp::setXmirror(void)
{
	WrSensorReg(0x3040, 0x4000);
}
void  CCCTAPIApp::setYmirror(void)
{
	WrSensorReg(0x3040, 0x8000);
}
void  CCCTAPIApp::setXYmirror(void)
{
	WrSensorReg(0x3040, 0xC000);
}
void  CCCTAPIApp::SendUsbSpeed2Fpga(unsigned char speedType)
{
	WrFpgaReg(0x8,speedType);
}
void  CCCTAPIApp::SetAnalogGain_AutoTrig(unsigned char gainType)
{
	if(gainType==1)
		WrSensorReg(0x30B0,0x0080);
	else if(gainType==2)
		WrSensorReg(0x30B0,0x0090);
	else if(gainType==3)
		WrSensorReg(0x30B0,0x00A0);
	else if(gainType==4)
		WrSensorReg(0x30B0,0x00B0);
	else;

}
void  CCCTAPIApp::SetAnalogGain_FpgaTrig(unsigned char gainType)
{
	if(gainType==1)
		WrSensorReg(0x30B0,0x0480);
	else if(gainType==2)
		WrSensorReg(0x30B0,0x0490);
	else if(gainType==3)
		WrSensorReg(0x30B0,0x04A0);
	else if(gainType==4)
		WrSensorReg(0x30B0,0x04B0);
	else;

}
void  CCCTAPIApp::SetBitDepth(unsigned char chDepthType)
{
	if(chDepthType==0)
		WrFpgaReg(0x7, 0x0);
	else if(chDepthType==1)
		WrFpgaReg(0x7, 0x1);
	else if(chDepthType==2)
		WrFpgaReg(0x7, 0x2);
	else;
}