#pragma once

#include "wx/wx.h"

#include "ACLabel.hpp"
#include "ACButton.hpp"
#include "GUI_Utils.hpp"

using namespace Slic3r::GUI;

class ACDialogTopbar : public wxWindow
{
public:
    ACDialogTopbar(DPIFrame* parent, int toolbarW, int toolbarH = 62, const wxString& title = wxString("Configuration Manage"));
    ~ACDialogTopbar() = default;

    void Init(DPIFrame* parent);

    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMotion(wxMouseEvent& event);

    void SetTitle(wxString title);

public:
    void OnClose(wxEvent &event);
private:
    wxString    m_title;
    DPIFrame*  m_frame;
    ACLabel  *  m_title_item;
    ACButton *  m_close_button;
    int         m_toolbar_w;
    int         m_toolbar_h;
    wxPoint m_delta;
    wxRect m_normalRect;

    //DECLARE_EVENT_TABLE()
};
