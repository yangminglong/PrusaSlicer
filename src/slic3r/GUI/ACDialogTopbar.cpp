#include "ACDialogTopbar.hpp"
#include "wx/artprov.h"
#include "wx/aui/framemanager.h"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "GUI.hpp"
#include "wxExtensions.hpp"
#include "Plater.hpp"
#include "MainFrame.hpp"
#include "wx/dc.h"
#include "wx/dcgraph.h"
//#include "WebViewDialog.hpp"
//#include "PartPlate.hpp"


using namespace Slic3r;

ACDialogTopbar::ACDialogTopbar(DPIFrame* parent, int toolbarW, int toolbarH, const wxString& title) 
    : wxWindow(parent, ID_DIALOG_TOP_BAR, wxDefaultPosition, wxDefaultSize)
    , m_title(title)
    , m_toolbar_w(toolbarW)
    , m_toolbar_h(toolbarH)
{ 
    Init(parent);
}

void ACDialogTopbar::Init(DPIFrame* parent) 
{
    m_frame = parent;

    SetBackgroundColour(AC_COLOR_LIGHTBLUE);

    ACStateColor background_color = ACStateColor(
        std::make_pair(AC_COLOR_MAIN_BLUE, (int) ACStateColor::Checked),
        std::make_pair(AC_COLOR_BG_LIGHTGRAY, (int) ACStateColor::Hovered), 
        std::make_pair(wxTransparentColor, (int) ACStateColor::Normal));

    ACStateColor color();

    int buttonSize = 20;
    int buttonIconSize = 14;
    int buttonPaddingSize = (buttonSize-buttonIconSize)/2;

    m_title_item = new ACLabel(this, m_title, wxALIGN_CENTRE);
    m_title_item->SetFont(ACLabel::Head_16);
    wxSize titleTextSize = m_title_item->GetTextSize();

    m_close_button = new ACButton(this, "", "topbar_close", wxBORDER_NONE, FromDIP(buttonIconSize));
    m_close_button->SetPaddingSize(wxSize(FromDIP(buttonPaddingSize), FromDIP(buttonPaddingSize)));
    m_close_button->SetAlignLeft(false);
    m_close_button->SetBackgroundColor(background_color);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddStretchSpacer(1);
    mainSizer->Add(m_title_item, wxEXPAND|wxTOP|wxBOTTOM, (m_toolbar_h-titleTextSize.y)/2);
    mainSizer->Add(m_close_button, 1, wxTOP|wxBOTTOM|wxRIGHT, (m_toolbar_h-buttonSize)/2);
    SetSizer(mainSizer);
    Fit();

    this->SetSize(m_toolbar_w, m_toolbar_h);

    this->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ACDialogTopbar::OnClose, this, m_close_button->GetId());

    mainSizer->Layout();
    this->Layout();
}

void ACDialogTopbar::SetTitle(wxString title)
{
    //wxGCDC dc(this);
    //title = wxControl::Ellipsize(title, dc, wxELLIPSIZE_END, FromDIP(TOPBAR_TITLE_WIDTH));

    m_title_item->SetLabel(title);

    this->Refresh();
}


void ACDialogTopbar::OnClose(wxEvent& event) 
{ 
    m_frame->Close();
}

void ACDialogTopbar::OnMouseLeftDown(wxMouseEvent& event)
{
    wxPoint mouse_pos = ::wxGetMousePosition();
    wxPoint frame_pos = m_frame->GetScreenPosition();
    static wxPoint m_delta;
    m_delta = mouse_pos - frame_pos;

    bool isAtbuttonPos = m_close_button != nullptr && m_close_button->GetRect().Contains(event.GetPosition()) ? true : false;

    if (isAtbuttonPos)
    {
        CaptureMouse();
#ifdef __WXMSW__
        ReleaseMouse();
        ::PostMessage((HWND) m_frame->GetHandle(), WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(mouse_pos.x, mouse_pos.y));
        return;
#endif //  __WXMSW__
    }
    
    event.Skip();
}

void ACDialogTopbar::OnMouseLeftUp(wxMouseEvent& event)
{
    wxPoint mouse_pos = ::wxGetMousePosition();
    if (HasCapture())
    {
        ReleaseMouse();
    }

    event.Skip();
}

void ACDialogTopbar::OnMouseMotion(wxMouseEvent& event)
{
    wxPoint mouse_pos = ::wxGetMousePosition();

    if (!HasCapture()) {
        //m_frame->OnMouseMotion(event);

        event.Skip();
        return;
    }

    if (event.Dragging() && event.LeftIsDown())
    {
        // leave max state and adjust position 
        if (m_frame->IsMaximized()) {
            wxRect rect = m_frame->GetRect();
            // Filter unexcept mouse move
            if (m_delta + rect.GetLeftTop() != mouse_pos) {
                m_delta = mouse_pos - rect.GetLeftTop();
                m_delta.x = m_delta.x * m_normalRect.width / rect.width;
                m_delta.y = m_delta.y * m_normalRect.height / rect.height;
                m_frame->Restore();
            }
        }
        m_frame->Move(mouse_pos - m_delta);
    }
    event.Skip();
}



