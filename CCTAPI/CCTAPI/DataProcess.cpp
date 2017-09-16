#include "StdAfx.h"

#include "DataProcess.h"


#define MSG_CMD_DESTORY		0
#define MSG_CMD_SINGLE		1
#define MSG_CMD_RUN			2
#define MSG_CMD_STOP		3

#define MSG_CMD_CHANGDisplayDataBytes	7
#define MSG_CMD_CHANGEAMPLIFIER		8
#define MSG_CMD_GETAVGCOUNT  9
#define MSG_DATA_HANDALE	10

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif.

HANDLE g_hMutexDataProcess;
CDataProcess::CDataProcess(void)
{
	m_hThread=NULL;
	m_pPutMsg=NULL;
	m_pGetMsg=NULL;
	m_hDC=0;

	memset(&m_BitmapInfo,0,sizeof(BITMAPINFO));
	m_BitmapInfo.bmiHeader.biBitCount=24;
	m_BitmapInfo.bmiHeader.biClrImportant=0;
	m_BitmapInfo.bmiHeader.biClrUsed=0;
	m_BitmapInfo.bmiHeader.biCompression=0;

	m_BitmapInfo.bmiHeader.biPlanes=1;
	m_BitmapInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);

	m_BitmapInfo.bmiHeader.biXPelsPerMeter=0;
	m_BitmapInfo.bmiHeader.biYPelsPerMeter=0;

	m_bEnd=FALSE;

	m_ProcType=Normal_Proc;

	m_ChangeType=Normal_Change;
	m_bCreateBmp=FALSE;

	m_lFrameCount=0;

	g_hMutexDataProcess=CreateMutex(NULL,FALSE,NULL);
}

CDataProcess::~CDataProcess(void)
{
		delete []m_Out;
		delete []m_In;
}

int CDataProcess::Open(int height,int width,LPMV_CALLBACK2 CallBackFunc)
{
	g_height=height;
	g_width=width;
	g_width_L=width;
	h_callback=CallBackFunc;
	CGuard guard(m_Mutex);
	m_pPutMsg = new UD_MESSAGE();
	m_pPutMsg->pData = new char[g_height*g_width_L];
	m_pGetMsg = new UD_MESSAGE();
	m_pGetMsg->pData = new char[g_height*g_width_L];
	m_MsgQueue.Open(5, g_height*g_width_L);
	m_In = new byte[g_height*g_width_L];
	m_Out = new tagRGB[g_height*g_width];
	memset(m_In,0,sizeof(m_In));
	memset(m_Out,0,sizeof(m_Out));
	m_bEnd=FALSE;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadProcess,this,0,NULL);

	return 0;
}
int CDataProcess::Close()
{
	//CGuard guard(m_Mutex);
	m_bEnd=TRUE;

	WaitForSingleObject(g_hMutexDataProcess, INFINITE);
	ReleaseMutex(g_hMutexDataProcess);
	return 0;
}

int CDataProcess::Input( VOID * pData,int dwSizes )
{
	CGuard guard(m_Mutex);
	if(m_bEnd)
		return -1;
	m_pPutMsg->iDataSize = dwSizes;
	memcpy(m_pPutMsg->pData,pData,m_pPutMsg->iDataSize);
	m_pPutMsg->iType = MSG_DATA_HANDALE;
	++m_lFrameCount;
	//return 0;
	return PutMessage(m_pPutMsg);//暂时注掉
}

void CDataProcess::ThreadProcessFunction()
{
	int iRet=0;
	int endflag = 1;
	WaitForSingleObject(g_hMutexDataProcess, INFINITE);
	while (endflag!=0)
	{
		if(m_bEnd)
		{
			CloseMsgQueue();
			break;
		}
		if (m_MsgQueue.GetMessage(m_pGetMsg)!=0)
		{
			Sleep(1);
			continue;
		}
		switch (m_pGetMsg->iType)
		{
			case MSG_CMD_DESTORY:
				m_MsgQueue.Reset();
				endflag=0;
				break;
			case MSG_DATA_HANDALE:
				memcpy(m_In,m_pGetMsg->pData,m_pGetMsg->iDataSize);
				ProcessData();
				memset(m_In,0,sizeof(m_In));
				memset(m_Out,0,sizeof(m_Out));
				break;
			default:
				break;
		}
	}
	ReleaseMutex(g_hMutexDataProcess);
}

unsigned int __stdcall CDataProcess::ThreadProcess( void *handle )
{
	CDataProcess *pThis = (CDataProcess *)handle;
	pThis->ThreadProcessFunction();
	return NULL;
}
int CDataProcess::OutPutWrapper(LPMV_CALLBACK2 CallBackFunc,LPVOID lpUser)
{
	CallBackFunc(m_In,this);
	return 0;
}



int CDataProcess::ProcessData()
{
	if(!m_bEnd)
	{

		OutPutWrapper(h_callback,this);
		memset(m_In,0,sizeof(m_In));
		memset(m_Out,0,sizeof(m_Out));
	}
	return 0;
}

int CDataProcess::PutMessage( UD_MESSAGE *msg )
{
	int iRet=0;
	while(1)
	{
		if(m_bEnd)
			break;
		iRet = m_MsgQueue.PutMessage(msg);
		if (iRet==0)
			break;
		else if (iRet==-1)
		{
			continue;
		} 
		else
		{
			iRet=-1;
			break;
		}
	}	
	return iRet;
}

int CDataProcess::ByteToRGB( byte *pIn ,tagRGB* pOut )
{
	static tagRGB sTempRgb;
#ifdef RGB565
	static tagRGB s565Rgb;
	byte maskGH=0x7;//0000,0111,low 3 bits
	byte maskGL=0xE0;//1110,0000,high 3 bits
	byte maskB=0x1F;//0001,1111,
	int x6=255/63;//4
	int x5=255/31;//8
	for(int i=0;i<g_height;i++)
	{
		for(int j=0;j<g_width_L;j+=2)//16bit data length 
		{
			s565Rgb.R=pIn[i * g_width_L + j]>>3;//R5
			s565Rgb.G=((pIn[i * g_width_L + j]&0x7)<<3)+((pIn[i * g_width_L + j+1]&0xE0)>>5);//G6
			s565Rgb.B=pIn[i * g_width_L + j+1]&0x1F;//B5
			
			sTempRgb.R=s565Rgb.R*x5;
			sTempRgb.G=s565Rgb.G*x6;
			sTempRgb.B=s565Rgb.B*x5;
			
			RgbChangeProc(pOut[i * g_width_L + j/2],sTempRgb);
		}
	}
#else
	for(int i=0;i<g_height;i+=2)
	{
		for(int j=0;j<g_width;j+=2)
		{
			sTempRgb.R= pIn[i * g_width + j];
			sTempRgb.G= (pIn[i * g_width + j+1]>>1)+(pIn[(i+1) * g_width + j]>>1);
			sTempRgb.B= pIn[(i+1) * g_width + j+1];
			RgbChangeProc(pOut[i * g_width + j],sTempRgb);
		}
	}
	for(int i=0;i<g_height;i+=2)
	{
		for(int j=1;j<g_width-1;j+=2)
		{
			sTempRgb.R= pIn[i * g_width + j+1];
			sTempRgb.G= (pIn[i * g_width + j]>>1)+(pIn[(i+1) * g_width + j+1]>>1);
			sTempRgb.B= pIn[(i+1) * g_width + j];
			RgbChangeProc(pOut[i * g_width + j],sTempRgb);
		}
	}
	for(int i=1;i<g_height-1;i+=2)
	{
		for(int j=0;j<g_width;j+=2)
		{
			sTempRgb.R= pIn[(i+1) * g_width + j];
			sTempRgb.G= (pIn[i * g_width + j]>>1)+(pIn[(i+1) * g_width + j+1]>>1);
			sTempRgb.B= pIn[i * g_width + j+1];
			RgbChangeProc(pOut[i * g_width + j],sTempRgb);
		}
	}
	for(int i=1;i<g_height-1;i+=2)
	{
		for(int j=1;j<g_width-1;j+=2)
		{
			sTempRgb.R= pIn[(i+1) * g_width + j+1];
			sTempRgb.G= (pIn[i * g_width + j+1]>>1)+(pIn[(i+1) * g_width + j]>>1);
			sTempRgb.B= pIn[i * g_width + j];
			RgbChangeProc(pOut[i * g_width + j],sTempRgb);
		}
	}
#endif
	return 0;
}

void CDataProcess::CloseMsgQueue()
{
	if(m_pGetMsg!=NULL)
	{
		if(m_pGetMsg->pData!=NULL)
		{
			delete[] m_pGetMsg->pData;
			m_pGetMsg->pData=NULL;
		}
		delete m_pGetMsg;
		m_pGetMsg=NULL;
	}
	if(m_pPutMsg!=NULL)
	{
		if(m_pPutMsg->pData!=NULL)
		{
			delete[] m_pPutMsg->pData;
			m_pPutMsg->pData=NULL;
		}
		delete m_pPutMsg;
		m_pPutMsg=NULL;
	}
	m_MsgQueue.Close();
}

int CDataProcess::GetFrameCount( int& fCount )
{
	//CGuard guard(m_Mutex);
	fCount=m_lFrameCount;
	m_lFrameCount=0;
	return 0;
}

void CDataProcess::DoYmirrorProc()
{
	tagRGB TempRgb;
	for(int i=0;i<g_height;++i)
	{
		for(int j=0;j<g_width/2;++j )
		{
			memcpy(&TempRgb,&m_Out[i*g_width + j],sizeof(tagRGB));
			memcpy(&m_Out[i*g_width + j],&m_Out[i*g_width + g_width-1-j],sizeof(tagRGB));
			memcpy(&m_Out[i*g_width + g_width-1-j],&TempRgb,sizeof(tagRGB));
		}
	}
}

void CDataProcess::DoXmirrorProc()
{
	tagRGB* TempRgbBuf = new tagRGB[g_width];//如果用到，考虑指针地址循环使用
	for(int i=0;i<g_height/2;++i)
	{
		memcpy(TempRgbBuf,&m_Out[i*g_width],sizeof(tagRGB)*g_width);
		memcpy(&m_Out[i*g_width],&m_Out[(g_height-1-i)*g_width],sizeof(tagRGB)*g_width);
		memcpy(&m_Out[(g_height-1-i)*g_width],TempRgbBuf,sizeof(tagRGB)*g_width);
		memset(TempRgbBuf,0,sizeof(TempRgbBuf));
	}
	delete []TempRgbBuf;
}

int CDataProcess::SetProcType( DataProcessType type )
{
	CGuard guard(m_Mutex);
	m_ProcType=type;
	return 0;
}

int CDataProcess::SetChangeType( RgbChangeType type )
{
	CGuard guard(m_Mutex);
	m_ChangeType=type;
	return 0;
}

void CDataProcess::RgbChangeProc( tagRGB& DestRgb,const tagRGB& OrgRgb )
{
	switch(m_ChangeType)
	{
	case RB_Change:
		DestRgb.R=OrgRgb.B;
		DestRgb.G=OrgRgb.G;
		DestRgb.B=OrgRgb.R;
		break;
	case RG_Change:
		DestRgb.R=OrgRgb.G;
		DestRgb.G=OrgRgb.R;
		DestRgb.B=OrgRgb.B;
		break;
	case GB_Change:
		DestRgb.R=OrgRgb.R;
		DestRgb.G=OrgRgb.B;
		DestRgb.B=OrgRgb.G;
		break;
	case Normal_Change:
	default:
		DestRgb.R=OrgRgb.R;
		DestRgb.G=OrgRgb.G;
		DestRgb.B=OrgRgb.B;
		break;
	}
}