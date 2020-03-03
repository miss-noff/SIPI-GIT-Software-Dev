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
#include "GuiSampleView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CGuiSampleView
IMPLEMENT_DYNCREATE(CGuiSampleView, CScrollView)

BEGIN_MESSAGE_MAP(CGuiSampleView, CScrollView)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// CGuiSampleView construction/destruction
CGuiSampleView::CGuiSampleView()
{
    // TODO: add construction code here

}


CGuiSampleView::~CGuiSampleView()
{
}


BOOL CGuiSampleView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CScrollView::PreCreateWindow(cs);
}


void CGuiSampleView::OnInitialUpdate()
{
    CScrollView::OnInitialUpdate();
}


void CGuiSampleView::OnDraw(CDC* pDC)
{
    CGuiSampleDoc* pDoc = GetDocument();
    ASSERT(pDoc != NULL);

    const Pylon::CPylonBitmapImage pylonBitmapImage = pDoc->GetBitmapImage();

    // Use pixel coordinates.
    int oldMapMode = pDC->SetMapMode(MM_TEXT);
    const COLORREF colorBackground = RGB(0, 0, 0);

    CRect rectClient;
    this->GetClientRect(&rectClient);

    if (pylonBitmapImage.IsValid())
    {
        // Create a DC to copy the bitmap from.
        CDC dcBitmap;
        dcBitmap.CreateCompatibleDC(pDC);

        HGDIOBJ oldhBmp = dcBitmap.SelectObject((HGDIOBJ)(HBITMAP)pylonBitmapImage);

        // Copy bitmap in real size (100% zoom).
        CRect rectSrc(0, 0, pylonBitmapImage.GetWidth(), pylonBitmapImage.GetHeight());
        CRect rectDest(rectSrc);

        BOOLEAN isSmaller = rectDest.Width() < rectSrc.Width() || rectDest.Height() < rectSrc.Height();
        int oldStreachBltMode;
        if (isSmaller)
        {
            // When the image is scaled down, we set the stretch mode to reduce the number of scaling artefacts.
            oldStreachBltMode = pDC->SetStretchBltMode(COLORONCOLOR);
        }

        // Paint the image onto the screen.
        pDC->StretchBlt(rectDest.left, rectDest.top, rectSrc.Width(), rectSrc.Height(),
                        &dcBitmap,
                        rectSrc.left, rectSrc.top, rectSrc.Width(), rectSrc.Height(), SRCCOPY);

        // Restore the state of the DC.
        if (isSmaller)
        {
            pDC->SetStretchBltMode(oldStreachBltMode);
        }

        dcBitmap.SelectObject(oldhBmp);

        // Fill the remaining space on the right.
        if (rectDest.Width() < rectClient.Width())
        {
            CRect rectFill(rectClient);
            rectFill.left = rectDest.right;
            rectFill.bottom = rectDest.bottom;

            pDC->FillSolidRect(&rectFill, colorBackground);
        }

        // Fill the remaining space on the bottom.
        if (rectDest.Height() < rectClient.Height())
        {
            CRect rectFill(rectClient);
            rectFill.top = rectDest.bottom;

            pDC->FillSolidRect(&rectFill, colorBackground);
        }
    }
    else
    {
        // No image available. Fill the entire screen.
        pDC->FillSolidRect(&rectClient, colorBackground);
    }

    pDC->SetMapMode(oldMapMode);
}

// CGuiSampleView diagnostics
#ifdef _DEBUG
void CGuiSampleView::AssertValid() const
{
    CScrollView::AssertValid();
}


void CGuiSampleView::Dump(CDumpContext& dc) const
{
    CScrollView::Dump(dc);
}


CGuiSampleDoc* CGuiSampleView::GetDocument() const // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGuiSampleDoc)));
    return (CGuiSampleDoc*)m_pDocument;
}
#endif //_DEBUG


BOOL CGuiSampleView::OnEraseBkgnd(CDC* pDC)
{
    // OnDraw paints the entire window. We don't want the system to erase the background
    // as this will induce flicker. So we return true and don't call the base class.
    return TRUE;
}


void CGuiSampleView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    EUpdateHint eHint = (EUpdateHint)lHint;
    if (eHint == UpdateHint_All || (eHint & UpdateHint_Image))
    {
        CGuiSampleDoc* pDoc = GetDocument();
        ASSERT(pDoc != NULL);

        const Pylon::CPylonBitmapImage pylonBitmapImage = pDoc->GetBitmapImage();

        CSize sizeTotal;
        sizeTotal.cx = sizeTotal.cy = 1;

        if (pylonBitmapImage.IsValid())
        {
            sizeTotal.cx = pylonBitmapImage.GetWidth();
            sizeTotal.cy = pylonBitmapImage.GetHeight();
        }

        SetScrollSizes(MM_TEXT, sizeTotal);

        // Repaint this view.
        // This will invoke OnDraw().
        Invalidate();
    }
}
