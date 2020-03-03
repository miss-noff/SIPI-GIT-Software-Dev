/*
    Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
    in the pylon C++ API documentation delivered with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.
    
    This sample illustrates the use of a MFC GUI together with the pylon C++ API to enumerate the attached cameras, to
    configure a camera, to start and stop the grab and to display and store grabbed images. 
    It shows how to use GUI controls to display and modify camera parameters.
*/
#include "stdafx.h"
#include "GuiSample.h"

#include "GuiSampleDoc.h"
#include "AutoPacketSizeConfiguration.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CGuiSampleDoc
IMPLEMENT_DYNCREATE(CGuiSampleDoc, CDocument)

BEGIN_MESSAGE_MAP(CGuiSampleDoc, CDocument)
    ON_COMMAND(ID_CAMERA_GRABONE, &CGuiSampleDoc::OnGrabOne)
    ON_UPDATE_COMMAND_UI(ID_CAMERA_GRABONE, &CGuiSampleDoc::OnUpdateGrabOne)
    ON_COMMAND(ID_CAMERA_STARTGRABBING, &CGuiSampleDoc::OnStartGrabbing)
    ON_UPDATE_COMMAND_UI(ID_CAMERA_STARTGRABBING, &CGuiSampleDoc::OnUpdateStartGrabbing)
    ON_COMMAND(ID_CAMERA_STOPGRAB, &CGuiSampleDoc::OnStopGrab)
    ON_UPDATE_COMMAND_UI(ID_CAMERA_STOPGRAB, &CGuiSampleDoc::OnUpdateStopGrab)
    ON_COMMAND(ID_NEW_GRABRESULT, &CGuiSampleDoc::OnNewGrabresult)
    ON_COMMAND(ID_VIEW_REFRESH, &CGuiSampleDoc::OnViewRefresh)
    ON_COMMAND(ID_FILE_SAVE_AS, &CGuiSampleDoc::OnFileSaveAs)
    ON_COMMAND(ID_FILE_IMAGE_SAVE_AS, &CGuiSampleDoc::OnFileImageSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_IMAGE_SAVE_AS, &CGuiSampleDoc::OnUpdateFileImageSaveAs)
    ON_COMMAND( ID_UPDATE_NODES, &CGuiSampleDoc::OnUpdateNodes )
END_MESSAGE_MAP()

// CGuiSampleDoc construction/destruction
CGuiSampleDoc::CGuiSampleDoc()
    : m_cntGrabbedImages(0)
    , m_cntSkippedImages(0)
    , m_cntGrabErrors(0)
    , m_hTestImage(NULL)
    , m_hGain(NULL)
    , m_hExposureTime(NULL)
{
    // TODO: add one-time construction code here
}


CGuiSampleDoc::~CGuiSampleDoc()
{
}

// Initial one-time initialization
BOOL CGuiSampleDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    try
    {
        // Register this object as an image event handler, so we will be notified of new new images
        // See Pylon::CImageEventHandler for details
        m_camera.RegisterImageEventHandler(this, Pylon::RegistrationMode_ReplaceAll, Pylon::Ownership_ExternalOwnership);
        // Register this object as a configuration event handler, so we will be notified of camera state changes.
        // See Pylon::CConfigurationEventHandler for details
        m_camera.RegisterConfiguration(this, Pylon::RegistrationMode_ReplaceAll, Pylon::Ownership_ExternalOwnership);
    }
    catch (const Pylon::GenericException& e)
    {
        TRACE(CUtf82W(e.what()));
        return FALSE;

        UNUSED(e);
    }

    return TRUE;
}


// Some features of USB cameras are of type float (i.e. Gain).
// To use the value in Windows controls, we need to convert it to an integer.
// This conversion can be done by the GenICam.
// All float features have a so-called "alias feature" representing the float value
// converted to an integer. This function retrieves the alias feature for the feature passed.
// If the feature passed is already of type integer, the function returns the
// value passed converted to IInteger.
// If the feature passed doesn't have an alias and is not an integer, the function returns an
// empty parameter.
Pylon::CIntegerParameter CGuiSampleDoc::GetIntegerParameter(GenApi::INode* pNode) const
{
    Pylon::CFloatParameter parameterFloat(pNode);
    Pylon::CIntegerParameter parameterInteger;

    // Is this a float feature?
    if (parameterFloat.IsValid())
    {
        // Get the alias.
        // If it doesn't exist, return an empty parameter.
         parameterFloat.GetAlternativeIntegerRepresentation(parameterInteger);
    }
    else
    {
        // Convert the node to an integer parameter.
        // If conversion isn't possible, return an empty parameter.
        parameterInteger.Attach(pNode);
    }

    return parameterInteger;
}

// Called when a node was possibly changed
void CGuiSampleDoc::OnNodeChanged(GenApi::INode* pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    // Uncomment the following line if you want to see which nodes are getting callbacks.
    //TRACE(_T("Node changed: %s\n"), (LPCWSTR)CUtf82W(pNode->GetName().c_str()));

    // Tell the document that some camera features must be updated.
    CWnd* pWnd = AfxGetApp()->GetMainWnd();
    // When the application shuts down the windows may already be gone.
    if (pWnd != NULL)
    {
        // You must use PostMessage here to separate the grab thread from the GUI thread.
        pWnd->PostMessage( WM_COMMAND, MAKEWPARAM( ID_UPDATE_NODES, 0 ), 0 );
    }
    
}

// CGuiSampleDoc serialization
void CGuiSampleDoc::Serialize(CArchive& ar)
{
    ASSERT(FALSE && "serialization is not supported");
}

// CGuiSampleDoc diagnostics
#ifdef _DEBUG
void CGuiSampleDoc::AssertValid() const
{
    CDocument::AssertValid();
}


void CGuiSampleDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

// This will be called by MFC before a new document is created
// or when the application is shutting down.
// Perform all cleanup here.
void CGuiSampleDoc::DeleteContents()
{
    // Make sure the device is not grabbing.
    OnStopGrab();

    // Free the grab result, if present.
    m_bitmapImage.Release();
    m_ptrGrabResult.Release();

    // Perform cleanup.
    if (m_camera.IsPylonDeviceAttached())
    {
        try
        {
            // Deregister the node callbacks.
            if (m_hExposureTime)
            {
                GenApi::Deregister( m_hExposureTime );
                m_hExposureTime = NULL;
            }
            if (m_hGain)
            {
                GenApi::Deregister( m_hGain );
                m_hGain = NULL;
            }
            if (m_hPixelFormat)
            {
                GenApi::Deregister( m_hPixelFormat );
                m_hPixelFormat = NULL;
            }
            if (m_hTestImage)
            {
                GenApi::Deregister( m_hTestImage );
                m_hTestImage = NULL;
            }

            // Clear the pointer to the features.
            m_exposureTime.Release();
            m_gain.Release();
            m_testImage.Release();
            m_pixelFormat.Release();

            // Close camera.
            // This will also stop the grab.
            m_camera.Close();

            // Free the camera.
            // This will also stop the grab and close the camera.
            m_camera.DestroyDevice();

            // Tell all the views that there is no camera anymore.
            UpdateAllViews(NULL, UpdateHint_All);
        }
        catch (const Pylon::GenericException& e)
        {
            TRACE(_T("Error during cleanup: %s"), (LPCWSTR)CUtf82W(e.what()) );
            UNUSED(e);
        }
    }

    // Call the base class.
    CDocument::DeleteContents();
}

// Enumerate devices.
void CGuiSampleDoc::OnViewRefresh()
{
    try
    {
        // Refresh the list of all attached cameras.
        theApp.EnumerateDevices();

        // Update the GUI.

        // Always update the device list and the image
        EUpdateHint hint = EUpdateHint(UpdateHint_DeviceList | UpdateHint_Image);

        if (m_camera.IsPylonDeviceAttached())
        {
            // in this SDI application the document will be reused.
            // We need to update the window title, in case the camera has been removed.
            hint = EUpdateHint(hint | UpdateHint_Feature);
        }
        else
        {
            // in this SDI application the document will be reused.
            // We need to update the window title, in case the camera has been removed.
            CString s;
            s.LoadString(AFX_IDS_UNTITLED);
            SetTitle(s);
        }

        UpdateAllViews(NULL, hint);
    }
    catch (const Pylon::GenericException& e)
    {
        TRACE(_T("Error during Refresh: %s"), (LPCWSTR)CUtf82W(e.what()));
        UNUSED(e);
    }
}

// Pylon::CImageEventHandler functions
void CGuiSampleDoc::OnImagesSkipped(Pylon::CInstantCamera& camera, size_t countOfSkippedImages)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
    ++m_cntSkippedImages;
}


void CGuiSampleDoc::OnImageGrabbed(Pylon::CInstantCamera& camera, const Pylon::CGrabResultPtr& grabResult)
{
    // NOTE: 
    // This function is called from the CInstantCamera grab thread.
    // You shouldn't perform lengthy operations here.
    // Also, you shouldn't access any UI objects, since we are not in the GUI thread.
    // We just store the grab result in the document and post a message to the CFrameWnd
    // to notify it of the new result. In response to this message, we will update the GUI.

    // The following line is commented out as this function will be called very often
    // filling up the debug output.
    //TRACE(_T("%s\n"), __FUNCTIONW__);

    // The m_ptrGrabResult will be accessed from different threads,
    // so we need to protect it with the m_MemberLock.
    CSingleLock lock(&m_MemberLock, TRUE);

    // When overwriting the current CGrabResultPtr, the old result will automatically be
    // released and reused by CInstantCamera.
    m_ptrGrabResult = grabResult;

    lock.Unlock();

    // Tell the document that there is a new image available so it can update the image window.
    CWnd* pWnd = AfxGetApp()->GetMainWnd();
    ASSERT(pWnd != NULL);
    if (pWnd != NULL)
    {
        // You must use PostMessage here to separate the grab thread from the GUI thread.
        pWnd->PostMessage(WM_COMMAND, MAKEWPARAM(ID_NEW_GRABRESULT, 0), 0);
    }
}

// Pylon::CConfigurationEventHandler functions
void CGuiSampleDoc::OnAttach(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnAttached(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnDetach(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnDetached(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnDestroy(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnDestroyed(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnOpen(Pylon::CInstantCamera& camera)
{
    Pylon::String_t strFriendlyName = camera.GetDeviceInfo().GetFriendlyName();
    TRACE(_T("%s - '%s'\n"), __FUNCTIONT__, (LPCWSTR)CUtf82W(strFriendlyName.c_str()));
}


void CGuiSampleDoc::OnOpened(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnClose(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnClosed(Pylon::CInstantCamera& camera)
{
    Pylon::String_t strFriendlyName = camera.GetDeviceInfo().GetFriendlyName();
    TRACE(_T("%s - '%s'\n"), __FUNCTIONT__, (LPCWSTR)CUtf82W(strFriendlyName.c_str()));
}


void CGuiSampleDoc::OnGrabStart(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
    
    // Reset statistics.
    m_cntGrabbedImages = 0;
    m_cntSkippedImages = 0;
    m_cntGrabErrors = 0;
}


void CGuiSampleDoc::OnGrabStarted(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnGrabStop(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnGrabStopped(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s Grabbed: %I64u; Errors: %I64u\n"), __FUNCTIONW__, m_cntGrabbedImages, m_cntGrabErrors);

    // Deregister all configurations.
    m_camera.DeregisterConfiguration(&m_singleConfiguration);
    m_camera.DeregisterConfiguration(&m_continousConfiguration);
}


void CGuiSampleDoc::OnGrabError(Pylon::CInstantCamera& camera, const char* errorMessage)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);
}


void CGuiSampleDoc::OnCameraDeviceRemoved(Pylon::CInstantCamera& camera)
{
    TRACE(_T("%s\n"), __FUNCTIONW__);

    theApp.SetDeviceFullName(NULL);

    try
    {
        // Mark everything as invalid.
        CSingleLock lock(&m_MemberLock, TRUE);
        m_ptrGrabResult.Release();
        m_bitmapImage.Release();
        m_camera.DestroyDevice();

        // Tell the document the camera and the image are gone and let it update the GUI.
        CWnd* pWnd = AfxGetApp()->GetMainWnd();
        ASSERT(pWnd != NULL);
        if (pWnd != NULL)
        {
            // Do a refresh to update the list of devices and the GUI.
            // You must use PostMessage here to separate the grab thread from the GUI thread.
            pWnd->PostMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_REFRESH, 0), 0);
        }
    }
    catch (Pylon::GenericException)
    {
    }
}


// Called from the GUI thread when there is a new grab result.
// You should update the window displaying the image.
void CGuiSampleDoc::OnNewGrabresult()
{
    // Hold a reference to the result to make sure the grab result
    // won't be deleted while we're in this function.
    Pylon::CGrabResultPtr ptr = GetGrabResultPtr();

    // First check whether the smart pointer is valid.
    // Then call GrabSucceeded on the CGrabResultData which the smart pointer references.
    if (ptr.IsValid() && ptr->GrabSucceeded())
    {
        // This is where you would do image processing
        // and other tasks.
        // Attention: If you perform lengthy operations, the GUI may become
        // unresponsive as the application doesn't process messages.

        ++m_cntGrabbedImages;

        // Convert the grab result to a dib so we can display it on the screen.
        m_bitmapImage.CopyImage(ptr);
    }
    else
    {
        ++m_cntGrabErrors;
        // If the grab result is invalid, we also mark the bitmap as invalid.
        m_bitmapImage.Release();
    }

    UpdateAllViews(NULL, UpdateHint_Image);
}


const Pylon::CGrabResultPtr CGuiSampleDoc::GetGrabResultPtr() const
{
    // We must protect this member as it will be accessed from the grab thread and the GUI thread.
    CSingleLock lock(&m_MemberLock, TRUE);
    return m_ptrGrabResult;
}


const Pylon::CPylonBitmapImage& CGuiSampleDoc::GetBitmapImage() const
{
    // No need to protect this member as it will only be accessed from the GUI thread.
    return m_bitmapImage;
}


Pylon::CIntegerParameter& CGuiSampleDoc::GetExposureTime()
{
    return m_exposureTime;
}


Pylon::CIntegerParameter& CGuiSampleDoc::GetGain()
{
    return m_gain;
}


Pylon::CEnumParameter& CGuiSampleDoc::GetTestImage()
{
    return m_testImage;
}


Pylon::CEnumParameter& CGuiSampleDoc::GetPixelFormat()
{
    return m_pixelFormat;
}


// The user clicked on single grab.
void CGuiSampleDoc::OnGrabOne()
{
    // Camera may have been disconnected.
    if (!m_camera.IsOpen() || m_camera.IsGrabbing())
    {
        return;
    }

    // Since we may switch between single and continuous shot, we must configure the camera accordingly.
    // The predefined configurations are only executed once when the camera is opened.
    // To be able to use them in our use case, we just call them explicitly to apply the configuration.
    m_singleConfiguration.OnOpened(m_camera);

    // Grab one image.
    // When the image is received, pylon will call out the OnImageGrab() handler.
    m_camera.StartGrabbing(1, Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
}


void CGuiSampleDoc::OnUpdateGrabOne(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_camera.IsOpen() && !m_camera.IsGrabbing());
}


void CGuiSampleDoc::OnUpdateStartGrabbing(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_camera.IsOpen() && !m_camera.IsGrabbing());
}


// The user clicked on continuous grab.
void CGuiSampleDoc::OnStartGrabbing()
{
    // Camera may have been disconnected.
    if (!m_camera.IsOpen() || m_camera.IsGrabbing())
    {
        return;
    }

    // Since we may switch between single and continuous shot, we must configure the camera accordingly.
    // The predefined configurations are only executed once when the camera is opened.
    // To be able to use them in our use case, we just call them explicitly to apply the configuration.
    m_continousConfiguration.OnOpened(m_camera);

    // Start grabbing until StopGrabbing() is called.
    m_camera.StartGrabbing(Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
}


// The user clicked on stop grab.
void CGuiSampleDoc::OnStopGrab()
{
    // Camera may have been disconnected.
    if (m_camera.IsGrabbing())
    {
        m_camera.StopGrabbing();
    }
}


void CGuiSampleDoc::OnUpdateStopGrab(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_camera.IsOpen() && m_camera.IsGrabbing());
}

// We overwrite the OnOpenDocument handler to open a camera device.
// In our case, lpszPathName is the full name of the device.
BOOL CGuiSampleDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // Note: lpszPathName is the full name of the camera.

    // Make sure everything is closed.
    DeleteContents();

    ASSERT(!m_camera.IsPylonDeviceAttached());

    try
    {
        // Add the AutoPacketSizeConfiguration and let pylon delete it when not needed anymore.
        m_camera.RegisterConfiguration(new CAutoPacketSizeConfiguration(), Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete);

        // Create the device and attach it to CInstantCamera.
        // Let CInstantCamera take care of destroying the device.
        const Pylon::String_t strDeviceFullName = GetString_t(lpszPathName);
        Pylon::IPylonDevice* pDevice = Pylon::CTlFactory::GetInstance().CreateDevice(strDeviceFullName);
        m_camera.Attach(pDevice, Pylon::Cleanup_Delete);

        // Open camera.
        m_camera.Open();

        // Get the Exposure Time feature.
        // On GigE cameras, the feature is named 'ExposureTimeRaw'.
        // On USB cameras, it is named 'ExposureTime'.
        m_exposureTime = GetIntegerParameter( m_camera.GetNodeMap().GetNode( "ExposureTime" ) );
        if (!m_exposureTime.IsValid())
        {
            m_exposureTime = GetIntegerParameter( m_camera.GetNodeMap().GetNode( "ExposureTimeRaw" ) );
        }
        if (m_exposureTime.IsValid())
        {
            // Add a callback that triggers the update.
            m_hExposureTime = GenApi::Register( m_exposureTime.GetNode(), *this, &CGuiSampleDoc::OnNodeChanged );
        }

        // Get the Gain feature.
        // On GigE cameras, the feature is named 'GainRaw'.
        // On USB cameras, it is named 'Gain'.
        m_gain = GetIntegerParameter( m_camera.GetNodeMap().GetNode( "Gain" ) );
        if (!m_gain.IsValid())
        {
            m_gain = GetIntegerParameter( m_camera.GetNodeMap().GetNode( "GainRaw" ) );
        }
        if (m_gain.IsValid())
        {   // Add a callback that triggers the update.
            m_hGain = GenApi::Register( m_gain.GetNode(), *this, &CGuiSampleDoc::OnNodeChanged );
        }

        // Get the Test Image Selector feature.
        m_testImage.Attach(m_camera.GetNodeMap(), "TestImageSelector");
        if (m_testImage.IsValid())
        {
            // Add a callback that triggers the update.
            m_hTestImage = GenApi::Register(m_testImage.GetNode(), *this, &CGuiSampleDoc::OnNodeChanged);
        }

        // Get the Pixel Format feature.
        m_pixelFormat.Attach(m_camera.GetNodeMap(), "PixelFormat");
        if (m_pixelFormat.IsValid())
        {
            // Add a callback that triggers the update.
            m_hPixelFormat = GenApi::Register(m_pixelFormat.GetNode(), *this, &CGuiSampleDoc::OnNodeChanged);
        }

        // Clear everything.
        CSingleLock lock(&m_MemberLock, TRUE);
        m_ptrGrabResult.Release();
        m_bitmapImage.Release();

        // Mark the document as "not modified".
        SetModifiedFlag(FALSE);

        return TRUE;
    }
    catch (const Pylon::GenericException& e)
    {
        UNUSED(e);
        TRACE(CUtf82W(e.what()));
        
        // Something went wrong. Make sure we don't leave the document object partially initialized.
        DeleteContents();
        
        return FALSE;
    }
}

// In our case, lpszPathName is the full name of the device and not an existing file.
// We provide an override to prevent the MFC implementation returning an error
// because the actual file does not exist.
void CGuiSampleDoc::SetPathName(LPCTSTR lpszPathName, BOOL /* bAddToMRU */)
{
    m_strPathName = lpszPathName;
    ASSERT(!m_strPathName.IsEmpty());
    m_bEmbedded = FALSE;
    ASSERT_VALID(this);

    SetTitle(lpszPathName);
}

// Save the image to disk.
void CGuiSampleDoc::OnFileImageSaveAs()
{
    // Make sure no one overwrites the grab result while we display the file dialog.
    // By holding a reference we make sure the grab result won't be
    // used by the CInstantCamera grab engine while doing a continuous grab.
    Pylon::CGrabResultPtr ptrGrabResult = GetGrabResultPtr();

    // We must have a valid grab result in order to save.
    if (!ptrGrabResult.IsValid())
    {
        return;
    }

    CString strFilter;
    strFilter.LoadString(IDS_FILTER_IMAGETYPES);

    // Show file dialog
    CFileDialog dlg(FALSE, NULL, _T("*.bmp"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, AfxGetMainWnd());

    if (dlg.DoModal() == IDOK)
    {
        try
        {
            // Convert the selected file type to EImageFileFormat enum.
            // Note: nFilterIndex starts at 1 from the first selected type
            Pylon::EImageFileFormat imageFileFormat = static_cast<Pylon::EImageFileFormat>(dlg.m_ofn.nFilterIndex - 1);
            if (imageFileFormat < Pylon::ImageFileFormat_Bmp || imageFileFormat > Pylon::ImageFileFormat_Raw)
            {
                throw Pylon::RuntimeException("Invalid file type", __FILE__, __LINE__);
            }

            // For demonstration purposes, we output a warning if the grab result can't be saved without conversion.
            if (!Pylon::CImagePersistence::CanSaveWithoutConversion(imageFileFormat, m_ptrGrabResult))
            {
                TRACE(_T("Warning: Image must be converted to be saved."));
            }

            CString strFullFileName = dlg.GetPathName();

            // Save the file to disk.
            // The image will be converted automatically if the target format 
            // doesn't support the bit depth or the pixel format of the grab result.
            Pylon::CImagePersistence::Save(imageFileFormat, GetString_t(strFullFileName), ptrGrabResult);
        }
        catch (const Pylon::GenericException& e)
        {
            CString strErrorMessage;
            strErrorMessage.Format(_T("Could not save image!\n\n(%s)"), (LPCWSTR)CUtf82W(e.what()));

            AfxMessageBox(strErrorMessage, MB_OK | MB_ICONSTOP);
        }
    }
}


void CGuiSampleDoc::OnUpdateFileImageSaveAs(CCmdUI *pCmdUI)
{
    // We can only save if we have a valid grab result.
    pCmdUI->Enable(m_ptrGrabResult.IsValid());
}

// Called when a camera feature changes its properties.
void CGuiSampleDoc::OnUpdateNodes()
{
    // Check the camera. It may have been removed.
    if (m_camera.IsPylonDeviceAttached())
    {
        UpdateAllViews(NULL, UpdateHint_Feature);
    }
}
