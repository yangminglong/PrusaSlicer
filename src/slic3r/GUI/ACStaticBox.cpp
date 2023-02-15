#include "ACStaticBox.hpp"
#include "ACStateColor.hpp"
#include <wx/dcgraph.h>

BEGIN_EVENT_TABLE(ACStaticBox, wxWindow)

// catch paint events
//EVT_ERASE_BACKGROUND(ACStaticBox::eraseEvent)
EVT_PAINT(ACStaticBox::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

ACStaticBox::ACStaticBox()
    : state_handler(this), radius(8), radiusType(CornerAll)
{
    border_color = ACStateColor(
        std::make_pair(*wxLIGHT_GREY, (int) ACStateColor::Disabled), 
        std::make_pair(0x303A3C, (int) ACStateColor::Normal));
}

ACStaticBox::ACStaticBox(wxWindow* parent,
                   wxWindowID      id,
                   const wxPoint & pos,
                   const wxSize &  size, long style)
    : ACStaticBox()
{
    Create(parent, id, pos, size, style);
}

bool ACStaticBox::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
    if (style & wxBORDER_NONE)
        border_width = 0;
    wxWindow::Create(parent, id, pos, size, style);
    state_handler.attach({&border_color, &background_color, &background_color2});
    state_handler.update_binds();
    SetBackgroundColour(GetParentBackgroundColor(parent));
    return true;
}

void ACStaticBox::SetCornerRadius(double radius)
{
    this->radius = radius;
    Refresh();
}

void ACStaticBox::SetCornerRadiusType(CornerRadiusType type)
{
    this->radiusType = (int)type;
    Refresh();
}

void ACStaticBox::SetCornerRadius(double radius, CornerRadiusType type)
{
    this->radius = radius;
    this->radiusType = (int)type;
    Refresh();
}

void ACStaticBox::SetBorderWidth(int width)
{
    border_width = width;
    Refresh();
}

void ACStaticBox::SetBorderColor(ACStateColor const &color)
{
    border_color = color;
    state_handler.update_binds();
    Refresh();
}

void ACStaticBox::SetBorderColorNormal(wxColor const &color)
{
    border_color.setColorForStates(color, 0);
    Refresh();
}

void ACStaticBox::SetBackgroundColor(ACStateColor const &color)
{
    background_color = color;
    state_handler.update_binds();
    Refresh();
}

void ACStaticBox::SetBackgroundColorNormal(wxColor const &color)
{
    background_color.setColorForStates(color, 0);
    Refresh();
}

void ACStaticBox::SetBackgroundColor2(ACStateColor const &color)
{
    background_color2 = color;
    state_handler.update_binds();
    Refresh();
}

wxColor ACStaticBox::GetParentBackgroundColor(wxWindow* parent)
{
    if (auto box = dynamic_cast<ACStaticBox*>(parent)) {
        if (box->background_color.count() > 0) {
            if (box->background_color2.count() == 0)
                return box->background_color.defaultColor();
            auto s = box->background_color.defaultColor();
            auto e = box->background_color2.defaultColor();
            int r = (s.Red() + e.Red()) / 2;
            int g = (s.Green() + e.Green()) / 2;
            int b = (s.Blue() + e.Blue()) / 2;
            return wxColor(r, g, b);
        }
    }
    if (parent)
        return parent->GetBackgroundColour();
    return *wxWHITE;
}

void ACStaticBox::eraseEvent(wxEraseEvent& evt)
{
    // for transparent background, but not work
#ifdef __WXMSW__
    wxDC *dc = evt.GetDC();
    wxSize size = GetSize();
    wxClientDC dc2(GetParent());
    dc->Blit({0, 0}, size, &dc2, GetPosition());
#endif
}

void ACStaticBox::paintEvent(wxPaintEvent& evt)
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
void ACStaticBox::render(wxDC& dc)
{
#ifdef __WXMSW__
    if (radius == 0) {
        doRender(dc);
        return;
    }

	wxSize size = GetSize();
    wxMemoryDC memdc;
    wxBitmap bmp(size.x, size.y);
    memdc.SelectObject(bmp);
    //memdc.Blit({0, 0}, size, &dc, {0, 0});
    memdc.SetBackground(wxBrush(GetBackgroundColour()));
    memdc.Clear();
    {
        wxGCDC dc2(memdc);
        doRender(dc2);
    }

    memdc.SelectObject(wxNullBitmap);
	dc.DrawBitmap(bmp, 0, 0);
#else
    doRender(dc);
#endif
}

void ACStaticBox::doRender(wxDC& dc)
{
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    wxSize size = GetSize();
    int states = state_handler.states();
    if (background_color2.count() == 0) {
        if ((border_width && border_color.count() > 0) || background_color.count() > 0) {
            wxRect rc(0, 0, size.x, size.y);
            if (border_width && border_color.count() > 0) {
                if (dc.GetContentScaleFactor() == 1.0) {
                    int d  = floor(border_width / 2.0);
                    int d2 = floor(border_width - 1);
                    rc.x += d;
                    rc.width -= d2;
                    rc.y += d;
                    rc.height -= d2;
                } else {
                    int d  = 1;
                    rc.x += d;
                    rc.width -= d;
                    rc.y += d;
                    rc.height -= d;
                }
                dc.SetPen(wxPen(border_color.colorForStates(states), border_width));
            } else {
                dc.SetPen(wxPen(background_color.colorForStates(states)));
            }

            if (background_color.count() > 0)
                dc.SetBrush(wxBrush(background_color.colorForStates(states)));
            else
                dc.SetBrush(wxBrush(GetBackgroundColour()));
            
            if (radius == 0) {
                dc.DrawRectangle(rc);
            }
            else {
                dc.DrawRoundedRectangle(rc, radius - border_width);

                if (radiusType != CornerAll) {
                    if ((radiusType & (int) CornerTopLeft) == 0) {
                        wxRect src(rc.x, rc.y, rc.width / 2, rc.height / 2);
                        dc.DrawRectangle(src);
                    }
                    if ((radiusType & (int) CornerTopRight) == 0) {
                        wxRect src(rc.x + rc.width / 2+1, rc.y, rc.width / 2,
                                   rc.height / 2);
                        dc.DrawRectangle(src);
                    }
                    if ((radiusType & (int) CornerBottomLeft) == 0) {
                        wxRect src(rc.x, rc.y + rc.height / 2 + 1,
                                   rc.width / 2,
                                   rc.height / 2);
                        dc.DrawRectangle(src);
                    }
                    if ((radiusType & (int) CornerBottomRight) == 0) {
                        wxRect src(rc.x + rc.width / 2 + 1,
                                   rc.y + rc.height / 2 + 1,
                                   rc.width / 2,
                                   rc.height / 2);
                        dc.DrawRectangle(src);
                    }
                }
            }
        }
    }
    else {
        wxColor start = background_color.colorForStates(states);
        wxColor stop = background_color2.colorForStates(states);
        int r = start.Red(), g = start.Green(), b = start.Blue();
        int dr = (int) stop.Red() - r, dg = (int) stop.Green() - g, db = (int) stop.Blue() - b;
        int lr = 0, lg = 0, lb = 0;
        for (int y = 0; y < size.y; ++y) {
            dc.SetPen(wxPen(wxColor(r, g, b)));
            dc.DrawLine(0, y, size.x, y);
            lr += dr; while (lr >= size.y) { ++r, lr -= size.y; } while (lr <= -size.y) { --r, lr += size.y; }
            lg += dg; while (lg >= size.y) { ++g, lg -= size.y; } while (lg <= -size.y) { --g, lg += size.y; }
            lb += db; while (lb >= size.y) { ++b, lb -= size.y; } while (lb <= -size.y) { --b, lb += size.y; }
        }
    }
}
