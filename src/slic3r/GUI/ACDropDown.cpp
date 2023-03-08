#include "ACDropDown.hpp"
#include "ACLabel.hpp"

#include <wx/dcgraph.h>
#include "wx/defs.h"
#include "ACDefines.h"

wxDEFINE_EVENT(EVT_DISMISS, wxCommandEvent);

BEGIN_EVENT_TABLE(ACDropDown, wxPopupTransientWindow)

EVT_LEFT_DOWN(ACDropDown::mouseDown)
EVT_LEFT_UP(ACDropDown::mouseReleased)
EVT_MOUSE_CAPTURE_LOST(ACDropDown::mouseCaptureLost)
EVT_MOTION(ACDropDown::mouseMove)
EVT_MOUSEWHEEL(ACDropDown::mouseWheelMoved)

// catch paint events
EVT_PAINT(ACDropDown::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

ACDropDown::ACDropDown(std::vector<wxString> &texts,
                   std::vector<wxBitmap> &icons)
    : texts(texts)
    , icons(icons)
    , state_handler(this)
    , border_color(0xDBDBDB)
    , text_color(0x363636)
    , selector_border_color(
        std::make_pair(AC_COLOR_DROPDOWN_BD_HOV, (int) ACStateColor::Hovered),
        std::make_pair(AC_COLOR_DROPDOWN_BD_NOR, (int) ACStateColor::Normal))
    , selector_background_color(
        std::make_pair(AC_COLOR_DROPDOWN_ITEM_BG_HOV, (int) ACStateColor::Hovered),
        std::make_pair(AC_COLOR_BG_LIGHTGRAY_ITEM_DISABLE, (int) ACStateColor::Checked),
        std::make_pair(AC_COLOR_DROPDOWN_ITEM_BG_NOR, (int) ACStateColor::Normal))
{
    //this->SetBackgroundColour(wxTransparentColour);
}

ACDropDown::ACDropDown(wxWindow *             parent,
                   std::vector<wxString> &texts,
                   std::vector<wxBitmap> &icons,
                   long           style)
    : ACDropDown(texts, icons)
{
    Create(parent, style);
}

void ACDropDown::Create(wxWindow *     parent,
         long           style)
{
    wxPopupTransientWindow::Create(parent);
    SetWindowStyle(  wxFRAME_SHAPED|wxCLIP_CHILDREN);
    //SetBackgroundStyle(wxBG_STYLE_PAINT);
    //SetBackgroundColour(wxTransparentColour);
    SetTransparent(true);
    state_handler.attach({&border_color, &text_color, &selector_border_color, &selector_background_color});
    state_handler.update_binds();
    if ((style & DD_NO_CHECK_ICON) == 0)
        check_bitmap = ScalableBitmap(this, "checked", 16);
    text_off = style & DD_NO_TEXT;

    // BBS set default font
    SetFont(ACLabel::Body_14);
#ifdef __WXOSX__
    // wxPopupTransientWindow releases mouse on idle, which may cause various problems,
    //  such as losting mouse move, and dismissing soon on first LEFT_DOWN event.
    Bind(wxEVT_IDLE, [] (wxIdleEvent & evt) {});
#endif
}

void ACDropDown::Invalidate(bool clear)
{
    if (clear) {
        selection = hover_item = -1;
        offset = wxPoint();
    }
    assert(selection < (int) texts.size());
    need_sync = true;
}

void ACDropDown::SetSelection(int n)
{
    assert(n < (int) texts.size());
    if (n >= (int) texts.size())
        n = -1;
    if (selection == n) return;
    selection = n;
    paintNow();
}

wxString ACDropDown::GetValue() const
{
    return selection >= 0 ? texts[selection] : wxString();
}

void ACDropDown::SetValue(const wxString &value)
{
    auto i = std::find(texts.begin(), texts.end(), value);
    selection = i == texts.end() ? -1 : std::distance(texts.begin(), i);
}

void ACDropDown::SetCornerRadius(double radius)
{
    this->radius = radius;
    paintNow();
}

void ACDropDown::SetBorderColor(ACStateColor const &color)
{
    border_color = color;
    state_handler.update_binds();
    paintNow();
}

void ACDropDown::SetSelectorBorderColor(ACStateColor const &color)
{
    selector_border_color = color;
    state_handler.update_binds();
    paintNow();
}

void ACDropDown::SetTextColor(ACStateColor const &color)
{
    text_color = color;
    state_handler.update_binds();
    paintNow();
}

void ACDropDown::SetSelectorBackgroundColor(ACStateColor const &color)
{
    selector_background_color = color;
    state_handler.update_binds();
    paintNow();
}

void ACDropDown::SetUseContentWidth(bool use)
{
    if (use_content_width == use)
        return;
    use_content_width = use;
    need_sync = true;
    messureSize();
}

void ACDropDown::SetAlignIcon(bool align) { align_icon = align; }

void ACDropDown::Rescale()
{
    need_sync = true;
}

bool ACDropDown::HasDismissLongTime()
{
    auto now = boost::posix_time::microsec_clock::universal_time();
    return !IsShown() &&
        (now - dismissTime).total_milliseconds() >= 200;
}

void ACDropDown::paintEvent(wxPaintEvent& evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxBufferedPaintDC dc(this);
    render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void ACDropDown::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    //wxClientDC dc(this); 
    //render(dc);
    Refresh();
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void ACDropDown::render(wxDC &dc)
{
    if (texts.size() == 0) return;
    int states = state_handler.states();
    dc.SetPen(wxPen(border_color.colorForStates(states)));
    //dc.SetBackgroundMode(wxTRANSPARENT);
	dc.SetBackground(*wxTRANSPARENT_BRUSH);
    dc.SetBrush(*wxWHITE_BRUSH);
    // if (GetWindowStyle() & wxBORDER_NONE)
    //    dc.SetPen(wxNullPen);

    wxSize size = GetSize();

    //wxGraphicsPath path = dc.GetGraphicsContext()->CreatePath();
    //path.AddRoundedRectangle(0, 0, size.x, size.y, radius);
    //this->SetShape(path);

    // draw background
    //if (radius == 0)
        dc.DrawRectangle(0, 0, size.x, size.y);
    //else
    //    dc.DrawRoundedRectangle(0, 0, size.x, size.y, radius);

    // draw hover rectangle
    wxRect rcContent = {{0, offset.y}, rowSize};
    if (hover_item >= 0 && (states & ACStateColor::Hovered)) {
        rcContent.y += rowSize.y * hover_item;
        if (rcContent.GetBottom() > 0 && rcContent.y < size.y) {
            if (selection == hover_item)
                dc.SetBrush(wxBrush(selector_background_color.colorForStates(states | ACStateColor::Checked)));
            dc.SetPen(wxPen(selector_border_color.colorForStates(states)));
            rcContent.Deflate(4, 1);
            dc.DrawRectangle(rcContent);
            rcContent.Inflate(4, 1);
        }
        rcContent.y = offset.y;
    }
    // draw checked rectangle
    if (selection >= 0 && (selection != hover_item || (states & ACStateColor::Hovered) == 0)) {
        rcContent.y += rowSize.y * selection;
        if (rcContent.GetBottom() > 0 && rcContent.y < size.y) {
            dc.SetBrush(wxBrush(selector_background_color.colorForStates(states | ACStateColor::Checked)));
            dc.SetPen(wxPen(selector_background_color.colorForStates(states)));
            rcContent.Deflate(4, 1);
            dc.DrawRectangle(rcContent);
            rcContent.Inflate(4, 1);
        }
        rcContent.y = offset.y;
    }

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    {
        wxSize offset = (rowSize - textSize) / 2;
        rcContent.Deflate(0, offset.y);
    }

    // draw position bar
    if (rowSize.y * texts.size() > size.y) {
        int    height = rowSize.y * texts.size();
        wxRect rect = {size.x - 6, -offset.y * size.y / height, 4,
                       size.y * size.y / height};
        dc.SetPen(wxPen(border_color.defaultColor()));
        dc.SetBrush(wxBrush(*wxLIGHT_GREY));
        dc.DrawRoundedRectangle(rect, 2);
        rcContent.width -= 6;
    }

    // draw check icon
    rcContent.x += 5;
    rcContent.width -= 5;
    if (check_bitmap.bmp().IsOk()) {
        auto szBmp = check_bitmap.GetBmpSize();
        if (selection >= 0) {
            wxPoint pt = rcContent.GetLeftTop();
            pt.y += (rcContent.height - szBmp.y) / 2;
            pt.y += rowSize.y * selection;
            if (pt.y + szBmp.y > 0 && pt.y < size.y)
                dc.DrawBitmap(check_bitmap.bmp(), pt);
        }
        rcContent.x += szBmp.x + 5;
        rcContent.width -= szBmp.x + 5;
    }
    // draw texts & icons
    dc.SetTextForeground(text_color.colorForStates(states));
    for (int i = 0; i < texts.size(); ++i) {
        if (rcContent.GetBottom() < 0) {
            rcContent.y += rowSize.y;
            continue;
        }
        if (rcContent.y > size.y) break;
        wxPoint pt   = rcContent.GetLeftTop();
        auto &  icon = icons[i];
        if (iconSize.x > 0) {
            if (icon.IsOk()) {
                pt.y += (rcContent.height - icon.GetSize().y) / 2;
                dc.DrawBitmap(icon, pt);
            }
            pt.x += iconSize.x + 5;
            pt.y = rcContent.y;
        } else if (icon.IsOk()) {
            pt.y += (rcContent.height - icon.GetSize().y) / 2;
            dc.DrawBitmap(icon, pt);
            pt.x += icon.GetWidth() + 5;
            pt.y = rcContent.y;
        }
        auto text = texts[i];
        if (!text_off && !text.IsEmpty()) {
            wxSize tSize = dc.GetMultiLineTextExtent(text);
            if (pt.x + tSize.x > rcContent.GetRight()) {
                text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END,
                                            rcContent.GetRight() - pt.x);
            }
            pt.y += (rcContent.height - textSize.y) / 2;
            dc.SetFont(GetFont());
            dc.DrawText(text, pt);
        }
        rcContent.y += rowSize.y;
    }
}

void ACDropDown::messureSize()
{
    if (!need_sync) return;
    textSize = wxSize();
    iconSize = wxSize();
    wxClientDC dc(GetParent() ? GetParent() : this);
    for (size_t i = 0; i < texts.size(); ++i) {
        wxSize size1 = text_off ? wxSize() : dc.GetMultiLineTextExtent(texts[i]);
        if (icons[i].IsOk()) {
            wxSize size2 = icons[i].GetSize();
            if (size2.x > iconSize.x) iconSize = size2;
            if (!align_icon) {
                size1.x += size2.x + (text_off ? 0 : 5);
            }
        }
        if (size1.x > textSize.x) textSize = size1;
    }
    if (!align_icon) iconSize.x = 0;
    wxSize szContent = textSize;
    szContent.x += 10;
    if (check_bitmap.bmp().IsOk()) {
        auto szBmp = check_bitmap.bmp().GetSize();
        szContent.x += szBmp.x + 5;
    }
    if (iconSize.x > 0) szContent.x += iconSize.x + (text_off ? 0 : 5);
    if (iconSize.y > szContent.y) szContent.y = iconSize.y;
    szContent.y += 10;
    if (texts.size() > 15) szContent.x += 6;
    if (GetParent()) {
        auto x = GetParent()->GetSize().x;
        if (!use_content_width || x > szContent.x)
            szContent.x = x;
    }
    rowSize = szContent;
    szContent.y *= std::min((size_t)15, texts.size());
    szContent.y += texts.size() > 15 ? rowSize.y / 2 : 0;
    wxWindow::SetSize(szContent);




    need_sync = false;
}

void ACDropDown::autoPosition()
{
    messureSize();
    wxPoint pos = GetParent()->ClientToScreen(wxPoint(0, -6));
    wxPoint old = GetPosition();
    wxSize size = GetSize();
    Position(pos, {0, GetParent()->GetSize().y + 12});
    if (old != GetPosition()) {
        size = rowSize;
        size.y *= std::min((size_t)15, texts.size());
        size.y += texts.size() > 15 ? rowSize.y / 2 : 0;
        if (size != GetSize()) {
            wxWindow::SetSize(size);
            offset = wxPoint();
            Position(pos, {0, GetParent()->GetSize().y + 12});
        }
    }
    if (GetPosition().y > pos.y) {
        // may exceed
        auto drect = wxDisplay(GetParent()).GetGeometry();
        if (GetPosition().y + size.y + 10 > drect.GetBottom()) {
            if (use_content_width && texts.size() <= 15) size.x += 6;
            size.y = drect.GetBottom() - GetPosition().y - 10;
            wxWindow::SetSize(size);
            if (selection >= 0) {
                if (offset.y + rowSize.y * (selection + 1) > size.y)
                    offset.y = size.y - rowSize.y * (selection + 1);
                else if (offset.y + rowSize.y * selection < 0)
                    offset.y = -rowSize.y * selection;
            }
        }
    }
}

void ACDropDown::mouseDown(wxMouseEvent& event)
{
    // Receivce unexcepted LEFT_DOWN on Mac after OnDismiss
    if (!IsShown())
        return;
    // force calc hover item again
    mouseMove(event);
    pressedDown = true;
    CaptureMouse();
    dragStart   = event.GetPosition();
}

void ACDropDown::mouseReleased(wxMouseEvent& event)
{
    if (pressedDown) {
        dragStart = wxPoint();
        pressedDown = false;
        if (HasCapture())
            ReleaseMouse();
        if (hover_item >= 0) { // not moved
            sendDropDownEvent();
            DismissAndNotify();
        }
    }
}

void ACDropDown::mouseCaptureLost(wxMouseCaptureLostEvent &event)
{
    wxMouseEvent evt;
    mouseReleased(evt);
}

void ACDropDown::mouseMove(wxMouseEvent &event)
{
    wxPoint pt  = event.GetPosition();
    if (pressedDown) {
        wxPoint pt2 = offset + pt - dragStart;
        dragStart = pt;
        if (pt2.y > 0)
            pt2.y = 0;
        else if (pt2.y + rowSize.y * texts.size() < GetSize().y)
            pt2.y = GetSize().y - rowSize.y * texts.size();
        if (pt2.y != offset.y) {
            offset = pt2;
            hover_item = -1; // moved
        } else {
            return;
        }
    }
    if (!pressedDown || hover_item >= 0) {
        int hover = (pt.y - offset.y) / rowSize.y;
        if (hover >= (int) texts.size()) hover = -1;
        if (hover == hover_item) return;
        hover_item = hover;
        //if (hover >= 0)
        //    SetToolTip(texts[hover]);
    }
    paintNow();
}

void ACDropDown::mouseWheelMoved(wxMouseEvent &event)
{
    auto delta = event.GetWheelRotation() > 0 ? rowSize.y : -rowSize.y;
    wxPoint pt2 = offset + wxPoint{0, delta};
    if (pt2.y > 0)
        pt2.y = 0;
    else if (pt2.y + rowSize.y * texts.size() < GetSize().y)
        pt2.y = GetSize().y - rowSize.y * texts.size();
    if (pt2.y != offset.y) {
        offset = pt2;
    } else {
        return;
    }
    int hover = (event.GetPosition().y - offset.y) / rowSize.y;
    if (hover >= (int) texts.size()) hover = -1;
    if (hover != hover_item) {
        hover_item = hover;
        //if (hover >= 0) SetToolTip(texts[hover]);
    }
    paintNow();
}

// currently unused events
void ACDropDown::sendDropDownEvent()
{
    selection = hover_item;
    wxCommandEvent event(wxEVT_COMBOBOX, GetId());
    event.SetEventObject(this);
    event.SetInt(selection);
    event.SetString(GetValue());
    GetEventHandler()->ProcessEvent(event);
}

void ACDropDown::OnDismiss()
{
    dismissTime = boost::posix_time::microsec_clock::universal_time();
    hover_item  = -1;
    wxCommandEvent e(EVT_DISMISS);
    GetEventHandler()->ProcessEvent(e);
}
