#include "Notebook.hpp"

#ifdef _WIN32

#include "GUI_App.hpp"
#include "wxExtensions.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include "ACDefines.h"
#include "ACButton.hpp"

wxDEFINE_EVENT(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED, wxCommandEvent);

ButtonsListCtrl::ButtonsListCtrl(wxWindow *parent, bool add_mode_buttons/* = false*/) :
    wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__
        
    //int em = em_unit(this);// Slic3r::GUI::wxGetApp().em_unit();
    //m_btn_margin  = std::lround(0.3 * em);
    //m_line_margin = std::lround(0.1 * em);
    m_btn_margin  = 0;
    m_line_margin = 0;

    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(m_sizer);

    m_sizer->AddStretchSpacer(1);

    m_buttons_sizer = new wxFlexGridSizer(1, m_btn_margin, m_btn_margin);
    m_sizer->Add(m_buttons_sizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM, m_btn_margin);
   
    m_sizer->AddStretchSpacer(1);

    if (add_mode_buttons) {
        m_mode_sizer = new ModeSizer(this, m_btn_margin);
        m_sizer->AddStretchSpacer(20);
        m_sizer->Add(m_mode_sizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, m_btn_margin);
    }

    this->Bind(wxEVT_PAINT, &ButtonsListCtrl::OnPaint, this);
}

void ButtonsListCtrl::OnPaint(wxPaintEvent&)
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(this);
    const wxSize sz = GetSize();
    wxPaintDC dc(this);

    // draw background
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(AC_COLOR_LIGHTBLUE); // AC light blue
    dc.DrawRectangle(0,0, sz.x, sz.y);

    if (m_selection < 0 || m_selection >= (int)m_pageButtons.size())
        return;

    //const wxColour& selected_btn_bg  = Slic3r::GUI::wxGetApp().get_color_selected_btn_bg();
    //const wxColour& default_btn_bg   = Slic3r::GUI::wxGetApp().get_highlight_default_clr();
    const wxColour& btn_marker_color = Slic3r::GUI::wxGetApp().get_color_hovered_btn_label();

    wxColour        selected_btn_bg(wxColor(255, 255, 255));
    wxColour        default_btn_bg(wxColor(204, 224, 255)); // Gradient #414B4E

    // highlight selected notebook button

    for (int idx = 0; idx < int(m_pageButtons.size()); idx++) {
        ACButton *btn = m_pageButtons[idx];

        //btn->SetBackgroundColour(idx == m_selection ? selected_btn_bg : default_btn_bg);

        //wxPoint pos = btn->GetPosition();
        //wxSize size = btn->GetSize();
        //const wxColour& clr = idx == m_selection ? btn_marker_color : default_btn_bg;
        //dc.SetPen(clr);
        //dc.SetBrush(clr);
        //dc.DrawRectangle(pos.x, pos.y + size.y, size.x, sz.y - size.y);

        //dc.SetPen(wxColour(255,0,0));
        //dc.SetBrush(wxColour(100, 0, 0));
        //dc.DrawRectangle(pos.x, pos.y, size.x, size.y);
    }

    // highlight selected mode button

    if (m_mode_sizer) {
        const std::vector<ModeButton*>& mode_btns = m_mode_sizer->get_btns();
        for (int idx = 0; idx < int(mode_btns.size()); idx++) {
            ModeButton* btn = mode_btns[idx];
            btn->SetBackgroundColour(btn->is_selected() ? selected_btn_bg : default_btn_bg);

            //wxPoint pos = btn->GetPosition();
            //wxSize size = btn->GetSize();
            //const wxColour& clr = btn->is_selected() ? btn_marker_color : default_btn_bg;
            //dc.SetPen(clr);
            //dc.SetBrush(clr);
            //dc.DrawRectangle(pos.x, pos.y + size.y, size.x, sz.y - size.y);
        }
    }

    // Draw orange bottom line

    //dc.SetPen(btn_marker_color);
    //dc.SetBrush(btn_marker_color);
    //dc.DrawRectangle(1, sz.y - m_line_margin, sz.x, m_line_margin);


}

void ButtonsListCtrl::UpdateMode()
{
    if (m_mode_sizer)
        m_mode_sizer->SetMode(Slic3r::GUI::wxGetApp().get_mode());
}

void ButtonsListCtrl::Rescale()
{
    if (m_mode_sizer)
        m_mode_sizer->msw_rescale();

    for (ACButton* btn : m_pageButtons) 
        btn->Rescale();

    // no gap
    //int em = em_unit(this);
    //m_btn_margin = std::lround(0.3 * em);
    //m_line_margin = std::lround(0.1 * em);
    //m_buttons_sizer->SetVGap(m_btn_margin);
    //m_buttons_sizer->SetHGap(m_btn_margin);

    m_sizer->Layout();
}

void ButtonsListCtrl::SetSelection(int sel)
{
    if (m_selection == sel)
        return;
    if (m_selection >= 0) {
        m_pageButtons[m_selection]->SetSelected(false);
        ACStateColor bg_color = ACStateColor( std::pair{wxColour(204, 224, 255), (int) ACStateColor::Normal} );
        m_pageButtons[m_selection]->SetBackgroundColor(bg_color);
    }

    m_selection = sel;

    if (m_selection >= 0) {
        m_pageButtons[m_selection]->SetSelected(true);
        ACStateColor bg_color = ACStateColor( std::pair{wxColour(255, 255, 255), (int) ACStateColor::Normal} );
        m_pageButtons[m_selection]->SetBackgroundColor(bg_color);
    }
    Refresh();
}

bool ButtonsListCtrl::InsertPage(size_t             n,
                                 const wxString &   text,
                                 bool               bSelect /* = false*/,
                                 const std::string &bmp_name /* = ""*/,
                                 const std::string &inactive_bmp_name)
{
    ACButton *btn = new ACButton(this, text.empty() ? text : " " + text, bmp_name, wxNO_BORDER, FromDIP(18));
    
    btn->SetCornerRadius(FromDIP(10), ACButton::CornerTop);
    btn->SetBackgroundColour(wxColour(204, 224, 255)); // ac light blue
    btn->SetPaddingSize(wxSize(FromDIP(25), FromDIP(11)));
    btn->SetAlignLeft(false);

    int em = em_unit(this);
    // BBS set size for button
    //btn->SetMinSize({(text.empty() ? 40 : 136) * em / 10, 36 * em / 10});

    ACStateColor bg_color = ACStateColor(
        std::pair{wxColour(204, 224, 255), (int) ACStateColor::Normal});
    btn->SetBackgroundColor(bg_color);

    ACStateColor text_color = ACStateColor(
        std::pair{wxColour(29, 105, 224), (int) ACStateColor::Pressed},
        std::pair{wxColour(57, 134, 255), (int) ACStateColor::Hovered},
        std::pair{wxColour(62, 81, 116), (int) ACStateColor::Normal}
        );
    btn->SetTextColor(text_color);

    btn->SetInactiveIcon(inactive_bmp_name.empty() ? bmp_name : inactive_bmp_name);
    btn->SetSelected(false);

    btn->Bind(wxEVT_BUTTON, [this, btn](wxCommandEvent& event) {
        if (auto it = std::find(m_pageButtons.begin(), m_pageButtons.end(), btn); it != m_pageButtons.end()) {
            auto sel = it - m_pageButtons.begin();
            //do it later
            //SetSelection(sel);
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
            evt.SetId(sel);
            wxPostEvent(this->GetParent(), evt);
            Refresh();
        }
    });

    //Slic3r::GUI::wxGetApp().UpdateDarkUI(btn);

    m_pageButtons.insert(m_pageButtons.begin() + n, btn);
    m_buttons_sizer->Insert(n, new wxSizerItem(btn));
    m_buttons_sizer->SetCols(m_buttons_sizer->GetCols() + 1);
    m_sizer->Layout();
    return true;
}

void ButtonsListCtrl::RemovePage(size_t n)
{
    ACButton* btn = m_pageButtons[n];
    m_pageButtons.erase(m_pageButtons.begin() + n);
    m_buttons_sizer->Remove(n);
#if __WXOSX__
    RemoveChild(btn);
#else
    btn->Reparent(nullptr);
#endif
    btn->Destroy();
    m_sizer->Layout();
}

bool ButtonsListCtrl::SetPageImage(size_t n, const std::string& bmp_name) const
{
    if (n >= m_pageButtons.size())
        return false;
     //return m_pageButtons[n]->SetBitmap_(bmp_name);

     ScalableBitmap bitmap(NULL, bmp_name);
     // m_pageButtons[n]->SetBitmap_(bitmap);
     return true;
}

void ButtonsListCtrl::SetPageText(size_t n, const wxString& strText)
{
    ACButton* btn = m_pageButtons[n];
    btn->SetLabel(strText);
}

wxString ButtonsListCtrl::GetPageText(size_t n) const
{
    ACButton* btn = m_pageButtons[n];
    return btn->GetLabel();
}

#endif // _WIN32


