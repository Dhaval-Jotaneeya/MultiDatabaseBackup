
// MultiDatabaseBackupRestoreDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MultiDatabaseBackupRestore.h"
#include "MultiDatabaseBackupRestoreDlg.h"
#include "afxdialogex.h"
#include <libpq-fe.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//	----------------------------------------------------------------------------------------------------------------------------------------------------------------
CString			____itt(int n)
{
	CString s;
	s.Format(_T("%d"), n);
	return CString();
}
	
BOOL			____isFileExist(LPCTSTR strPath)
{
	CFile file;
	CFileStatus status;

	return file.GetStatus(strPath, status);
}

CString			____get_edt_text(CEdit& edit)
{
	CString str;
	edit.GetWindowText(str);
	return str;
}

CString			____get_temp_path()
{
	TCHAR path[255];
	GetTempPath(255, path);
	return CString(path);
}

bool			____save_server_setting(map<CString, CString>& settings, CString sFilePath)
{
	//	create full string 
	CString sFullString = L"";
	for (pair<CString, CString> cur_setting : settings)
	{
		sFullString += cur_setting.first + L"==" + cur_setting.second + L"\n";
	}
	sFullString = sFullString.Trim();

	//	save to file
	CStdioFile file;
	if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite))
		return false;
	file.WriteString(sFullString);
	file.Close();

	return true;
}

bool			____read_server_setting(CString sFilePath, map<CString, CString>& settings)
{

	//	check for file
	if (!____isFileExist(sFilePath))
		return false;

	//	save to file
	CStdioFile file;
	if (!file.Open(sFilePath, CFile::modeRead))
		return false;

	CString sLine = L"";
	while (file.ReadString(sLine))
	{
		if (sLine.IsEmpty())
			continue;

		int nIndex = sLine.Find(L"==", 0);

		CString sKey = sLine.Mid(0, nIndex);
		CString sValue = sLine.Mid(nIndex + 2, sLine.GetLength());

		settings[sKey] = sValue;
	}

	file.Close();

	return true;
}

CString			____get_cmb_selected(CComboBox& box)
{
	CString s;
	int nIndex = box.GetCurSel();
	box.GetLBText(nIndex, s);
	return s;
}

void			____fillListCtrl(CListCtrl& listctrl, vector<vector<CString>> vector2d, vector<CString> heading)
{
	listctrl.DeleteAllItems();
	while (listctrl.DeleteColumn(0));

	for (int nHeadingIndex = 0; nHeadingIndex < heading.size(); nHeadingIndex++)
	{
		listctrl.InsertColumn(nHeadingIndex, heading[nHeadingIndex], LVS_ALIGNLEFT, 200);
	}

	for (int nRow = 0; nRow < vector2d.size(); nRow++)
	{
		listctrl.InsertItem(nRow, vector2d[nRow][0]);
		for (int nCol = 1; nCol < vector2d[nRow].size(); nCol++)
		{
			listctrl.SetItemText(nRow, nCol, vector2d[nRow][nCol]);
		}
	}
}

void			____addIntoListCtrl(CListCtrl& listctrl, vector<CString> list, int nIndex)
{
	for (int i = 0; i < list.size(); i++)
	{
		if (nIndex == 0)
		{
			listctrl.InsertItem(i, list[i]);
		}
		else
		{
			listctrl.SetItemText(nIndex, i, list[i]);
		}
	}
	
}

void			____fillListBox(CListBox& listbox, vector<CString> data)
{
	for (int i = 0; i < data.size(); i++)
	{
		listbox.AddString(data[i]);
	}
}

//	----------------------------------------------------------------------------------------------------------------------------------------------------------------


//	----------------------------------------------------------------------------------------------------------------------------------------------------------------

struct QueryResult
{
	int nRow;
	int nCol;
	vector<CString> headers;
	vector<vector<CString>> data;
	CString error;
};

//  create connection with given parameter
bool connectToDatabase(CString sHost, CString sPort, CString sUser, CString sPass, CString sDBName, PGconn* (&pDBConn), CString& sError)
{
	//  connection string 
	CString sConnectionString = _T("host= ") + sHost + _T(" port= ") + sPort + _T(" user= ") + sUser + _T(" password= ") + sPass + _T(" dbname= ") + sDBName;

	//  connect to databse
	pDBConn = PQconnectdb(CStringA(sConnectionString));

	//  validation
	if (PQstatus(pDBConn) != CONNECTION_OK)
	{
		sError = (CString)PQerrorMessage(pDBConn);
		return false;
	}

	//  return 
	sError = _T("");
	return true;
}

//  execute tupple query and store into vector map
bool executeSelectQuery(PGconn* (&pDBConn), CString& sQuery, QueryResult& queryResult)
{
	//  execute query
	PGresult* pDBRes = PQexec(pDBConn, CStringA(sQuery));

	//  validation
	if (PQresultStatus(pDBRes) != PGRES_TUPLES_OK)
	{
		queryResult.error = (CString)PQresultErrorMessage(pDBRes);
		return false;
	}

	//  fill data
	{
		//  row col count
		queryResult.nRow = PQntuples(pDBRes);
		queryResult.nCol = PQnfields(pDBRes);

		//  headers
		for (int nCol = 0; nCol < queryResult.nCol; nCol++)
			queryResult.headers.push_back((CString)PQfname(pDBRes, nCol));

		//  data
		//  initialize

		for (int nRow = 0; nRow < queryResult.nRow; nRow++)
		{
			queryResult.data.push_back(vector<CString>());
			for (int nCol = 0; nCol < queryResult.nCol; nCol++)
			{
				queryResult.data[nRow].push_back((CString)PQgetvalue(pDBRes, nRow, nCol));
			}
		}
	}

	//  return 
	queryResult.error = _T("");
	return true;
}

//  get database list
bool getDatabaseList(PGconn* (&pDBConn), vector<CString>& m_list, CString& sError)
{
	QueryResult res;
	CString sQuery = L"select datname from pg_database";
	if (!executeSelectQuery(pDBConn, sQuery, res))
	{
		AfxMessageBox(res.error, MB_ICONERROR);
		return false;
	}


	for (int i = 0; i < res.nRow; i++)
	{
		m_list.push_back(res.data[i][0]);
	}

	return true;
}


bool createProcessBack(CString sCMD)
{
	//	for pipe
	HANDLE hStdInPipeRead = NULL;
	HANDLE hStdInPipeWrite = NULL;
	HANDLE hStdOutPipeRead = NULL;
	HANDLE hStdOutPipeWrite = NULL;
	// Create two pipes.
	BOOL ok = TRUE;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	ok = CreatePipe(&hStdInPipeRead, &hStdInPipeWrite, &sa, 0);
	if (ok == FALSE) return -1;
	ok = CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0);
	if (ok == FALSE) return -1;

	// Create the process.
	STARTUPINFO si = { };
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdError = hStdOutPipeWrite;
	si.hStdOutput = hStdOutPipeWrite;
	si.hStdInput = hStdInPipeRead;
	PROCESS_INFORMATION pi = { };

	// Start the child process. 
	int nStrBuffer = sCMD.GetLength() + 50;
	if (!CreateProcess(
		NULL,			// No module name (use command line)
		sCMD.GetBuffer(nStrBuffer),
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		//0,              // No creation flags
		CREATE_NO_WINDOW,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return false;
	}
	sCMD.ReleaseBuffer();

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}


bool createProcess(CString sCMD)
{
	//	for pipe
	HANDLE hStdInPipeRead = NULL;
	HANDLE hStdInPipeWrite = NULL;
	HANDLE hStdOutPipeRead = NULL;
	HANDLE hStdOutPipeWrite = NULL;
	// Create two pipes.
	BOOL ok = TRUE;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	ok = CreatePipe(&hStdInPipeRead, &hStdInPipeWrite, &sa, 0);
	if (ok == FALSE) return -1;
	ok = CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0);
	if (ok == FALSE) return -1;

	// Create the process.
	STARTUPINFO si = { };
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdError = hStdOutPipeWrite;
	si.hStdOutput = hStdOutPipeWrite;
	si.hStdInput = hStdInPipeRead;
	PROCESS_INFORMATION pi = { };

	// Start the child process. 
	int nStrBuffer = sCMD.GetLength() + 50;
	if (!CreateProcess(
		NULL,			// No module name (use command line)
		sCMD.GetBuffer(nStrBuffer),
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		//0,              // No creation flags
		CREATE_NO_WINDOW,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
	)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return false;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close pipes we do not need.
	CloseHandle(hStdOutPipeWrite);
	CloseHandle(hStdInPipeRead);

	// The main loop for reading output from the DIR command.
	char buf[1024 + 1] = { };
	DWORD dwRead = 0;
	DWORD dwAvail = 0;
	ok = ReadFile(hStdOutPipeRead, buf, 1024, &dwRead, NULL);
	while (ok == TRUE)
	{
		buf[dwRead] = '\0';
		OutputDebugStringA(buf);
		puts(buf);
		ok = ReadFile(hStdOutPipeRead, buf, 1024, &dwRead, NULL);
	}

	// Clean up and exit.
	CloseHandle(hStdOutPipeRead);
	CloseHandle(hStdInPipeWrite);
	DWORD dwExitCode = 0;
	GetExitCodeProcess(pi.hProcess, &dwExitCode);
	
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

//	----------------------------------------------------------------------------------------------------------------------------------------------------------------


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMultiDatabaseBackupRestoreDlg dialog



CMultiDatabaseBackupRestoreDlg::CMultiDatabaseBackupRestoreDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MULTIDATABASEBACKUPRESTORE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMultiDatabaseBackupRestoreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDT_HOST, m_edtHost);
	DDX_Control(pDX, IDC_EDT_PORT, m_edtPort);
	DDX_Control(pDX, IDC_EDT_MAINTAINACE_DB, m_edtUser);
	DDX_Control(pDX, IDC_EDT_USER, m_edtPass);
	DDX_Control(pDX, IDC_EDT_PASS, m_edtMaintainanceDB);
	DDX_Control(pDX, IDC_LIST_DISPLAYLIST, m_listDisplay);
	DDX_Control(pDX, IDC_EDT_DUMPFILELOC, m_edtDumpFilePath);
	DDX_Control(pDX, IDC_EDT_OUTPUT_DIR, m_edtOutDir);
}

BEGIN_MESSAGE_MAP(CMultiDatabaseBackupRestoreDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMultiDatabaseBackupRestoreDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BTN_BROWSE_OUT_DIR, &CMultiDatabaseBackupRestoreDlg::OnBnClickedBtnBrowseOutDir)
	ON_BN_CLICKED(IDC_BTN_RECONNECT, &CMultiDatabaseBackupRestoreDlg::OnBnClickedBtnReconnect)
END_MESSAGE_MAP()


// CMultiDatabaseBackupRestoreDlg message handlers

BOOL CMultiDatabaseBackupRestoreDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return onInit();  // return TRUE  unless you set the focus to a control
}

void CMultiDatabaseBackupRestoreDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMultiDatabaseBackupRestoreDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMultiDatabaseBackupRestoreDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


bool CMultiDatabaseBackupRestoreDlg::onInit()
{
	//	fill values
	{
		map<CString, CString> settings;
		if (____read_server_setting(m_sSettingFilePath, settings))
		{
			//	apply settings from file
			applySettings(settings);
		}
		else
		{
			//	apply defaut settings
			applyDefaultSettings();
		}

	}

	m_listDisplay.SetExtendedStyle(
		m_listDisplay.GetExtendedStyle() 
		| LVS_EX_FLATSB 
		| LVS_EX_CHECKBOXES 
		| LVS_EX_GRIDLINES 
		| LVS_EX_FULLROWSELECT 
		| LVS_EX_AUTOCHECKSELECT 
		| LVS_EX_AUTOSIZECOLUMNS
		| LVS_EX_SNAPTOGRID
	);

	OnBnClickedBtnReconnect();

	m_listDisplay.InsertColumn(0, L"Databases", LVS_ALIGNLEFT, 200);
	m_listDisplay.InsertColumn(1, L"status.", LVS_ALIGNLEFT, 200);

	return true;
}

void CMultiDatabaseBackupRestoreDlg::OnBnClickedOk()
{
	CStdioFile backupScriptFile;

	for (int i = 0; i < m_listDisplay.GetItemCount(); i++)
	{
		if (!m_listDisplay.GetCheck(i))
			continue;

		VERIFY(m_listDisplay.SetItemText(i, 1, L"backup processing...."));
		m_listDisplay.Update(i);
		m_listDisplay.Invalidate();
		m_listDisplay.UpdateWindow();

		CString sCMD = L"";
		sCMD += L"set PGPASSWORD=" + ____get_edt_text(m_edtPass) + L"\n";
		sCMD += L"\"" + ____get_edt_text(m_edtDumpFilePath) + L"\" --file \"" + ____get_edt_text(m_edtOutDir) + L"\\" + m_listDisplay.GetItemText(i, 0) + L".sql\" --host \"" + ____get_edt_text(m_edtHost) + L"\" --port \"" + ____get_edt_text(m_edtPort) + L"\" --username \"" + ____get_edt_text(m_edtUser) + L"\" --verbose --role \"" + ____get_edt_text(m_edtUser) + L"\" --no-password --format=c --blobs --encoding \"UTF8\" \"" + m_listDisplay.GetItemText(i, 0) + L"\"";
		
		
		if (!backupScriptFile.Open(L"temp_backup_script.bat", CFile::modeCreate | CFile::modeWrite))
		{
			AfxMessageBox(L"Failed to write bat file.", MB_ICONERROR);
			return;
		}

		backupScriptFile.WriteString(sCMD);
		backupScriptFile.Close();

		if (!createProcess(L"temp_backup_script.bat"))
		{
			AfxMessageBox(L"Create Process Failed", MB_ICONERROR);
			return;
		}


		VERIFY(m_listDisplay.SetItemText(i, 1, L"backup sucessfully"));
		m_listDisplay.Update(i);
		m_listDisplay.Invalidate();
		m_listDisplay.UpdateWindow();
	}

	CFile::Remove(L"temp_backup_script.bat");

	//	store settings
	storeDlgSettingToFile();

	ShellExecuteA(NULL, "open", CStringA(____get_edt_text(m_edtOutDir)), NULL, NULL, SW_SHOWDEFAULT);

	//CDialogEx::OnOK();
}

void CMultiDatabaseBackupRestoreDlg::applyDefaultSettings()
{
	m_edtHost.SetWindowText(L"localhost");
	m_edtPort.SetWindowText(L"5440");
	m_edtUser.SetWindowText(L"postgres");
	m_edtPass.SetWindowText(L"postgres");
	m_edtMaintainanceDB.SetWindowText(L"postgres");
	m_edtDumpFilePath.SetWindowText(L"C:\\Program Files\\PostgreSQL\\15\\bin\\pg_dump.exe");
	m_edtOutDir.SetWindowText(L"");
}

void CMultiDatabaseBackupRestoreDlg::applySettings(map<CString, CString> &settings)
{
	m_edtHost.SetWindowText(settings[L"Host"]);
	m_edtPort.SetWindowText(settings[L"Port"]);
	m_edtUser.SetWindowText(settings[L"User"]);
	m_edtPass.SetWindowText(settings[L"Pass"]);
	m_edtDumpFilePath.SetWindowText(settings[L"DumpFilePath"]);
	m_edtMaintainanceDB.SetWindowText(settings[L"MaintainaceDB"]);
	m_edtOutDir.SetWindowText(settings[L"OutFolderPath"]);
	
}

bool CMultiDatabaseBackupRestoreDlg::storeDlgSettingToFile()
{
	map<CString, CString> settings;

	//	dlg settings
	{
		settings[L"Host"] = ____get_edt_text(m_edtHost);
		settings[L"Port"] = ____get_edt_text(m_edtPort);
		settings[L"User"] = ____get_edt_text(m_edtUser);
		settings[L"Pass"] = ____get_edt_text(m_edtPass);
		settings[L"MaintainaceDB"] = ____get_edt_text(m_edtMaintainanceDB);
		settings[L"DumpFilePath"] = ____get_edt_text(m_edtDumpFilePath);
		settings[L"OutFolderPath"] = ____get_edt_text(m_edtOutDir);
	}
	
	return ____save_server_setting(settings, m_sSettingFilePath);
}

void CMultiDatabaseBackupRestoreDlg::OnBnClickedBtnReconnect()
{
	CString sError = L"";
	PGconn* pDBConn= NULL;

	//	connect to database
	{
		if (!connectToDatabase(
			____get_edt_text(m_edtHost),
			____get_edt_text(m_edtPort),
			____get_edt_text(m_edtUser),
			____get_edt_text(m_edtPass),
			____get_edt_text(m_edtMaintainanceDB),
			pDBConn,
			sError
		))
		{
			AfxMessageBox(L"Error while connecting database: " + sError, MB_ICONERROR);
			return;
		}
	}


	vector<CString> sDBList;
	//	get database list and fill into list control
	{
		//	get database list
		if (!getDatabaseList(pDBConn, sDBList, sError))
		{
			AfxMessageBox(L"Error while getting database list:\n" + sError, MB_ICONERROR);
			return;
		}

		//	fill into listcontrol
		____addIntoListCtrl(m_listDisplay, sDBList, 0);
	}

}

void CMultiDatabaseBackupRestoreDlg::OnBnClickedBtnBrowseOutDir()
{
	CFolderPickerDialog dlg;
	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	m_sOutFolderPath = dlg.GetFolderPath();
	m_edtOutDir.SetWindowText(m_sOutFolderPath);

}


