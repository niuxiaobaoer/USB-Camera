#include "StdAfx.h"
#include "DataCapture.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
HANDLE g_hMutexDataCapture;
extern long g_lBytePerSecond;
CDataCapture::CDataCapture(void)
{
	m_pDataProcess=NULL;
	m_iCount=0;		
	m_iRowIndex=0;		
	m_bFindDbFive=FALSE;	
	m_pInData=NULL;		
	m_pOutData=NULL;	
	g_hMutexDataCapture=CreateMutex(NULL,FALSE,NULL);
	
}

CDataCapture::~CDataCapture(void)
{
	//Close();
}
int CDataCapture::Open(CDataProcess *pProcess,int height,int width)
{
	m_pDataProcess=pProcess;
	g_height=height;
	g_width=width;
	g_width_L=g_width;
	ReadDataBytes=g_height*g_width+512;

	m_pReadBuff=new char[ReadDataBytes];//此处应对m_pReadBuff是否分配内存成功，即是否为NULL
	m_pOutData=new byte[(g_height*g_width_L+1)];//+1 added by qbc
	m_pInData=new byte[ReadDataBytes*4];

	memset(m_pReadBuff, 0, ReadDataBytes  * sizeof(byte));
	memset(m_pOutData, 0, (g_height*g_width_L + 1)  * sizeof(byte));
	memset(m_pInData,0,(ReadDataBytes*4)  * sizeof(byte));//(ReadDataBytes+g_width_L+3)*sizeof(byte)等同于sizeof（m_pInData）
	m_bCapture=TRUE;
	m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadProcess,this,0,NULL);
	return 0;
}
int CDataCapture::Close()
{
	m_bCapture=FALSE;

	WaitForSingleObject(g_hMutexDataCapture, INFINITE);

	Sleep(100);
	if(m_pOutData!=NULL)
	{
		delete[] m_pOutData;
		m_pOutData=NULL;
	}
	if(m_pInData!=NULL)
	{
		delete[] m_pInData;
		m_pInData=NULL;
	}
	delete[] m_pReadBuff;

	ReleaseMutex(g_hMutexDataCapture);

	return 0;
}
int CDataCapture::ThreadProcessFunction()
{
	long lRet=0;
	WaitForSingleObject(g_hMutexDataCapture, INFINITE);
	while(m_bCapture)
	{
		lRet=ReadDataBytes;
		ReadData(m_pReadBuff,lRet);
		if(lRet>0)
		{
			if(!m_bCapture) break;
			g_lBytePerSecond+=lRet;
			Input(m_pReadBuff,lRet);//测试，暂时注掉
		}
	}
	ReleaseMutex(g_hMutexDataCapture);
	return 0;
}
unsigned int __stdcall CDataCapture::ThreadProcess( void* handle )
{
	CDataCapture* pThis=(CDataCapture*)handle;
	pThis->ThreadProcessFunction();
	return 0;
}
int CDataCapture::Input( const LPVOID lpData,const DWORD dwSize )
{
	int iBytes=0;
	iBytes=dwSize+m_iCount;//m_iCount上一次拷贝剩余数据
	bool b_header=false,b_imu=false;
	memcpy(m_pInData+m_iCount,lpData,dwSize);
	int datalen=g_width*g_height;
	for(int i=0;i<iBytes;++i)
	{
		if ((i + datalen) >= iBytes)
		{
			m_iCount = iBytes - i;
			memcpy(m_pInData, m_pInData + i, m_iCount);
			return 0;
		}

		if(m_pInData[i]==0x33&&m_pInData[i+1]==0xcc&&m_pInData[i+14]==0x22&&m_pInData[i+15]==0xdd&&b_header==FALSE)
		{
			i=i+16;
			memcpy(m_pOutData,m_pInData+i,datalen);
			m_pDataProcess->Input(m_pOutData,datalen);
		}
		
#ifdef _USE55AA
		if(!m_bFindDbFive)
		{
			if(m_pInData[i]==0x55)
			{
				m_bFindDbFive=TRUE;
			}
			continue;
		}
		if(0xaa==m_pInData[i])
		{
			if((i+g_width_L+2)>=iBytes)//如果剩下的最后几个数据长度小于video_width*2+2行号个，不足以构成完整一行，拷贝到下一缓存
			{
				m_iCount=iBytes-i;
				memcpy(m_pInData,m_pInData+i,m_iCount);
				return 0;
			}
			m_iRowIndex=m_pInData[i+1];		//行号高8位
			m_iRowIndex<<=8;				 
			m_iRowIndex+=m_pInData[i+2];	//行号低8位
			if(m_iRowIndex>g_height+2)
			{
				//AfxMessageBox(L"行号出错");
				return 0;
				//exit(1);
			}
			memcpy(m_pOutData+m_iRowIndex*g_width_L,m_pInData+i+3,g_width_L);

			if(m_iRowIndex>=(g_height-1))
			{
				m_pDataProcess->Input(m_pOutData,g_height*g_width_L);

			}
			i=i+g_width_L+2;
		}
		m_bFindDbFive=FALSE;//找到0x55后，无论下个数据是不是0xaa都置状态位为FALSE,然后重新找0x55
#endif // _USE55AA
	}
	return 0;
}
