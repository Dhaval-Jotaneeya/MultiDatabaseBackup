
// MultiDatabaseBackupRestoreDlg.h : header file
//

#pragma once
#include <vector>
#include <map>
using namespace std;

// CMultiDatabaseBackupRestoreDlg dialog
class CMultiDatabaseBackupRestoreDlg : public CDialogEx
{
// Construction
public:
	CMultiDatabaseBackupRestoreDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MULTIDATABASEBACKUPRESTORE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edtHost;
	CEdit m_edtPort;
	CEdit m_edtUser;
	CEdit m_edtPass;
	CEdit m_edtMaintainanceDB;
	CListCtrl m_listDisplay;
	CEdit m_edtDumpFilePath;
	bool onInit();
	void applyDefaultSettings();
	void applySettings(map<CString, CString>& settings);
	afx_msg void OnBnClickedOk();
	bool storeDlgSettingToFile();

	CString m_sSettingFilePath = L"MultiDatabaseBackup.settings";
	afx_msg void OnBnClickedBtnReconnect();
	CEdit m_edtOutDir;
	afx_msg void OnBnClickedBtnBrowseOutDir();
	CString m_sOutFolderPath;
};
