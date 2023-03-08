#include "ACButton.hpp"
#include "ACStateColor.hpp"
#include "ACLabel.hpp"
#include "ACStateHandler.hpp"

#include <wx/dcgraph.h>

BEGIN_EVENT_TABLE(ACButton, ACStaticBox)

EVT_LEFT_DOWN(ACButton::mouseDown)
EVT_LEFT_UP(ACButton::mouseReleased)
EVT_MOUSE_CAPTURE_LOST(ACButton::mouseCaptureLost)
EVT_KEY_DOWN(ACButton::keyDownUp)
EVT_KEY_UP(ACButton::keyDownUp)

// catch paint events
EVT_PAINT(ACButton::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

ACButton::ACButton()
    : paddingSize(10, 10)
{
    background_color = ACStateColor(
        std::make_pair(0xF0F0F0, (int) ACStateColor::Disabled),
        std::make_pair(0x37EE7C, (int) ACStateColor::Hovered | ACStateColor::Checked),
        std::make_pair(0x00AE42, (int) ACStateColor::Checked),
        std::make_pair(*wxLIGHT_GREY, (int) ACStateColor::Hovered), 
        std::make_pair(*wxWHITE, (int) ACStateColor::Normal));
    text_color       = ACStateColor(
        std::make_pair(*wxLIGHT_GREY, (int) ACStateColor::Disabled), 
        std::make_pair(*wxBLACK, (int) ACStateColor::Normal));
}

ACButton::ACButton(wxWindow* parent, wxString text, wxString icon, long style, int iconSize)
    : ACButton()
{
    Create(parent, text, icon, style, iconSize);
}

bool ACButton::Create(wxWindow* parent, wxString text, wxString icon, long style, int iconSize)
{
    ACStaticBox::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style);

    if (style & wxBORDER_NONE)
        m_alignLeft = true;

    state_handler.attach({&text_color});
    state_handler.update_binds();
    //BBS set default font
    SetFont(ACLabel::Body_14);
    wxWindow::SetLabel(text);
    if (!icon.IsEmpty()) {
        //BBS set button icon default size to 20
        this->active_icon = ScalableBitmap(this, icon.ToStdString(), iconSize > 0 ? iconSize : 20);
    }
    messureSize();
    return true;
}

void ACButton::SetLabel(const wxString& label)
{
    wxWindow::SetLabel(label);
    messureSize();
    Refresh();
}

void ACButton::SetIcon(const wxString& icon)
{
    if (!icon.IsEmpty()) {
        //BBS set button icon default size to 20
        this->active_icon = ScalableBitmap(this, icon.ToStdString(), this->active_icon.px_cnt());
    }
    else
    {
        this->active_icon = ScalableBitmap();
    }
    Refresh();
}

void ACButton::SetInactiveIcon(const wxString &icon)
{
    if (!icon.IsEmpty()) {
        // BBS set button icon default size to 20
        this->inactive_icon = ScalableBitmap(this, icon.ToStdString(), this->active_icon.px_cnt());
    } else {
        this->inactive_icon = ScalableBitmap();
    }
    Refresh();
}

void ACButton::SetMinSize(const wxSize& size)
{
    minSize = size;
    messureSize();
}

void ACButton::SetPaddingSize(const wxSize& size)
{
    paddingSize = size;
    messureSize();
}

void ACButton::SetOffset(const wxSize& size)
{
    m_offset = size;
    messureSize();
}


void ACButton::SetTextColor(ACStateColor const& color)
{
    text_color = color;
    state_handler.update_binds();
    Refresh();
}

void ACButton::SetTextColorNormal(wxColor const &color)
{
    text_color.setColorForStates(color, 0);
    Refresh();
}

bool ACButton::Enable(bool enable)
{
    bool result = wxWindow::Enable(enable);
    if (result) {
        wxCommandEvent e(EVT_ENABLE_CHANGED);
        e.SetEventObject(this);
        GetEventHandler()->ProcessEvent(e);
    }
    return result;
}

void ACButton::SetCanFocus(bool canFocus) { this->canFocus = canFocus; }

void ACButton::Rescale()
{
    if (this->active_icon.bmp().IsOk())
        this->active_icon.msw_rescale();

    if (this->inactive_icon.bmp().IsOk())
        this->inactive_icon.msw_rescale();

    messureSize();
}

void ACButton::paintEvent(wxPaintEvent& evt)
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
void ACButton::render(wxDC& dc)
{
    ACStaticBox::render(dc);

    // draw icon and text
    int states = state_handler.states();
    wxSize size = GetSize();
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    // calc content size
    wxSize szIcon;
    wxSize szContent = textSize;

    ScalableBitmap icon;
    if (m_selected || ((states & (int)ACStateColor::State::Hovered) != 0))
        icon = active_icon;
    else
        icon = inactive_icon;
    int padding = 5;// between icon and text
    if (icon.bmp().IsOk()) {
        if (szContent.y > 0) {
            //BBS norrow size between text and icon
            szContent.x += padding;
        }
        szIcon = icon.GetBmpSize();
        szContent.x += szIcon.x;
        if (szIcon.y > szContent.y)
            szContent.y = szIcon.y;
        if (szContent.x > size.x) {
            int d = std::min(padding, szContent.x - size.x);
            padding -= d;
            szContent.x -= d;
        }
    }

    wxSize offset(0,0);
    if (m_alignLeft) {
        offset = m_offset;
    } else {
        // move to center
        offset = (size - szContent) / 2;
        if (offset.x < 0) offset.x = 0;        
    }

    wxRect rcContent = { {0, 0}, size };
    rcContent.Deflate(offset.x, offset.y);
    // start draw
    
    // icon
    wxPoint pt = rcContent.GetLeftTop();
    if (icon.bmp().IsOk()) {
        pt.y += (rcContent.height - szIcon.y) / 2;
        dc.DrawBitmap(icon.bmp(), pt);
        //BBS norrow size between text and icon
        pt.x += szIcon.x + padding;
        pt.y = rcContent.y;
    }
    // text
    auto text = GetLabel();
    if (!text.IsEmpty()) {
        if (pt.x + textSize.x > size.x)
            text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
        pt.y += (rcContent.height - textSize.y) / 2;
        dc.SetFont(GetFont());
        dc.SetTextForeground(text_color.colorForStates(states));
        dc.DrawText(text, pt);
    }
}

void ACButton::messureSize()
{
    wxClientDC dc(this);
    wxString text = GetLabel();
    if (text.IsEmpty()) {
        text = "";
    }
    textSize = dc.GetTextExtent(text);
    if (minSize.GetWidth() > 0) {
        wxWindow::SetMinSize(minSize);
        return;
    }
    wxSize szContent = textSize;
    if (this->active_icon.bmp().IsOk()) {
        if (szContent.y > 0) {
            //BBS norrow size between text and icon
            szContent.x += 5;
        }
        wxSize szIcon = this->active_icon.GetBmpSize();
        szContent.x += szIcon.x;
        if (szIcon.y > szContent.y)
            szContent.y = szIcon.y;
    }
    wxSize size = szContent + paddingSize * 2;

    if (minSize.GetHeight() > 0)
        size.SetHeight(minSize.GetHeight());

    wxWindow::SetMinSize(size);
}

void ACButton::mouseDown(wxMouseEvent& event)
{
    event.Skip();
    pressedDown = true;
    if (canFocus)
        SetFocus();
    CaptureMouse();
}

void ACButton::mouseReleased(wxMouseEvent& event)
{
    event.Skip();
    if (pressedDown) {
        pressedDown = false;
        if (HasCapture())
            ReleaseMouse();
        if (wxRect({0, 0}, GetSize()).Contains(event.GetPosition()))
            sendButtonEvent();
    }
}

void ACButton::mouseCaptureLost(wxMouseCaptureLostEvent &event)
{
    wxMouseEvent evt;
    mouseReleased(evt);
}

void ACButton::keyDownUp(wxKeyEvent &event)
{
    if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN) {
        wxMouseEvent evt(event.GetEventType() == wxEVT_KEY_UP ? wxEVT_LEFT_UP : wxEVT_LEFT_DOWN);
        event.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
        return;
    }
    if (event.GetEventType() == wxEVT_KEY_DOWN &&
        (event.GetKeyCode() == WXK_TAB || event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT 
        || event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN))
        HandleAsNavigationKey(event);
    else
        event.Skip();
}

void ACButton::sendButtonEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
}

#ifdef __WIN32__

WXLRESULT ACButton::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (nMsg == WM_GETDLGCODE) { return DLGC_WANTMESSAGE; }
    if (nMsg == WM_KEYDOWN) {
        wxKeyEvent event(CreateKeyEvent(wxEVT_KEY_DOWN, wParam, lParam));
        switch (wParam) {
        case WXK_RETURN: { // WXK_RETURN key is handled by default button
            GetEventHandler()->ProcessEvent(event);
            return 0;
        }
        }
    }
    return wxWindow::MSWWindowProc(nMsg, wParam, lParam);
}

#endif

bool ACButton::AcceptsFocus() const { return canFocus; }