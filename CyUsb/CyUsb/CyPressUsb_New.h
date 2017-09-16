#pragma once

#include "CyPressUsb.h"

class CCyPressUsb_New:public CCyPressUsb
{
public:
	CCyPressUsb_New(void);
	virtual ~CCyPressUsb_New(void);

public:
	int OpenUsb();						    
	int CloseUsb();							
	int ReadData(char* pbuff,LONG &lBytes);
	int WriteData(char* pbuff,LONG &lBytes);
	int	SendOrder(PUSB_ORDER pOrder);

private:
	CCyUSBDevice m_Usb;
};

