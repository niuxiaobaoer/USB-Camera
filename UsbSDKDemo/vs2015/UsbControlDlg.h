
// UsbControlDlg.h : 头文件
//

#pragma once
#include "Include/CCTAPI.h"
#include "Include/CyUsb.h"
#include "VideoDlg.h"
#include "afxwin.h"
#include <cv.hpp>
#include <opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
// CUsbControlDlg 对话框
typedef
	VOID
	(WINAPI * LPMV_CALLBACK2)(LPVOID lpParam, LPVOID lpUser);

class CUsbControlDlg : public CDialogEx
{
// 构造
	HICON m_hIcon;
	enum { IDD = IDD_USBCONTROL_DIALOG };
	DECLARE_MESSAGE_MAP()

public:
	CUsbControlDlg(CWnd* pParent = NULL);	
	~CUsbControlDlg();
		
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	
	virtual BOOL OnInitDialog();

private:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBtnOpenusb();
	afx_msg void OnBnClickedBtnOpenfile();
	afx_msg void OnBnClickedBtnVideocapture();
	afx_msg void OnBnClickedBtnStopcapture();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCbnSelchangeComboDriver();
	afx_msg void OnBnClickedRadioDriver();
	//afx_msg void OnBnClickedRadioProcType();
	afx_msg void OnBnClickedRadioChangeType();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnBnClickedBtnReview();
	static unsigned int __stdcall ThreadProcess(void* pParam);
	void ThreadProcessFunction();
	BOOL CloseRbfFile();
	BOOL OpenRbfFile();
	void SendData();
	BOOL OpenDataFile();
	BOOL CloseDataFile();

	//void _stdcall RawCallBack(LPVOID lpParam,LPVOID lpUser);
	void Split(CString in, CString *out, int &outcnt, CString spliter,int maxn);
private:
	BOOL	m_bUsbOpen;
	long          m_lBytePerSecond;
	int			  m_iProcType;
	CBrush*       m_pBrush;	
	CButton m_chk_save_all;
	CCCTAPIApp* h_cctapi;

public:
	afx_msg void OnBnClickedBtnCreatebmp();
	CEdit m_Edit_Width;
	afx_msg void OnEnChangeEdit1();
	CEdit m_Edit_Height;
	afx_msg void OnEnChangeEdit2();
	CString m_sEdit_Width;
	CString m_sEdit_Height;
	afx_msg void OnBnClickedButton2();
	void saveVideoFun(cv::Mat frame);
	BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBnSnap();
	CComboBox cbTrigMode;
	CEdit eFpgaFreq;
	CEdit eGainValue;
	CEdit eExpoValue;
	afx_msg void setTrigMode();
	afx_msg void setGainValue();
	CButton cbAutoGain;
	CButton cbAutoExpo;
	afx_msg void setAutoGainExpo();
	afx_msg void setExpoValue();
	afx_msg void setFpgaFreq();
	afx_msg void OnBnClickedButtonsofttrig();


	afx_msg void OnBnClickedCheckSaveall();
	afx_msg void OnBnClickedButtonWrSensor();
	int str2hex(CString);
	afx_msg void OnBnClickedButtonRdSensor();
	afx_msg void OnBnClickedButtonWrFpga();
	afx_msg void OnBnClickedButtonRdFpga();
	afx_msg void OnBnClickedButtonInitSensor();
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnBnClickedRadio3();
	afx_msg void OnBnClickedRadioNormal();
	afx_msg void OnBnClickedRadioXmirror();
	afx_msg void OnBnClickedRadioYmirror();
	afx_msg void OnBnClickedRadioXymirror();
	afx_msg void OnBnClickedButtonOpenUsb();
	afx_msg void OnBnClickedButtonCloseUsb();
	afx_msg void OnBnClickedCheckSaveVedio();
	afx_msg void OnBnClickedRadioAutoTrig();
	afx_msg void OnBnClickedRadioFpgaTrig();
	afx_msg void OnBnClickedButtonUsbSpeedCkeck();
	afx_msg void OnBnClickedRadio4();
	afx_msg void OnBnClickedRadio8bitDepth();
	afx_msg void OnBnClickedRadio16bitDepth();
	afx_msg void OnBnClickedRadioAnlog1x();
	afx_msg void OnBnClickedRadioAnlog2x();
	afx_msg void OnBnClickedRadioAnlog3x();
	afx_msg void OnBnClickedRadioAnlog4x();
	afx_msg void OnEnChangeEditRoiX1();
	afx_msg void OnEnChangeEditRoiY1();
	afx_msg void OnEnChangeEditRoiX2();
	afx_msg void OnEnChangeEditRoiY2();
	afx_msg void OnEnChangeEditRoiX3();
	afx_msg void OnEnChangeEditRoiY3();
	afx_msg void OnEnChangeEditRoiX4();
	afx_msg void OnEnChangeEditRoiY4();
	afx_msg void OnEnChangeEditRoiX5();
	afx_msg void OnEnChangeEditRoiY5();

	afx_msg void OnBnClickedCheckRoi();
	afx_msg void OnBnClickedCheckRoiBox();
	afx_msg void OnEnChangeEditRoiWidth();
	afx_msg void OnEnChangeEditRoiHeight();
	afx_msg void OnBnClickedRadio5();
};
