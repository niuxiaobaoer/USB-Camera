// CyUsb.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include "CyUsb.h"
#include "CyPressUsb.h"
#include "CyPressUsb_New.h"
#include "CyPressUsb_Old.h"


CCyPressUsb* g_pCyUsb=NULL;

CYUSB_API int CyUsb_Init( CYUSB_DRIVER driver/*=NEW_TYPE*/ )
{
	CyUsb_Destroy();
	if(OLD_DRIVER==driver)
	{
		g_pCyUsb=new CCyPressUsb_Old();
	}
	else
	{
		g_pCyUsb=new CCyPressUsb_New();
	}
	return 0;
}

CYUSB_API int CyUsb_Destroy()
{
	if(g_pCyUsb!=NULL)
	{
		g_pCyUsb->CloseUsb();
		delete g_pCyUsb;
		g_pCyUsb=NULL;
	}
	return 0;
}

CYUSB_API int OpenUsb()
{
	return g_pCyUsb==NULL?-1:g_pCyUsb->OpenUsb();
}

CYUSB_API int CloseUsb()
{
	return g_pCyUsb==NULL?-1:g_pCyUsb->CloseUsb();
}

CYUSB_API int ReadData( char* pbuff,LONG &lBytes )
{
	return g_pCyUsb==NULL?-1:g_pCyUsb->ReadData(pbuff,lBytes);
}

CYUSB_API int WriteData( char* pbuff,LONG &lBytes )
{
	return g_pCyUsb==NULL?-1:g_pCyUsb->WriteData(pbuff,lBytes);
}

CYUSB_API int SendOrder(PUSB_ORDER pOrder )
{
	return g_pCyUsb==NULL?-1:g_pCyUsb->SendOrder(pOrder);
}



