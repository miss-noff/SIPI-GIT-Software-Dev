/*
    Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
    in the pylon C++ API documentation delivered with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.
    
    This sample illustrates the use of a MFC GUI together with the pylon C++ API to enumerate the attached cameras, to
    configure a camera, to start and stop the grab and to display and store grabbed images. 
    It shows how to use GUI controls to display and modify camera parameters.
*/
#pragma once

class CGuiSampleView : public CScrollView
{
protected: // create from serialization only
    CGuiSampleView();
    DECLARE_DYNCREATE(CGuiSampleView)

// Attributes
public:
    CGuiSampleDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
    virtual void OnInitialUpdate(); // called first time after construct
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view

// Implementation
public:
    virtual ~CGuiSampleView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
};

#ifndef _DEBUG  // debug version in GuiSampleView.cpp
inline CGuiSampleDoc* CGuiSampleView::GetDocument() const
   { return reinterpret_cast<CGuiSampleDoc*>(m_pDocument); }
#endif
