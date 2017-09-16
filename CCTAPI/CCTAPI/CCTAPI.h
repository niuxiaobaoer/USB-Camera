// CCTAPI.h : main header file for the CCTAPI DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#ifdef COMPILE_API
	#define CCT_API __declspec(dllexport) 
#else 
	#define CCT_API __declspec(dllimport)
#endif

#include "resource.h"		// main symbols
#include "DataCapture.h"
#include "CyUsb.h"

class  CCT_API CCCTAPIApp :public CWinApp// //public ICCTAPI,
{
public:
	CCCTAPIApp();
	int startCap(int height,int width,LPMV_CALLBACK2 CallBackFunc);
	int startCap2(int height,int width,LPMV_CALLBACK2 CallBackFunc);
	int stopCap();
	int stopCap2();

	void WrSensorReg(unsigned short, unsigned short);
	unsigned short RdSensorReg(unsigned short);
	void WrFpgaReg(unsigned char, unsigned char);
	unsigned char RdFpgaReg(unsigned char);
	void InitSensor(void);
	void GetFpsMbs(int &FCount,long &lBytePerSecond );
	void SetTrigModAuto(void );
	void SetTrigModFpga(unsigned char fpgafreq);
	void setFpgaFreq(unsigned char fpgafreq);
	void setGainValue(unsigned short GainValue);
	void setAutoGainExpo(bool isAutoGain, bool isAutoExpo);
	void setExpoValue(unsigned short ExpoValue);
	void setResolution(unsigned char resNo);
	void setNormal(void);
	void setXmirror(void);
	void setYmirror(void);
	void setXYmirror(void);
	void SendUsbSpeed2Fpga(unsigned char speedType);
	void SetAnalogGain_AutoTrig(unsigned char gainType);
	void SetAnalogGain_FpgaTrig(unsigned char gainType);
	void SetBitDepth(unsigned char chDepthType);
	void destory();
// Overrides


private:
	char*         m_pReadBuff;
	//long          m_lBytePerSecond;
	bool m_bUsbOpen;
	//added by qbc

	CDataProcess *m_pDataProcess;
	CDataCapture *m_pDataCapture;
	bool m_bOpened;
	bool m_bClosed;

	//BYTE          m_byData[64];
	//USB_ORDER     m_sUsbOrder;



	
	
};

