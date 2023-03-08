#include "ACTextInput.hpp"
#include "ACLabel.hpp"

#include <wx/dcgraph.h>
#include "ACDefines.h"

BEGIN_EVENT_TABLE(ACTextInput, ACStaticBox)

EVT_PAINT(ACTextInput::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

ACTextInput::ACTextInput()
    : label_color(
        std::make_pair(AC_COLOR_TEXTINPUT_FG_DIS, (int) ACStateColor::Disabled), 
        std::make_pair(AC_COLOR_TEXTINPUT_FG_HOV, (int) ACStateColor::Hovered),
        std::make_pair(AC_COLOR_TEXTINPUT_FG_NOR, (int) ACStateColor::Normal)
    ) , text_color(             
        std::make_pair(AC_COLOR_TEXTINPUT_FG_DIS, (int) ACStateColor::Disabled), 
        std::make_pair(AC_COLOR_TEXTINPUT_FG_HOV, (int) ACStateColor::Hovered),
        std::make_pair(AC_COLOR_TEXTINPUT_FG_NOR, (int) ACStateColor::Normal)
    )
{
    radius = 0;
    border_width = 1;
    border_color = ACStateColor(std::make_pair(AC_COLOR_TEXTINPUT_BD_DIS, (int) ACStateColor::Disabled)
                              , std::make_pair(AC_COLOR_TEXTINPUT_BD_HOV, (int) ACStateColor::Hovered)
                              , std::make_pair(AC_COLOR_TEXTINPUT_BD_NOR, (int) ACStateColor::Normal));
    background_color = ACStateColor(std::make_pair(AC_COLOR_TEXTINPUT_BG_DIS, (int) ACStateColor::Disabled)
                                  , std::make_pair(AC_COLOR_TEXTINPUT_BG_HOV, (int) ACStateColor::Hovered)
                                  , std::make_pair(AC_COLOR_TEXTINPUT_BG_NOR, (int) ACStateColor::Normal));
    SetFont(ACLabel::Body_12);
}

ACTextInput::ACTextInput(wxWindow *     parent,
                     wxString       text,
                     wxString       label,
                     wxString       icon,
                     const wxPoint &pos,
                     const wxSize & size,
                     long           style)
    : ACTextInput()
{
    Create(parent, text, label, icon, pos, size, style);
}

void ACTextInput::Create(wxWindow *     parent,
                       wxString       text,
                       wxString       label,
                       wxString       icon,
                       const wxPoint &pos,
                       const wxSize & size,
                       long           style)
{
    text_ctrl = nullptr;
    ACStaticBox::Create(parent, wxID_ANY, pos, size, style);
    wxWindow::SetLabel(label);
    style &= ~wxRIGHT;
    state_handler.attach({&label_color, & text_color});
    state_handler.update_binds();
    text_ctrl = new wxTextCtrl(this, wxID_ANY, text, {4, 4}, wxDefaultSize, style | wxBORDER_NONE | wxTE_PROCESS_ENTER);
    text_ctrl->SetFont(ACLabel::Body_14);
    text_ctrl->SetInitialSize(text_ctrl->GetBestSize());
    text_ctrl->SetBackgroundColour(background_color.colorForStates(state_handler.states()));
    text_ctrl->SetForegroundColour(text_color.colorForStates(state_handler.states()));
    state_handler.attach_child(text_ctrl);
    text_ctrl->Bind(wxEVT_KILL_FOCUS, [this](auto &e) {
        OnEdit();
        e.SetId(GetId());
        ProcessEventLocally(e);
        e.Skip();
    });
    text_ctrl->Bind(wxEVT_TEXT_ENTER, [this](auto &e) {
        OnEdit();
        e.SetId(GetId());
        ProcessEventLocally(e);
    });
    text_ctrl->Bind(wxEVT_RIGHT_DOWN, [this](auto &e) {
    }); // disable context menu
    
    if (!icon.IsEmpty()) {
        this->icon = ScalableBitmap(this, icon.ToStdString(), 16);
    }
    messureSize();
}

void ACTextInput::SetCornerRadius(double radius)
{
    this->radius = radius;
    Refresh();
}

void ACTextInput::SetLabel(const wxString& label)
{
    wxWindow::SetLabel(label);
    messureSize();
    Refresh();
}

void ACTextInput::SetIcon(const wxBitmap &icon)
{
    this->icon.bmp() = icon;
    Rescale();
}

void ACTextInput::SetLabelColor(ACStateColor const &color)
{
    label_color = color;
    state_handler.update_binds();
}

void ACTextInput::SetTextColor(ACStateColor const& color)
{
    text_color= color;
    state_handler.update_binds();
}

void ACTextInput::Rescale()
{
    if (!this->icon.name().empty())
        this->icon.msw_rescale();
    messureSize();
    Refresh();
}

bool ACTextInput::Enable(bool enable)
{
    bool result = /*text_ctrl->Enable(enable) && */wxWindow::Enable(enable);
    if (result) {
        wxCommandEvent e(EVT_ENABLE_CHANGED);
        e.SetEventObject(this);
        GetEventHandler()->ProcessEvent(e);
        wxColour disColor = background_color.colorForStates(state_handler.states());
        wxColour texColor = text_color.colorForStates(state_handler.states());


        text_ctrl->SetBackgroundColour(disColor);
        text_ctrl->SetForegroundColour(texColor);
    }
    return result;
}

void ACTextInput::SetMinSize(const wxSize& size)
{
    wxSize size2 = size;
    if (size2.y < 0) {
#ifdef __WXMAC__
        if (GetPeer()) // peer is not ready in Create on mac
#endif
        size2.y = GetSize().y;
    }
    wxWindow::SetMinSize(size2);
}

void ACTextInput::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
    if (sizeFlags & wxSIZE_USE_EXISTING) return;
    wxSize size = GetSize();
    wxPoint textPos = {5, 0};
    if (this->icon.bmp().IsOk()) {
        wxSize szIcon = this->icon.GetBmpSize();
        textPos.x += szIcon.x;
    }
    bool align_right = GetWindowStyle() & wxRIGHT;
    if (align_right)
        textPos.x += labelSize.x;
    if (text_ctrl) {
        wxSize textSize = text_ctrl->GetSize();
        textSize.x = size.x - textPos.x - labelSize.x - 10;
        text_ctrl->SetSize(textSize);
        text_ctrl->SetPosition({textPos.x, (size.y - textSize.y) / 2});
    }
}

void ACTextInput::DoSetToolTipText(wxString const &tip)
{
    wxWindow::DoSetToolTipText(tip);
    text_ctrl->SetToolTip(tip);
}

void ACTextInput::paintEvent(wxPaintEvent &evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void ACTextInput::render(wxDC& dc)
{
    ACStaticBox::render(dc);
    int states = state_handler.states();
    wxSize size = GetSize();
    bool   align_right = GetWindowStyle() & wxRIGHT;
    // start draw
    wxPoint pt = {5, 0};
    if (icon.bmp().IsOk()) {
        wxSize szIcon = icon.GetBmpSize();
        pt.y = (size.y - szIcon.y) / 2;
        dc.DrawBitmap(icon.bmp(), pt);
        pt.x += szIcon.x + 0;
    }
    auto text = wxWindow::GetLabel();
    if (!text.IsEmpty()) {
        wxSize textSize = text_ctrl->GetSize();
        if (align_right) {
            if (pt.x + labelSize.x > size.x)
                text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
            pt.y = (size.y - labelSize.y) / 2;
        } else {
            pt.x += textSize.x;
            pt.y = (size.y + textSize.y) / 2 - labelSize.y;
        }
        dc.SetTextForeground(label_color.colorForStates(states));
        dc.SetFont(GetFont());
        dc.DrawText(text, pt);
    }
}

void ACTextInput::messureSize()
{
    wxSize size = GetSize();
    wxClientDC dc(this);
    labelSize = dc.GetTextExtent(wxWindow::GetLabel());
    wxSize textSize = text_ctrl->GetSize();
    int h = textSize.y + 8;
    if (size.y < h) {
        size.y = h;
    }
    wxSize minSize = size;
    minSize.x = GetMinWidth();
    SetMinSize(minSize);
    SetSize(size);
}
