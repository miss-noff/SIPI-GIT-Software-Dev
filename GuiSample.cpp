/*
    Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
    in the pylon C++ API documentation delivered with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.
    
    Defines the class behaviors for the application.
*/
#include "stdafx.h"
#include "GuiSample.h"
#include "MainFrm.h"

#include "GuiSampleDoc.h"
#include "ConfigView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CGuiSampleApp
BEGIN_MESSAGE_MAP(CGuiSampleApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, &CGuiSampleApp::OnAppAbout)
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
    ON_COMMAND(ID_OPEN_CAMERA, &CGuiSampleApp::OnOpenCamera)
END_MESSAGE_MAP()

// CGuiSampleApp construction
CGuiSampleApp::CGuiSampleApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

// The one and only CGuiSampleApp object
CGuiSampleApp theApp;

// CGuiSampleApp initialization
BOOL CGuiSampleApp::InitInstance()
{
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need
    // Change the registry key under which our settings are stored
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization
    SetRegistryKey(_T("Basler\\pylon\\Samples"));
    LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

    // Before using any pylon methods, the pylon runtime must be initialized.
    Pylon::PylonInitialize();

    // Get list of all attached devices.
    EnumerateDevices();
    
    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views
    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CGuiSampleDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CConfigView));
    if (!pDocTemplate)
        return FALSE;
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line.  Will return FALSE if
    // app was launched with /RegServer, /Register, /Unregserver or /Unregister.
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // The one and only window has been initialized, so show and update it
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();
    // call DragAcceptFiles only if there's a suffix
    //  In an SDI app, this should occur after ProcessShellCommand
    return TRUE;
}


int CGuiSampleApp::ExitInstance()
{
    // Releases all pylon resources.
    Pylon::PylonTerminate();

    return CWinApp::ExitInstance();
}

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}


void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CGuiSampleApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

// CGuiSampleApp message handlers
const Pylon::DeviceInfoList_t& CGuiSampleApp::GetDeviceInfoList() const
{
    return m_devices;
}

// Get list of all attached devices.
int CGuiSampleApp::EnumerateDevices()
{
    Pylon::DeviceInfoList_t devices;
    try
    {
        // Get the transport layer factory.
        Pylon::CTlFactory& TlFactory = Pylon::CTlFactory::GetInstance();

        // Get all attached cameras.
        TlFactory.EnumerateDevices(devices);
    }
    catch (const Pylon::GenericException& e)
    {
        UNUSED(e);
        devices.clear();

        TRACE(CUtf82W(e.what()));
    }

    m_devices = devices;

    // When calling this function, make sure to update the device list control,
    // because its items store pointers to elements in the m_devices list.
    return (int)m_devices.size();
}

// Will be called to pass the full name of the camera selected in the device list control.
CString CGuiSampleApp::SetDeviceFullName(LPCTSTR lpszFullDeviceName)
{
    CString oldValue = m_strDeviceFullName;
    m_strDeviceFullName = lpszFullDeviceName;
    return oldValue;
}


void CGuiSampleApp::OnOpenCamera()
{
    // Do nothing if there is no camera.
    if (m_strDeviceFullName.IsEmpty())
    {
        return;
    }

    // We only have one doc template. Get a pointer to it.
    POSITION pos = GetFirstDocTemplatePosition();
    CDocTemplate* pDocTemplate = GetNextDocTemplate(pos);

    // Open the document and use the full name of the device as the filename.
    CDocument* pNewDoc = pDocTemplate->OpenDocumentFile(m_strDeviceFullName, TRUE);
    if (pNewDoc == NULL)
    {
        CString strErrorMessage;
        strErrorMessage.Format(_T("Could not open camera \"%s\""), m_strDeviceFullName);
        AfxMessageBox(strErrorMessage);
    }
}
