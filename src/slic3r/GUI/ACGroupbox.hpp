/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/statbox.h
// Purpose:     ACGroupBox class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_ACGROUPBOX_H_
#define _WX_MSW_ACGROUPBOX_H_

#include "wx/compositewin.h"
#include "wx/statbox.h"


// Group box
class ACGroupBox : public wxCompositeWindowSettersOnly<wxStaticBoxBase>
{
public:
    ACGroupBox()
        : wxCompositeWindowSettersOnly<wxStaticBoxBase>()
    {
    }

    ACGroupBox(wxWindow *parent, wxWindowID id,
                const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxASCII_STR(wxStaticBoxNameStr))
        : wxCompositeWindowSettersOnly<wxStaticBoxBase>()
    {
        Create(parent, id, label, pos, size, style, name);
    }

    ACGroupBox(wxWindow* parent, wxWindowID id,
                wxWindow* label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString &name = wxASCII_STR(wxStaticBoxNameStr))
        : wxCompositeWindowSettersOnly<wxStaticBoxBase>()
    {
        Create(parent, id, label, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id,
                const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxASCII_STR(wxStaticBoxNameStr));

    bool Create(wxWindow *parent, wxWindowID id,
                wxWindow* label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxASCII_STR(wxStaticBoxNameStr));

    /// Implementation only
    virtual void GetBordersForSizer(int *borderTop, int *borderOther) const wxOVERRIDE;

    virtual bool SetBackgroundColour(const wxColour& colour) wxOVERRIDE;
    virtual bool SetFont(const wxFont& font) wxOVERRIDE;

    virtual WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const wxOVERRIDE;

    // returns true if the platform should explicitly apply a theme border
    virtual bool CanApplyThemeBorder() const wxOVERRIDE { return false; }

protected:
    virtual wxSize DoGetBestSize() const wxOVERRIDE;

public:
    //virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) wxOVERRIDE;

protected:
    virtual wxWindowList GetCompositeWindowParts() const wxOVERRIDE;

    //// return the region with all the windows inside this static box excluded
    //virtual WXHRGN MSWGetRegionWithoutChildren();

    //// remove the parts which are painted by static box itself from the given
    //// region which is embedded in a rectangle (0, 0)-(w, h)
    //virtual void MSWGetRegionWithoutSelf(WXHRGN hrgn, int w, int h);

    // paint the given rectangle with our background brush/colour
    virtual void PaintBackground(wxDC& dc, const struct tagRECT& rc);
    // paint the foreground of the static box
    virtual void PaintForeground(wxDC& dc, const struct tagRECT& rc);

    void OnPaint();

private:
    void PositionLabelWindow();

    //wxDECLARE_DYNAMIC_CLASS_NO_COPY(ACGroupBox);
};

// Indicate that we have the ctor overload taking wxWindow as label.
#define wxHAS_WINDOW_LABEL_IN_STATIC_BOX


class ACGroupBoxSizer: public wxBoxSizer
{
public:
    ACGroupBoxSizer(ACGroupBox *box, int orient);
    ACGroupBoxSizer(int orient, wxWindow *win, const wxString& label = wxEmptyString);
    virtual ~ACGroupBoxSizer();

    virtual wxSize CalcMin() wxOVERRIDE;
    virtual void RepositionChildren(const wxSize& minSize) wxOVERRIDE;

    ACGroupBox *GetGroupBox() const
        { return m_groupBox; }

    // override to hide/show the static box as well
    virtual void ShowItems (bool show) wxOVERRIDE;
    virtual bool AreAnyItemsShown() const wxOVERRIDE;

    virtual bool Detach( wxWindow *window ) wxOVERRIDE;
    virtual bool Detach( wxSizer *sizer ) wxOVERRIDE { return wxBoxSizer::Detach(sizer); }
    virtual bool Detach( int index ) wxOVERRIDE { return wxBoxSizer::Detach(index); }

protected:
    ACGroupBox   *m_groupBox;

private:
    wxDECLARE_CLASS(ACGroupBoxSizer);
    wxDECLARE_NO_COPY_CLASS(ACGroupBoxSizer);
};

#endif // _WX_MSW_ACGROUPBOX_H_

