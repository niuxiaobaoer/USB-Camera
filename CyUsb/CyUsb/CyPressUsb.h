#pragma once
#include "CyUsb.h"

#include "CyAPI.h"
#pragma comment(lib,"CyAPI.lib")

#include "EZUSBSYS.H"

class CCyPressUsb
{
public:
	CCyPressUsb(void);
	virtual ~CCyPressUsb(void);

public:
	virtual int OpenUsb();						    
	virtual int CloseUsb();							
	virtual int ReadData(char* pbuff,LONG &lBytes);
	virtual int WriteData(char* pbuff,LONG &lBytes);
	virtual int	SendOrder(PUSB_ORDER pOrder);
};

