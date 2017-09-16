#pragma once
#include "CyPressUsb.h"

class CCyPressUsb_Old:public CCyPressUsb
{
public:
	CCyPressUsb_Old(void);
	~CCyPressUsb_Old(void);

public:
	int OpenUsb();						    
	int CloseUsb();							
	int ReadData(char* pbuff,LONG &lBytes);
	int WriteData(char* pbuff,LONG &lBytes);
	int	SendOrder(PUSB_ORDER pOrder);

private:
	HANDLE m_hUsb;
};

