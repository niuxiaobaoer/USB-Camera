
// UsbControlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "UsbControl.h"	
#include "UsbControlDlg.h"
#include "afxdialogex.h"
#include "Include/CyAPI.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
//added by qbc
#define RESOLUTION_1280_720  0x55
#define RESOLUTION_1280_960  0x56
#define RESOLUTION_640_480_SKIP  0x57
#define RESOLUTION_640_480_BINNING  0x58
unsigned char g_byteBitDepthNo = 1;//默认为8位深度
unsigned char g_byteResolutionType = RESOLUTION_1280_720;

int g_width=1280 * g_byteBitDepthNo;
int g_height=720;
bool g_bSaveVedio = false;
HWND m_hwnd;
cv::VideoWriter h_vw;// ("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, cv::Size(g_width, g_height), false);
volatile bool snap;
volatile bool save_all;

//#define g_iRoiWidth 10
//#define g_iRoiHeight 10

bool g_bRoi = false;
bool g_bRoiBox = false;
unsigned int g_iRoiWidth = 10;
unsigned int g_iRoiHeight = 10;

//BYTE g_byteRoiBuffer1[g_iRoiWidth * g_iRoiHeight];
//BYTE g_byteRoiBuffer2[g_iRoiWidth * g_iRoiHeight];
//BYTE g_byteRoiBuffer3[g_iRoiWidth * g_iRoiHeight];
//BYTE g_byteRoiBuffer4[g_iRoiWidth * g_iRoiHeight];
//BYTE g_byteRoiBuffer5[g_iRoiWidth * g_iRoiHeight];

BYTE* g_byteRoiBuffer1 = NULL;
BYTE* g_byteRoiBuffer2 = NULL;
BYTE* g_byteRoiBuffer3 = NULL;
BYTE* g_byteRoiBuffer4 = NULL;
BYTE* g_byteRoiBuffer5 = NULL;

bool g_bFillRoiBuf = false;

CPoint g_ptRoi1(0,0);
CPoint g_ptRoi2(0, 0);
CPoint g_ptRoi3(0, 0);
CPoint g_ptRoi4(0, 0);
CPoint g_ptRoi5(0, 0);

bool g_bRoiWidthOK = false;
bool g_bRoiHeightOK = false;


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
enum ReqValue
{
	TRIGMODE=0xD0,IMGDISP,EXPOGAIN,GAIN,EXPO,MIRROR,RCEXTR,TRIGPERIOD,RSTHW,SOFTTRIG,RSTSENSOR
};
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()

};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CUsbControlDlg 对话框




CUsbControlDlg::CUsbControlDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CUsbControlDlg::IDD, pParent)
	, m_iProcType(0)
	, m_sEdit_Width(_T(""))
	, m_sEdit_Height(_T(""))
{

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bUsbOpen=FALSE;

	m_lBytePerSecond=0;
	m_pBrush=NULL;
	snap=false;

	g_bRoi = false;
	g_bRoiBox = false;
	g_iRoiWidth = 0;
	g_iRoiHeight = 0;

	h_cctapi = new CCCTAPIApp();
}

CUsbControlDlg::~CUsbControlDlg()
{
}

void CUsbControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_NORMAL, m_iProcType);
	DDX_Control(pDX, IDC_EDITHwTrigFreq, eFpgaFreq);
	DDX_Control(pDX, IDC_EDITGainValue, eGainValue);
	DDX_Control(pDX, IDC_EDITExpoValue, eExpoValue);
	DDX_Control(pDX, IDC_CHECKAutoGain, cbAutoGain);
	DDX_Control(pDX, IDC_CHECKAutoExpo, cbAutoExpo);
	DDX_Control(pDX, IDC_CHECK_SAVEALL, m_chk_save_all);
}

BEGIN_MESSAGE_MAP(CUsbControlDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_VIDEOCAPTURE, &CUsbControlDlg::OnBnClickedBtnVideocapture)
	ON_BN_CLICKED(IDC_BTN_STOPCAPTURE, &CUsbControlDlg::OnBnClickedBtnStopcapture)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_SNAP, &CUsbControlDlg::OnBnClickedBnSnap)
	ON_EN_CHANGE(IDC_EDITGainValue, &CUsbControlDlg::setGainValue)
	ON_BN_CLICKED(IDC_CHECKAutoGain, &CUsbControlDlg::setAutoGainExpo)
	ON_BN_CLICKED(IDC_CHECKAutoExpo, &CUsbControlDlg::setAutoGainExpo)
	ON_EN_CHANGE(IDC_EDITExpoValue, &CUsbControlDlg::setExpoValue)
	ON_EN_CHANGE(IDC_EDITHwTrigFreq, &CUsbControlDlg::setFpgaFreq)
	///ON_BN_CLICKED(IDC_BUTTONSoftTrig, &CUsbControlDlg::OnBnClickedButtonsofttrig)
	ON_BN_CLICKED(IDC_CHECK_SAVEALL, &CUsbControlDlg::OnBnClickedCheckSaveall)
	ON_BN_CLICKED(IDC_BUTTON_WR_SENSOR, &CUsbControlDlg::OnBnClickedButtonWrSensor)
	ON_BN_CLICKED(IDC_BUTTON_RD_SENSOR, &CUsbControlDlg::OnBnClickedButtonRdSensor)
	ON_BN_CLICKED(IDC_BUTTON_WR_FPGA, &CUsbControlDlg::OnBnClickedButtonWrFpga)
	ON_BN_CLICKED(IDC_BUTTON_RD_FPGA, &CUsbControlDlg::OnBnClickedButtonRdFpga)
	ON_BN_CLICKED(IDC_BUTTON_INIT_SENSOR, &CUsbControlDlg::OnBnClickedButtonInitSensor)
	ON_BN_CLICKED(IDC_RADIO1, &CUsbControlDlg::OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, &CUsbControlDlg::OnBnClickedRadio2)
	ON_BN_CLICKED(IDC_RADIO3, &CUsbControlDlg::OnBnClickedRadio3)
	ON_BN_CLICKED(IDC_RADIO_NORMAL, &CUsbControlDlg::OnBnClickedRadioNormal)
	ON_BN_CLICKED(IDC_RADIO_XMIRROR, &CUsbControlDlg::OnBnClickedRadioXmirror)
	ON_BN_CLICKED(IDC_RADIO_YMIRROR, &CUsbControlDlg::OnBnClickedRadioYmirror)
	ON_BN_CLICKED(IDC_RADIO_XYMIRROR, &CUsbControlDlg::OnBnClickedRadioXymirror)
	ON_BN_CLICKED(IDC_BUTTON_OPEN_USB, &CUsbControlDlg::OnBnClickedButtonOpenUsb)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE_USB, &CUsbControlDlg::OnBnClickedButtonCloseUsb)
	ON_BN_CLICKED(IDC_CHECK_SAVE_VEDIO, &CUsbControlDlg::OnBnClickedCheckSaveVedio)
	ON_BN_CLICKED(IDC_RADIO_AUTO_TRIG, &CUsbControlDlg::OnBnClickedRadioAutoTrig)
	ON_BN_CLICKED(IDC_BUTTON_USB_SPEED_CKECK, &CUsbControlDlg::OnBnClickedButtonUsbSpeedCkeck)
	ON_BN_CLICKED(IDC_RADIO_FPGA_TRIG, &CUsbControlDlg::OnBnClickedRadioFpgaTrig)
	
	ON_BN_CLICKED(IDC_RADIO4, &CUsbControlDlg::OnBnClickedRadio4)
	ON_BN_CLICKED(IDC_RADIO_8BIT_DEPTH, &CUsbControlDlg::OnBnClickedRadio8bitDepth)
	ON_BN_CLICKED(IDC_RADIO_16BIT_DEPTH, &CUsbControlDlg::OnBnClickedRadio16bitDepth)
	ON_BN_CLICKED(IDC_RADIO_ANLOG_1X, &CUsbControlDlg::OnBnClickedRadioAnlog1x)
	ON_BN_CLICKED(IDC_RADIO_ANLOG_2X, &CUsbControlDlg::OnBnClickedRadioAnlog2x)
	ON_BN_CLICKED(IDC_RADIO_ANLOG_3X, &CUsbControlDlg::OnBnClickedRadioAnlog3x)
	ON_BN_CLICKED(IDC_RADIO_ANLOG_4X, &CUsbControlDlg::OnBnClickedRadioAnlog4x)

	ON_EN_CHANGE(IDC_EDIT_ROI_X1, &CUsbControlDlg::OnEnChangeEditRoiX1)
	ON_EN_CHANGE(IDC_EDIT_ROI_Y1, &CUsbControlDlg::OnEnChangeEditRoiY1)
	ON_EN_CHANGE(IDC_EDIT_ROI_X2, &CUsbControlDlg::OnEnChangeEditRoiX2)
	ON_EN_CHANGE(IDC_EDIT_ROI_Y2, &CUsbControlDlg::OnEnChangeEditRoiY2)
	ON_EN_CHANGE(IDC_EDIT_ROI_X3, &CUsbControlDlg::OnEnChangeEditRoiX3)
	ON_EN_CHANGE(IDC_EDIT_ROI_Y3, &CUsbControlDlg::OnEnChangeEditRoiY3)
	ON_EN_CHANGE(IDC_EDIT_ROI_X4, &CUsbControlDlg::OnEnChangeEditRoiX4)
	ON_EN_CHANGE(IDC_EDIT_ROI_Y4, &CUsbControlDlg::OnEnChangeEditRoiY4)
	ON_EN_CHANGE(IDC_EDIT_ROI_X5, &CUsbControlDlg::OnEnChangeEditRoiX5)
	ON_EN_CHANGE(IDC_EDIT_ROI_Y5, &CUsbControlDlg::OnEnChangeEditRoiY5)

	ON_BN_CLICKED(IDC_CHECK_ROI, &CUsbControlDlg::OnBnClickedCheckRoi)
	ON_BN_CLICKED(IDC_CHECK_ROI_BOX, &CUsbControlDlg::OnBnClickedCheckRoiBox)
	ON_EN_CHANGE(IDC_EDIT_ROI_WIDTH, &CUsbControlDlg::OnEnChangeEditRoiWidth)
	ON_EN_CHANGE(IDC_EDIT_ROI_HEIGHT, &CUsbControlDlg::OnEnChangeEditRoiHeight)
	ON_BN_CLICKED(IDC_RADIO5, &CUsbControlDlg::OnBnClickedRadio5)
END_MESSAGE_MAP()


BOOL CUsbControlDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN )
	{
		if(pMsg->wParam == VK_RETURN)// || pMsg->wParam == VK_ESCAPE)
		{
			return TRUE;                // Do not process further
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

// CUsbControlDlg 消息处理程序

BOOL CUsbControlDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//------------------------------------
	CRect cRect,wRect,mRect;
	GetDesktopWindow()->GetWindowRect(wRect);
	GetWindowRect(cRect);
	mRect.right=wRect.right-50;
	mRect.bottom=wRect.bottom-50;
	mRect.left=mRect.right-cRect.Width();
	mRect.top=mRect.bottom-cRect.Height();
	MoveWindow(mRect);
	//------------------------------------



	SetTimer(1,1000,NULL);

	m_pBrush=new CBrush[2];
	m_pBrush[0].CreateSolidBrush(RGB(99,208,242));
	m_pBrush[1].CreateSolidBrush(RGB(174,238,250));

	HINSTANCE dll_handle=::LoadLibraryA("CCTAPI.dll");
	if(!dll_handle)
	{
		std::cerr<<"unable to load dll\n";
		return TRUE;
	}
	//icct_factory fac_func=reinterpret_cast<icct_factory>(
	//	::GetProcAddress(dll_handle,"create_CCTAPI"));


	cbAutoExpo.SetCheck(1);
	cbAutoGain.SetCheck(1);


	SetDlgItemText(IDC_STATIC_TEXT, L" ");
	GetDlgItem(IDC_EDITHwTrigFreq)->EnableWindow(false);
	//GetDlgItem(IDC_EDITGainValue)->EnableWindow(false);
	//GetDlgItem(IDC_EDITExpoValue)->EnableWindow(false);

	CButton* pBtn = (CButton *)GetDlgItem(IDC_RADIO_AUTO_TRIG);
	pBtn->SetCheck(1);

	pBtn = (CButton *)GetDlgItem(IDC_RADIO1);
	pBtn->SetCheck(1);
	
	SetDlgItemText(IDC_EDITHwTrigFreq, L"25");

	SetDlgItemText(IDC_STATIC_TEXT2, L"");

	pBtn = (CButton *)GetDlgItem(IDC_RADIO_8BIT_DEPTH);
	pBtn->SetCheck(1);
	g_byteBitDepthNo = 1;//默认为8位深度



	SetDlgItemText(IDC_EDIT_ROI_X1,L"200");
	SetDlgItemText(IDC_EDIT_ROI_Y1, L"50");
	SetDlgItemText(IDC_EDIT_ROI_X2, L"400");
	SetDlgItemText(IDC_EDIT_ROI_Y2, L"50");
	SetDlgItemText(IDC_EDIT_ROI_X3, L"300");
	SetDlgItemText(IDC_EDIT_ROI_Y3, L"150");
	SetDlgItemText(IDC_EDIT_ROI_X4, L"200");
	SetDlgItemText(IDC_EDIT_ROI_Y4, L"200");
	SetDlgItemText(IDC_EDIT_ROI_X5, L"400");
	SetDlgItemText(IDC_EDIT_ROI_Y5, L"200");
	SetDlgItemText(IDC_EDIT_ROI_WIDTH, L"10");
	SetDlgItemText(IDC_EDIT_ROI_HEIGHT, L"10");


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE

	
}

void CUsbControlDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();//加载控制台窗口模型
		int temp1 = 1;
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CUsbControlDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CUsbControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void BMPHeader(int lWidth, int lHeight,byte* m_buf)
{
	int mlBpp=8;
	bool lReverse=true;
	BITMAPFILEHEADER bhh;
	BITMAPINFOHEADER bih;
	memset(&bhh,0,sizeof(BITMAPFILEHEADER));
	memset(&bih,0,sizeof(BITMAPINFOHEADER));

	int widthStep				=	(((lWidth * mlBpp) + 31) & (~31)) / 8; //每行实际占用的大小（每行都被填充到一个4字节边界）
	int QUADSize 				= 	mlBpp==8?sizeof(RGBQUAD)*256:0;

	//构造彩色图的文件头
	bhh.bfOffBits				=	(DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + QUADSize; 
	bhh.bfSize					=	(DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + QUADSize + widthStep*lHeight;  
	bhh.bfReserved1				=	0; 
	bhh.bfReserved2				=	0;
	bhh.bfType					=	0x4d42;  

	//构造彩色图的信息头
	bih.biBitCount				=	mlBpp;
	bih.biSize					=	sizeof(BITMAPINFOHEADER);
	bih.biHeight				=	(lReverse?-1:1)*lHeight;
	bih.biWidth					=	lWidth;  
	bih.biPlanes				=	1;
	bih.biCompression			=	BI_RGB;
	bih.biSizeImage				=	widthStep*lHeight;  
	bih.biXPelsPerMeter			=	0;  
	bih.biYPelsPerMeter			=	0;  
	bih.biClrUsed				=	0;  
	bih.biClrImportant			=	0;  
	
	//构造灰度图的调色版
	RGBQUAD rgbquad[256];
	if(mlBpp==8)
	{
		for(int i=0;i<256;i++)
		{
			rgbquad[i].rgbBlue=i;
			rgbquad[i].rgbGreen=i;
			rgbquad[i].rgbRed=i;
			rgbquad[i].rgbReserved=0;
		}
	}

	int DIBSize = widthStep * lHeight;
	//TRACE(_T("DIBSIze is %d"),DIBSize);

	bool b_save_file	=true;
	if(b_save_file)
	{
		CString strName;
		CString camFolder;
		camFolder.Format(L"d:\\c6UDP\\cam%d",0);
		if(CreateDirectory(camFolder,NULL)||ERROR_ALREADY_EXISTS == GetLastError())
		{
			int iFileIndex=1;
			do 
			{
				strName.Format(L"d:\\c6UDP\\cam%d\\V_%d.bmp",0,iFileIndex);
				++iFileIndex;
			} while (_waccess(strName,0)==0);
			CT2CA pszConvertedAnsiString (strName);
			std::string cvfilename(pszConvertedAnsiString);
			
			CFile file;  
			if(file.Open(strName,CFile::modeWrite | CFile::modeCreate))  
			{
				file.Write((LPSTR)&bhh,sizeof(BITMAPFILEHEADER));  
				file.Write((LPSTR)&bih,sizeof(BITMAPINFOHEADER));  
				if(mlBpp==8) file.Write(&rgbquad,sizeof(RGBQUAD)*256);
				file.Write(m_buf,DIBSize);  
				file.Close();  
				return ;  
			}  
		}
	}

	
}

//BYTE *pDataBuffer16 = NULL; 
void _stdcall RawCallBack(LPVOID lpParam, LPVOID lpUser)
{
	int i = 0, j = 0, k = 0;
	BYTE *pDataBuffer = (BYTE*)lpParam;
	//pDataBuffer16 = new BYTE[g_height*g_width * 2];
	//memset(pDataBuffer16,0, g_height*g_width * 2);
	//for (i = 0; i < g_height*g_width; i++)
	//{
	//		pDataBuffer16[i*2+1] = pDataBuffer[i] ;
	//}

	
	if ((g_bRoi == true)&&(g_bRoiBox == true))
	{
		for (i = 0; i < g_iRoiWidth*g_byteBitDepthNo; i++)
		{
			pDataBuffer[g_ptRoi1.y *g_width + g_ptRoi1.x*g_byteBitDepthNo + i] = 0;
			pDataBuffer[(g_ptRoi1.y + g_iRoiHeight)*g_width + g_ptRoi1.x*g_byteBitDepthNo + i] = 0;

			pDataBuffer[g_ptRoi2.y *g_width + g_ptRoi2.x*g_byteBitDepthNo + i] = 0;
			pDataBuffer[(g_ptRoi2.y + g_iRoiHeight)*g_width + g_ptRoi2.x*g_byteBitDepthNo + i] = 0;

			pDataBuffer[g_ptRoi3.y *g_width + g_ptRoi3.x*g_byteBitDepthNo + i] = 0;
			pDataBuffer[(g_ptRoi3.y + g_iRoiHeight)*g_width + g_ptRoi3.x*g_byteBitDepthNo + i] = 0;

			pDataBuffer[g_ptRoi4.y *g_width + g_ptRoi4.x*g_byteBitDepthNo + i] = 0;
			pDataBuffer[(g_ptRoi4.y + g_iRoiHeight)*g_width + g_ptRoi4.x*g_byteBitDepthNo + i] = 0;

			pDataBuffer[g_ptRoi5.y *g_width + g_ptRoi5.x*g_byteBitDepthNo + i] = 0;
			pDataBuffer[(g_ptRoi5.y + g_iRoiHeight)*g_width + g_ptRoi5.x*g_byteBitDepthNo + i] = 0;
		}

		for (i = 0; i < g_iRoiHeight; i++)
		{
			pDataBuffer[(g_ptRoi1.y + i) *g_width + g_ptRoi1.x * g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi1.y + i) *g_width + g_ptRoi1.x * g_byteBitDepthNo+1] = 0;
			pDataBuffer[(g_ptRoi1.y + i) *g_width + (g_ptRoi1.x + g_iRoiWidth)*g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi1.y + i) *g_width + (g_ptRoi1.x + g_iRoiWidth)*g_byteBitDepthNo+1] = 0;

			pDataBuffer[(g_ptRoi2.y + i) *g_width + g_ptRoi2.x * g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi2.y + i) *g_width + g_ptRoi2.x * g_byteBitDepthNo + 1] = 0;
			pDataBuffer[(g_ptRoi2.y + i) *g_width + (g_ptRoi2.x + g_iRoiWidth)*g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi2.y + i) *g_width + (g_ptRoi2.x + g_iRoiWidth)*g_byteBitDepthNo + 1] = 0;

			pDataBuffer[(g_ptRoi3.y + i) *g_width + g_ptRoi3.x * g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi3.y + i) *g_width + g_ptRoi3.x * g_byteBitDepthNo + 1] = 0;
			pDataBuffer[(g_ptRoi3.y + i) *g_width + (g_ptRoi3.x + g_iRoiWidth)*g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi3.y + i) *g_width + (g_ptRoi3.x + g_iRoiWidth)*g_byteBitDepthNo + 1] = 0;

			pDataBuffer[(g_ptRoi4.y + i) *g_width + g_ptRoi4.x * g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi4.y + i) *g_width + g_ptRoi4.x * g_byteBitDepthNo + 1] = 0;
			pDataBuffer[(g_ptRoi4.y + i) *g_width + (g_ptRoi4.x + g_iRoiWidth)*g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi4.y + i) *g_width + (g_ptRoi4.x + g_iRoiWidth)*g_byteBitDepthNo + 1] = 0;

			pDataBuffer[(g_ptRoi5.y + i) *g_width + g_ptRoi5.x * g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi5.y + i) *g_width + g_ptRoi5.x * g_byteBitDepthNo + 1] = 0;
			pDataBuffer[(g_ptRoi5.y + i) *g_width + (g_ptRoi5.x + g_iRoiWidth)*g_byteBitDepthNo] = 0;
			pDataBuffer[(g_ptRoi5.y + i) *g_width + (g_ptRoi5.x + g_iRoiWidth)*g_byteBitDepthNo + 1] = 0;
		}

	}
	cv::Mat frame(g_height, (g_byteBitDepthNo == 1 ? g_width: g_width/2), (g_byteBitDepthNo==1? CV_8UC1: CV_16UC1),pDataBuffer);
	//cv::Mat frame(g_height, (g_byteBitDepthNo == 1 ? g_width : g_width / 2),  CV_16UC1, pDataBuffer16); 
	cv::imshow("disp",frame);
	//delete[] pDataBuffer16;
	//cv::waitKey(1);//没用，纯粹延时,   而且会导致数据量下降
	//BMPHeader(g_width,g_height,pDataBuffer);
	if(snap||save_all)
	{
		CString strName;
		CString camFolder;
		//camFolder.Format(L"cam%d",0);
		if(1)//CreateDirectory(camFolder,NULL)||ERROR_ALREADY_EXISTS == GetLastError())
		{
			int iFileIndex=1;
			do 
			{
				strName.Format(g_byteBitDepthNo == 1?L"V_%d.bmp": L"V_%d.png",iFileIndex);
				++iFileIndex;
			} while (_waccess(strName,0)==0);
		
			CT2CA pszConvertedAnsiString (strName);
			std::string cvfilename(pszConvertedAnsiString);

			cv::vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
			compression_params.push_back(0);    // 无压缩png.  
			//sprintf(fileName, "Picture %d.png", ++countCamera); //生成文件名  
			//cv::Mat pImgMat(pImg, 0);  //把IplImage转换为Mat  
			//imwrite(fileName, pImgMat, compression_params);  //保存图片</span>  


			cv::imwrite(cvfilename,frame, g_byteBitDepthNo == 1 ? compression_params: std::vector<int>());
			snap=false;
		}
	}
	if (g_bSaveVedio == true)
		h_vw.write(frame);//save video operation
	if (g_bRoi == true)
	{
		if (g_bFillRoiBuf == true)
		{

			for (i = 0; i < g_iRoiHeight; i++)
			{
				for (j = 0; j < g_iRoiWidth*g_byteBitDepthNo; j++)
				{
					g_byteRoiBuffer1[i*g_iRoiWidth*g_byteBitDepthNo + j] = pDataBuffer[(g_ptRoi1.y + i)*g_width + g_ptRoi1.x*g_byteBitDepthNo + j];
					g_byteRoiBuffer2[i*g_iRoiWidth*g_byteBitDepthNo + j] = pDataBuffer[(g_ptRoi2.y + i)*g_width + g_ptRoi2.x*g_byteBitDepthNo + j];
					g_byteRoiBuffer3[i*g_iRoiWidth*g_byteBitDepthNo + j] = pDataBuffer[(g_ptRoi3.y + i)*g_width + g_ptRoi3.x*g_byteBitDepthNo + j];
					g_byteRoiBuffer4[i*g_iRoiWidth*g_byteBitDepthNo + j] = pDataBuffer[(g_ptRoi4.y + i)*g_width + g_ptRoi4.x*g_byteBitDepthNo + j];
					g_byteRoiBuffer5[i*g_iRoiWidth*g_byteBitDepthNo + j] = pDataBuffer[(g_ptRoi5.y + i)*g_width + g_ptRoi5.x*g_byteBitDepthNo + j];
				}
			}
			g_bFillRoiBuf = false;

		}
	}

}
void CUsbControlDlg::OnBnClickedBtnVideocapture()
{
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}


	CButton* pBtn =(CButton*) GetDlgItem(IDC_CHECK_SAVE_VEDIO);
	if (pBtn->GetCheck() == 1)
	{
		CButton* pBtn2 = (CButton*)GetDlgItem(IDC_RADIO_16BIT_DEPTH);
		if (pBtn2->GetCheck() == 1)
		{
			SetDlgItemText(IDC_STATIC_TEXT, L"不支持16位深度视频录制。");
			return;
		}
		std::string filename = "videofile.avi";
		cv::Size videosize = cv::Size(g_byteBitDepthNo == 1? g_width: g_width/2,  g_height);
		bool ret=h_vw.open(filename, CV_FOURCC('X', 'V','I', 'D'), 15, videosize, 1);
		//h_vw.open(filename, -1, 15.0, videosize, 1);
		if (!h_vw.isOpened())
		{
			SetDlgItemText(IDC_STATIC_TEXT, L"保存视频失败。");
			return;
		}
		g_bSaveVedio = true;
		SetDlgItemText(IDC_STATIC_TEXT, L"开始采集。");
	}
	else
		g_bSaveVedio = false;



	// TODO: 在此添加控件通知处理程序代码

	GetDlgItem(IDC_RADIO1)->EnableWindow(false);
	GetDlgItem(IDC_RADIO2)->EnableWindow(false);
	GetDlgItem(IDC_RADIO3)->EnableWindow(false);
	GetDlgItem(IDC_RADIO4)->EnableWindow(false);

	GetDlgItem(IDC_RADIO_8BIT_DEPTH)->EnableWindow(false);
	GetDlgItem(IDC_RADIO_16BIT_DEPTH)->EnableWindow(false);
	GetDlgItem(IDC_RADIO5)->EnableWindow(false);

	GetDlgItem(IDC_CHECK_SAVE_VEDIO)->EnableWindow(false);



	cv::namedWindow("disp");
	HWND hWnd = (HWND)cvGetWindowHandle("disp");//获取子窗口的HWND
	HWND hParentWnd = ::GetParent(hWnd);//获取父窗口HWND。父窗口是我们要用的

	//::SetWindowPos(hParentWnd, HWND_TOPMOST, 100, 1, 500, 500, SWP_NOSIZE | SWP_NOMOVE); //修改窗口为最顶部

	 //隐藏窗口标题栏 
	long style = GetWindowLong(hParentWnd, GWL_STYLE);
	//style &= ~(WS_CAPTION);
	//style &= ~(WS_MAXIMIZEBOX); 
	//style &= ~(WS_MINIMIZEBOX);
	style &= ~(WS_SYSMENU);
	SetWindowLong(hParentWnd, GWL_STYLE, style);

	SetDlgItemText(IDC_STATIC_TEXT,L"采集中...");
	//CheckRadioButton(IDC_RADIO_NORMAL,IDC_RADIO_XYMIRROR,IDC_RADIO_NORMAL);

	if(h_cctapi->startCap2(g_height,g_width,RawCallBack)<0)
	{
		SetDlgItemText(IDC_STATIC_TEXT,L"USB设备打开失败！");

		GetDlgItem(IDC_RADIO1)->EnableWindow(true);
		GetDlgItem(IDC_RADIO2)->EnableWindow(true);
		GetDlgItem(IDC_RADIO3)->EnableWindow(true);
		GetDlgItem(IDC_RADIO4)->EnableWindow(true);

		GetDlgItem(IDC_RADIO_8BIT_DEPTH)->EnableWindow(true);
		GetDlgItem(IDC_RADIO_16BIT_DEPTH)->EnableWindow(true);
		GetDlgItem(IDC_RADIO5)->EnableWindow(true);

		GetDlgItem(IDC_CHECK_SAVE_VEDIO)->EnableWindow(true);

		return;
	}
	else
	{

	}
}


void CUsbControlDlg::OnBnClickedBtnStopcapture()
{
	if(h_cctapi->stopCap2()!=0)
	{
		SetDlgItemText(IDC_STATIC_TEXT,L"尚未采集");
		return;
	}
	cv::destroyWindow("disp");
	SetDlgItemText(IDC_STATIC_TEXT,L" ");
	//m_bUsbOpen=FALSE;

	GetDlgItem(IDC_RADIO1)->EnableWindow(true);
	GetDlgItem(IDC_RADIO2)->EnableWindow(true);
	GetDlgItem(IDC_RADIO3)->EnableWindow(true);
	GetDlgItem(IDC_RADIO4)->EnableWindow(true);

	GetDlgItem(IDC_RADIO_8BIT_DEPTH)->EnableWindow(true);
	GetDlgItem(IDC_RADIO_16BIT_DEPTH)->EnableWindow(true);
	GetDlgItem(IDC_RADIO5)->EnableWindow(true);

	GetDlgItem(IDC_CHECK_SAVE_VEDIO)->EnableWindow(true);

	SetDlgItemText(IDC_STATIC_TEXT, L"停止采集。");

	//h_vw.release();
}

void CUsbControlDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
	KillTimer(1);

	Sleep(100);
//	if(m_pVideoDlg!=NULL)
	{
//		delete m_pVideoDlg;
//		m_pVideoDlg=NULL;
	}
	//CloseUsb();
	//CyUsb_Destroy();
	if(m_pBrush!=NULL)
	{
		for(int i=0;i<2;++i)
		{
			if(m_pBrush[i].m_hObject!=NULL)
			{
				m_pBrush[i].DeleteObject();
			}
		}
		delete[] m_pBrush;
		m_pBrush=NULL;
	}
}

void CUsbControlDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	int iFrame=0;
	long lBytePerSecond = 0;

	double fRoiValue1 = 0.0;
	double fRoiValue2 = 0.0;
	double fRoiValue3 = 0.0;
	double fRoiValue4 = 0.0;
	double fRoiValue5 = 0.0;

	CString str1;
	CString str2;
	CString str3;
	CString str4;
	CString str5;


	int i = 0, j = 0;

	CString str;
	switch(nIDEvent)
	{
		case 1:
			{
				h_cctapi->GetFpsMbs(iFrame, lBytePerSecond);
				str.Format(L"%d Fps     %0.4f MBs",iFrame,float(lBytePerSecond)/1024.0/1024.0);
			 
				HWND hWnd = (HWND)cvGetWindowHandle("disp");//获取子窗口的HWND
				HWND hParentWnd = ::GetParent(hWnd);//获取父窗口HWND。父窗口是我们要用的
				
				if(hParentWnd !=NULL)
				{
					::SetWindowText(hParentWnd,str);
				}

				//////////////////////////////////
				if (g_bRoi == true)
				{
					unsigned short temp = 0, temp2=0;
					for (i = 0; i < g_iRoiHeight; i++)
					{
						for (j = 0; j < g_iRoiWidth; j++)
						{
							if (g_byteBitDepthNo == 1)
							{
								fRoiValue1 += g_byteRoiBuffer1[i*g_iRoiWidth + j];
								fRoiValue2 += g_byteRoiBuffer2[i*g_iRoiWidth + j];
								fRoiValue3 += g_byteRoiBuffer3[i*g_iRoiWidth + j];
								fRoiValue4 += g_byteRoiBuffer4[i*g_iRoiWidth + j];
								fRoiValue5 += g_byteRoiBuffer5[i*g_iRoiWidth + j];
							}
							else
							{
								temp  = g_byteRoiBuffer1[(i*g_iRoiWidth + j)*g_byteBitDepthNo];
								temp = temp << 8;
								fRoiValue1 += temp+g_byteRoiBuffer1[(i*g_iRoiWidth + j)*g_byteBitDepthNo+1];


								temp = g_byteRoiBuffer2[(i*g_iRoiWidth + j)*g_byteBitDepthNo];
								temp = temp << 8;
								fRoiValue2 += temp + g_byteRoiBuffer2[(i*g_iRoiWidth + j)*g_byteBitDepthNo + 1];

								temp = g_byteRoiBuffer3[(i*g_iRoiWidth + j)*g_byteBitDepthNo];
								temp = temp << 8;
								fRoiValue3 += temp + g_byteRoiBuffer3[(i*g_iRoiWidth + j)*g_byteBitDepthNo + 1];

								temp = g_byteRoiBuffer4[(i*g_iRoiWidth + j)*g_byteBitDepthNo];
								temp = temp << 8;
								fRoiValue4 += temp + g_byteRoiBuffer4[(i*g_iRoiWidth + j)*g_byteBitDepthNo + 1];

								temp = g_byteRoiBuffer5[(i*g_iRoiWidth + j)*g_byteBitDepthNo];
								temp = temp << 8;
								fRoiValue5 += temp + g_byteRoiBuffer5[(i*g_iRoiWidth + j)*g_byteBitDepthNo + 1];

							}
						}

					}

					fRoiValue1 = fRoiValue1 / (g_iRoiHeight*g_iRoiWidth);
					fRoiValue2 = fRoiValue2 / (g_iRoiHeight*g_iRoiWidth);
					fRoiValue3 = fRoiValue3 / (g_iRoiHeight*g_iRoiWidth);
					fRoiValue4 = fRoiValue4 / (g_iRoiHeight*g_iRoiWidth);
					fRoiValue5 = fRoiValue5 / (g_iRoiHeight*g_iRoiWidth);

					str1.Format(L"%0.4f", fRoiValue1);
					str2.Format(L"%0.4f", fRoiValue2);
					str3.Format(L"%0.4f", fRoiValue3);
					str4.Format(L"%0.4f", fRoiValue4);
					str5.Format(L"%0.4f", fRoiValue5);

					SetDlgItemText(IDC_EDIT_ROI_VALUE1, str1);
					SetDlgItemText(IDC_EDIT_ROI_VALUE2, str2);
					SetDlgItemText(IDC_EDIT_ROI_VALUE3, str3);
					SetDlgItemText(IDC_EDIT_ROI_VALUE4, str4);
					SetDlgItemText(IDC_EDIT_ROI_VALUE5, str5);
					
					g_bFillRoiBuf = true;
				}

				break;
			}
		default:
			break;
	}
	CDialogEx::OnTimer(nIDEvent);
}

//void CUsbControlDlg::OnBnClickedRadioProcType()
//{
//	UpdateData(TRUE);
	//h_cctapi->setMirrorType(DataProcessType(m_iProcType));
//}


HBRUSH CUsbControlDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	// TODO:  在此更改 DC 的任何特性
	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	int ID=pWnd->GetDlgCtrlID();
	if(ID==IDC_STATIC_TEXT)
	{
		pDC->SetTextColor(RGB(0,0,255));
		pDC->SetBkMode(TRANSPARENT);
	}
	switch(nCtlColor)
	{
	case CTLCOLOR_DLG:
	case CTLCOLOR_BTN:
		return m_pBrush[0];
	case CTLCOLOR_STATIC:
		return m_pBrush[0];
	default:
		return hbr;
	}
}


void CUsbControlDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString s_temp;
	UpdateData(true);
	s_temp = m_sEdit_Width.GetString();
	g_width = _tstoi(s_temp);
	s_temp.ReleaseBuffer();
}


void CUsbControlDlg::OnEnChangeEdit2()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString s_temp;
	UpdateData(true);
	s_temp = m_sEdit_Height.GetString();
	g_height = _tstoi(s_temp);
	s_temp.ReleaseBuffer();
}

void CUsbControlDlg::Split(CString in, CString *out, int &outcnt, CString spliter,int maxn)
{

	int d_len=spliter.GetLength();
	int j=0;
	int n=0;
	int m_pos;
	while(1)
	{
		m_pos= in.Find(spliter,j);
		if(m_pos==-1 && j==0)
		{
			out[0]=in.Mid(0);
			outcnt=0;//-1
			break;
		}
		if((m_pos==-1 && j!=0)||(n==maxn))
		{
			out[n]=in.Mid(j,in.GetLength()-j);
			outcnt=n;
			break;
		}

		if(j==0)
		{
			out[n]=in.Mid(0,m_pos);
			j=m_pos+d_len;
		}
		else
		{
			out[n]=in.Mid(j,m_pos-j);
			j=m_pos+d_len;
		}
		n++;
	}
}

void CUsbControlDlg::OnBnClickedBnSnap()
{
	SetDlgItemText(IDC_STATIC_TEXT, L"开始保存截图。");
	snap=true;
}


void CUsbControlDlg::setTrigMode()
{
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	CString strTrigMode;
	GetDlgItemText(IDC_COMBOTrigMode, strTrigMode);
	if (strTrigMode == L"AutoTrig")
	{
		h_cctapi->SetTrigModAuto();
	}
	else if (strTrigMode == L"FpgaTrig")
	{	
		CString s_temp;
		UpdateData(true);
		eFpgaFreq.GetWindowText(s_temp);
		unsigned char fpgafreq = _tstoi(s_temp);	
		if (cbTrigMode.GetCurSel() == 2 && fpgafreq <= 0)
		{
			SetDlgItemText(IDC_STATIC_TEXT, L"Need Fpga Freq");
			return;
		}
		h_cctapi->SetTrigModFpga(fpgafreq);
	}
	else
	{ }
	return;
}


void CUsbControlDlg::setFpgaFreq()
{
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	SetDlgItemText(IDC_STATIC_TEXT, L"FPGA触发频率设定成功。");
	CString s_temp;
	UpdateData(true);
	eFpgaFreq.GetWindowText(s_temp);
	unsigned char fpgafreq= _tstoi(s_temp);
	s_temp.ReleaseBuffer();
	//int a=cbTrigMode.GetCurSel();
	if(fpgafreq>0)
	{
		h_cctapi->setFpgaFreq(fpgafreq & 0xff);
	}
	else
	{
		SetDlgItemText(IDC_STATIC_TEXT,L"Check Trig value");
	}
}


void CUsbControlDlg::setGainValue()
{
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	
	CString s_temp;
	UpdateData(true);
	eGainValue.GetWindowText(s_temp);
	unsigned short GainValue= _tstoi(s_temp);
	s_temp.ReleaseBuffer();

	if(GainValue>0)
	{
		h_cctapi->setGainValue(GainValue);
		SetDlgItemText(IDC_STATIC_TEXT, L"增益值设定成功。");
	}
	else
	{
		SetDlgItemText(IDC_STATIC_TEXT,L"Check Gain?");
	}
}


void CUsbControlDlg::setAutoGainExpo()
{
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	if ((cbAutoGain.GetCheck() == true) && (cbAutoExpo.GetCheck() == true))
	{
		h_cctapi->setAutoGainExpo(true, true);

		//GetDlgItem(IDC_EDITGainValue)->EnableWindow(false);
		//GetDlgItem(IDC_EDITExpoValue)->EnableWindow(false);
		SetDlgItemText(IDC_STATIC_TEXT, L"自动增益，自动曝光。");
	}	
	else if ((cbAutoGain.GetCheck() == true) && (cbAutoExpo.GetCheck() == false))
	{
		h_cctapi->setAutoGainExpo(true, false);

		//GetDlgItem(IDC_EDITGainValue)->EnableWindow(false);
		//GetDlgItem(IDC_EDITExpoValue)->EnableWindow(true);
		SetDlgItemText(IDC_STATIC_TEXT, L"自动增益，手动曝光。");
	}
	else if ((cbAutoGain.GetCheck() == false) && (cbAutoExpo.GetCheck() == true))
	{
		h_cctapi->setAutoGainExpo(false, true);

		//GetDlgItem(IDC_EDITGainValue)->EnableWindow(true);
		//GetDlgItem(IDC_EDITExpoValue)->EnableWindow(false);
		SetDlgItemText(IDC_STATIC_TEXT, L"手动增益，自动曝光。");
	}
	else if ((cbAutoGain.GetCheck() == false) && (cbAutoExpo.GetCheck() == false))
	{
		h_cctapi->setAutoGainExpo(false, false);

		//GetDlgItem(IDC_EDITGainValue)->EnableWindow(true);
		//GetDlgItem(IDC_EDITExpoValue)->EnableWindow(true);
		SetDlgItemText(IDC_STATIC_TEXT, L"手动增益，手动曝光。");
	}
	else;
}


void CUsbControlDlg::setExpoValue()
{
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	
	CString s_temp;
	UpdateData(true);
	eExpoValue.GetWindowText(s_temp);
	unsigned short ExpoValue= _tstoi(s_temp);
	s_temp.ReleaseBuffer();
	if(/*cbAutoExpo.GetCheck()==false&&*/ExpoValue>0)
	{
		h_cctapi->setExpoValue(ExpoValue);
		SetDlgItemText(IDC_STATIC_TEXT, L"曝光值设定成功。");
	}
	else
	{
		SetDlgItemText(IDC_STATIC_TEXT,L"Check Expo?");
	}
}




void CUsbControlDlg::OnBnClickedButtonsofttrig()
{
	//if(cbTrigMode.GetCurSel()==1)
	//{
	//	m_sUsbOrder.ReqCode=SOFTTRIG;
	//	m_sUsbOrder.DataBytes=0;
	//	m_sUsbOrder.Direction=0;
	//	SendOrder(&m_sUsbOrder);
	//}
	//else
	//{
	//	SetDlgItemText(IDC_STATIC_TEXT,L"Triger Mode?");
	//}
}


void CUsbControlDlg::OnBnClickedCheckSaveall()
{
	// TODO: Add your control notification handler code here
	save_all=m_chk_save_all.GetCheck();

}


void CUsbControlDlg::OnBnClickedButtonWrSensor()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	CString strAddr, strValue;

	GetDlgItemText(IDC_EDIT_SENSOR_REGISTER_ADDR, strAddr);
	GetDlgItemText(IDC_EDIT_SENSOR_REGISTER_VALUE, strValue);

	unsigned short iAddr = str2hex(strAddr);
	unsigned short iValue = str2hex(strValue);

	h_cctapi->WrSensorReg(iAddr, iValue);
	SetDlgItemText(IDC_STATIC_TEXT, L"设置Sensor寄存器成功。");
}
int CUsbControlDlg::str2hex(CString str)
{
	int nLength = str.GetLength();
	int nBytes = WideCharToMultiByte(CP_ACP, 0, str, nLength, NULL, 0, NULL, NULL);
	char* p = new char[nBytes + 1];
	memset(p, 0, nLength + 1);
	WideCharToMultiByte(CP_OEMCP, 0, str, nLength, p, nBytes, NULL, NULL);
	p[nBytes] = 0;
	int a;
	sscanf(p, "%x", &a);
	delete[] p;
	return a;

}


void CUsbControlDlg::OnBnClickedButtonRdSensor()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	CString strAddr;

	GetDlgItemText(IDC_EDIT_SENSOR_REGISTER_ADDR, strAddr);

	unsigned short iAddr = str2hex(strAddr);	
	unsigned short irxval = h_cctapi->RdSensorReg(iAddr);
	CString s_temp;
	s_temp.Format(_T("%04x"), irxval);
	SetDlgItemText(IDC_EDIT_SENSOR_REGISTER_VALUE, s_temp);

	SetDlgItemText(IDC_STATIC_TEXT, L"读取Sensor寄存器成功。");
		


}


void CUsbControlDlg::OnBnClickedButtonWrFpga()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	CString strAddr, strValue;

	GetDlgItemText(IDC_EDIT_FPGA_REGISTER_ADDR, strAddr);
	GetDlgItemText(IDC_EDIT_FPGA_REGISTER_VALUE, strValue);

	unsigned char iAddr = str2hex(strAddr);
	unsigned char iValue = str2hex(strValue);

	h_cctapi->WrFpgaReg(iAddr, iValue);
}


void CUsbControlDlg::OnBnClickedButtonRdFpga()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	CString strAddr;
	GetDlgItemText(IDC_EDIT_FPGA_REGISTER_ADDR, strAddr);
	unsigned char iAddr = str2hex(strAddr);
	unsigned char irxval = h_cctapi->RdFpgaReg(iAddr);
	CString s_temp;
	s_temp.Format(_T("%02x"), irxval);
	SetDlgItemText(IDC_EDIT_FPGA_REGISTER_VALUE, s_temp);
	SetDlgItemText(IDC_STATIC_TEXT, L"FPGA触发频率设置成功。");
}


void CUsbControlDlg::OnBnClickedButtonInitSensor()
{
	// TODO: 在此添加控件通知处理程序代码

	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	h_cctapi->InitSensor();
	SetDlgItemText(IDC_STATIC_TEXT, L"初始化sensor成功。");
}

void CUsbControlDlg::OnBnClickedRadio1()//1280*720
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	h_cctapi->setResolution(0);

	g_byteResolutionType = RESOLUTION_1280_720;
	g_width = 1280 * g_byteBitDepthNo;
	g_height = 720 ;
	//SetDlgItemText(IDC_EDIT1, L"1280");
	//SetDlgItemText(IDC_EDIT2, L"720");
	SetDlgItemText(IDC_STATIC_TEXT, L"分辨率1280*720。");
}


void CUsbControlDlg::OnBnClickedRadio2()//1280*960
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	h_cctapi->setResolution(1);

	g_byteResolutionType = RESOLUTION_1280_960;
	g_width = 1280 * g_byteBitDepthNo;
	g_height = 960;
	
	//SetDlgItemText(IDC_EDIT1, L"1280");
	//SetDlgItemText(IDC_EDIT2, L"960");
	SetDlgItemText(IDC_STATIC_TEXT, L"分辨率1280*960。");
}


void CUsbControlDlg::OnBnClickedRadio3()//640*480  skip
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}

	h_cctapi->setResolution(2);

	g_byteResolutionType = RESOLUTION_640_480_SKIP;
	g_width = 640* g_byteBitDepthNo;
	g_height = 480 ;

	//SetDlgItemText(IDC_EDIT1, L"640");
	//SetDlgItemText(IDC_EDIT2, L"480");
	SetDlgItemText(IDC_STATIC_TEXT, L"分辨率640*480。");
}
void CUsbControlDlg::OnBnClickedRadio4()//640*480  binning
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}

	h_cctapi->setResolution(3);

	g_byteResolutionType = RESOLUTION_640_480_BINNING;
	g_width = 640 * g_byteBitDepthNo;
	g_height= 480;

	//SetDlgItemText(IDC_EDIT1, L"640");
	//SetDlgItemText(IDC_EDIT2, L"480");
	SetDlgItemText(IDC_STATIC_TEXT, L"分辨率640*480。");
}

void CUsbControlDlg::OnBnClickedRadioNormal()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	h_cctapi->setNormal();
}


void CUsbControlDlg::OnBnClickedRadioXmirror()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	h_cctapi->setXmirror();
}


void CUsbControlDlg::OnBnClickedRadioYmirror()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	h_cctapi->setYmirror();
}


void CUsbControlDlg::OnBnClickedRadioXymirror()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}

	h_cctapi->setXYmirror();
}


void CUsbControlDlg::OnBnClickedButtonOpenUsb()
{
	// TODO: 在此添加控件通知处理程序代码
	CyUsb_Init();
	if (OpenUsb()<0)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"打开USB失败。");
		return;
	}
		
	m_bUsbOpen = true;
	SetDlgItemText(IDC_STATIC_TEXT, L"打开USB成功。");



}


void CUsbControlDlg::OnBnClickedButtonCloseUsb()
{
	// TODO: 在此添加控件通知处理程序代码
	OnBnClickedBtnStopcapture();
	CloseUsb();
	m_bUsbOpen = false;
}


void CUsbControlDlg::OnBnClickedCheckSaveVedio()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CUsbControlDlg::OnBnClickedRadioAutoTrig()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}

	h_cctapi->SetTrigModAuto();

	GetDlgItem(IDC_EDITHwTrigFreq)->EnableWindow(false);
	SetDlgItemText(IDC_STATIC_TEXT, L"自动触发。");
}


void CUsbControlDlg::OnBnClickedRadioFpgaTrig()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}
	
	GetDlgItem(IDC_EDITHwTrigFreq)->EnableWindow(true);

	CString s_temp;
	UpdateData(true);
	eFpgaFreq.GetWindowText(s_temp);
	unsigned char fpgafreq = _tstoi(s_temp);
	if (/*cbTrigMode.GetCurSel() == 2 &&*/ fpgafreq <= 0)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"Need Fpga Freq");
		return;
	}

	h_cctapi->SetTrigModFpga(fpgafreq);
	SetDlgItemText(IDC_STATIC_TEXT, L"FPGA触发。");


	
}


void CUsbControlDlg::OnBnClickedButtonUsbSpeedCkeck()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bUsbOpen)
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"USB未打开。");
		return;
	}

	CCyUSBDevice* pUsbDev = new CCyUSBDevice(NULL);

	if (pUsbDev->bHighSpeed == false)
	{
		SetDlgItemText(IDC_STATIC_TEXT2, L"USB3.0");
		h_cctapi->SendUsbSpeed2Fpga(0x0);
	}
		
	else
	{
		SetDlgItemText(IDC_STATIC_TEXT2, L"USB2.0");
		h_cctapi->SendUsbSpeed2Fpga(0x1);
	}
		
}





void CUsbControlDlg::OnBnClickedRadio8bitDepth()
{
	// TODO: 在此添加控件通知处理程序代码
	g_byteBitDepthNo = 1;
	if ((g_byteResolutionType == RESOLUTION_1280_720)||(g_byteResolutionType == RESOLUTION_1280_960))
		g_width = 1280 * g_byteBitDepthNo;
	else if ((g_byteResolutionType == RESOLUTION_640_480_SKIP) || (g_byteResolutionType == RESOLUTION_640_480_BINNING))
		g_width = 640 * g_byteBitDepthNo;
	else;

	h_cctapi->SetBitDepth(0x0);
}


void CUsbControlDlg::OnBnClickedRadio16bitDepth()
{
	// TODO: 在此添加控件通知处理程序代码
	g_byteBitDepthNo = 2;
	if ((g_byteResolutionType == RESOLUTION_1280_720) || (g_byteResolutionType == RESOLUTION_1280_960))
		g_width = 1280 * g_byteBitDepthNo;
	else if ((g_byteResolutionType == RESOLUTION_640_480_SKIP) || (g_byteResolutionType == RESOLUTION_640_480_BINNING))
		g_width = 640 * g_byteBitDepthNo;
	else;
	h_cctapi->SetBitDepth(0x1);
}


void CUsbControlDlg::OnBnClickedRadioAnlog1x()
{
	// TODO: 在此添加控件通知处理程序代码
	if(this->IsDlgButtonChecked(IDC_RADIO_AUTO_TRIG)==1)
		h_cctapi->SetAnalogGain_AutoTrig(1);
	else if(this->IsDlgButtonChecked(IDC_RADIO_FPGA_TRIG) == 1)
		h_cctapi->SetAnalogGain_FpgaTrig(1);
	else;
}


void CUsbControlDlg::OnBnClickedRadioAnlog2x()
{
	// TODO: 在此添加控件通知处理程序代码
	if (this->IsDlgButtonChecked(IDC_RADIO_AUTO_TRIG) == 1)
		h_cctapi->SetAnalogGain_AutoTrig(2);
	else if (this->IsDlgButtonChecked(IDC_RADIO_FPGA_TRIG) == 1)
		h_cctapi->SetAnalogGain_FpgaTrig(2);
	else;
}


void CUsbControlDlg::OnBnClickedRadioAnlog3x()
{
	// TODO: 在此添加控件通知处理程序代码
	if (this->IsDlgButtonChecked(IDC_RADIO_AUTO_TRIG) == 1)
		h_cctapi->SetAnalogGain_AutoTrig(3);
	else if (this->IsDlgButtonChecked(IDC_RADIO_FPGA_TRIG) == 1)
		h_cctapi->SetAnalogGain_FpgaTrig(3);
	else;
}


void CUsbControlDlg::OnBnClickedRadioAnlog4x()
{
	// TODO: 在此添加控件通知处理程序代码
	if (this->IsDlgButtonChecked(IDC_RADIO_AUTO_TRIG) == 1)
		h_cctapi->SetAnalogGain_AutoTrig(4);
	else if (this->IsDlgButtonChecked(IDC_RADIO_FPGA_TRIG) == 1)
		h_cctapi->SetAnalogGain_FpgaTrig(4);
	else;
}





void CUsbControlDlg::OnEnChangeEditRoiX1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_X1, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiWidth > g_width))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或宽度错误。");
		g_bRoiWidthOK = false;
		return;
	}
	g_ptRoi1.x = temp;
	g_bRoiWidthOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiY1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_Y1, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiHeight > g_height))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或高度错误。");
		g_bRoiHeightOK = false;
		return;
	}
	g_ptRoi1.y = temp;
	g_bRoiHeightOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiX2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_X2, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiWidth > g_width))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或宽度错误。");
		g_bRoiWidthOK = false;
		return;
	}
	g_ptRoi2.x = temp;
	g_bRoiWidthOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiY2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_Y2, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiHeight > g_height))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或高度错误。");
		g_bRoiHeightOK = false;
		return;
	}
	g_ptRoi2.y = temp;
	g_bRoiHeightOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiX3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_X3, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiWidth > g_width))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或宽度错误。");
		g_bRoiWidthOK = false;
		return;
	}
	g_ptRoi3.x = temp;
	g_bRoiWidthOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiY3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_Y3, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiHeight > g_height))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或高度错误。");
		g_bRoiHeightOK = false;
		return;
	}
	g_ptRoi3.y = temp;
	g_bRoiHeightOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiX4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_X4, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiWidth > g_width))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或宽度错误。");
		g_bRoiWidthOK = false;
		return;
	}
	g_ptRoi4.x = temp;
	g_bRoiWidthOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiY4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_Y4, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiHeight > g_height))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或高度错误。");
		g_bRoiHeightOK = false;
		return;
	}
	g_ptRoi4.y = temp;
	g_bRoiHeightOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiX5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_X5, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiWidth > g_width))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标或宽度错误。");
		g_bRoiWidthOK = false;
		return;
	}
	g_ptRoi5.x = temp;
	g_bRoiWidthOK = true;
}


void CUsbControlDlg::OnEnChangeEditRoiY5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_Y5, str);
	int temp = _tstoi(str);
	if ((temp < 0) || (temp + g_iRoiHeight > g_height))
	{
		SetDlgItemText(IDC_STATIC_TEXT, L"坐标错误。");
		g_bRoiHeightOK = false;
		return;
	}
	g_ptRoi5.y = temp;
	g_bRoiHeightOK = true;
}





void CUsbControlDlg::OnBnClickedCheckRoi()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton* pBtn = (CButton*)GetDlgItem(IDC_CHECK_ROI);
	if (pBtn->GetCheck() == 1)
	{
		g_byteRoiBuffer1 = new BYTE[g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight];
		g_byteRoiBuffer2 = new BYTE[g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight];
		g_byteRoiBuffer3 = new BYTE[g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight];
		g_byteRoiBuffer4 = new BYTE[g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight];
		g_byteRoiBuffer5 = new BYTE[g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight];

		memset(g_byteRoiBuffer1, 0, g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight);
		memset(g_byteRoiBuffer2, 0, g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight);
		memset(g_byteRoiBuffer3, 0, g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight);
		memset(g_byteRoiBuffer4, 0, g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight);
		memset(g_byteRoiBuffer5, 0, g_iRoiWidth *g_byteBitDepthNo * g_iRoiHeight);

		GetDlgItem(IDC_EDIT_ROI_WIDTH)->EnableWindow(false);
		GetDlgItem(IDC_EDIT_ROI_HEIGHT)->EnableWindow(false);

		g_bRoi = true;
	}
	else
	{
		g_bRoi = false;

		GetDlgItem(IDC_EDIT_ROI_WIDTH)->EnableWindow(true);
		GetDlgItem(IDC_EDIT_ROI_HEIGHT)->EnableWindow(true);

		delete[] g_byteRoiBuffer1;
		delete[] g_byteRoiBuffer2;
		delete[] g_byteRoiBuffer3;
		delete[] g_byteRoiBuffer4;
		delete[] g_byteRoiBuffer5;

	}

}


void CUsbControlDlg::OnBnClickedCheckRoiBox()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton* pBtn = (CButton*)GetDlgItem(IDC_CHECK_ROI_BOX);
	if (pBtn->GetCheck() == 1)
	{

		g_bRoiBox = true;
	}
	else
	{
		g_bRoiBox = false;
	}
}


void CUsbControlDlg::OnEnChangeEditRoiWidth()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_WIDTH, str);
	unsigned int temp = g_iRoiWidth;
	g_iRoiWidth = _tstoi(str);

	OnEnChangeEditRoiX1();
	if (g_bRoiWidthOK == false)
	{
		g_iRoiWidth = temp;
		return;
	}
	OnEnChangeEditRoiX2();
	if (g_bRoiWidthOK == false)
	{
		g_iRoiWidth = temp;
		return;
	}
	OnEnChangeEditRoiX3();
	if (g_bRoiWidthOK == false)
	{
		g_iRoiWidth = temp;
		return;
	}
	OnEnChangeEditRoiX4();
	if (g_bRoiWidthOK == false)
	{
		g_iRoiWidth = temp;
		return;
	}
	OnEnChangeEditRoiX5();
	if (g_bRoiWidthOK == false)
	{
		g_iRoiWidth = temp;
		return;
	}
	return;
}


void CUsbControlDlg::OnEnChangeEditRoiHeight()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString str;
	GetDlgItemText(IDC_EDIT_ROI_HEIGHT, str);
	unsigned int temp = g_iRoiHeight;
	g_iRoiHeight = _tstoi(str);

	OnEnChangeEditRoiY1();
	if (g_bRoiHeightOK == false)
	{
		g_bRoiHeightOK = temp;
		return;
	}
	OnEnChangeEditRoiY2();
	if (g_bRoiHeightOK == false)
	{
		g_bRoiHeightOK = temp;
		return;
	}
	OnEnChangeEditRoiY3();
	if (g_bRoiHeightOK == false)
	{
		g_bRoiHeightOK = temp;
		return;
	}
	OnEnChangeEditRoiY4();
	if (g_bRoiHeightOK == false)
	{
		g_bRoiHeightOK = temp;
		return;
	}
	OnEnChangeEditRoiY5();
	if (g_bRoiHeightOK == false)
	{
		g_bRoiHeightOK = temp;
		return;
	}
	return;
}


void CUsbControlDlg::OnBnClickedRadio5()
{
	// TODO: 在此添加控件通知处理程序代码
	g_byteBitDepthNo = 1;
	if ((g_byteResolutionType == RESOLUTION_1280_720) || (g_byteResolutionType == RESOLUTION_1280_960))
		g_width = 1280 * g_byteBitDepthNo;
	else if ((g_byteResolutionType == RESOLUTION_640_480_SKIP) || (g_byteResolutionType == RESOLUTION_640_480_BINNING))
		g_width = 640 * g_byteBitDepthNo;
	else;

	h_cctapi->SetBitDepth(0x2);
}
