#include "ACListBox.hpp"

#include <wx/dcgraph.h>
#include "ACDefines.h"
#include "ACStateColor.hpp"

//BEGIN_EVENT_TABLE(ACListBox, ACStaticBox)
//
////EVT_PAINT(ACListBox::paintEvent)
//
//END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

wxDEFINE_EVENT(EVT_ACLISTBOX_SEL_CHANGED, wxCommandEvent);

ACListBox::ACListBox()
{
}

ACListBox::ACListBox(wxWindow *     parent,
                     wxWindowID      id,
                     const wxPoint &pos,
                     const wxSize  &size,
                     long           style)
{
    Create(parent, pos, size, style);
}

void ACListBox::Create(wxWindow *     parent,
                       const wxPoint &pos,
                       const wxSize  &size,
                       long           style)
{

    ACStaticBox::Create(parent, wxID_ANY, pos, size, style);

    SetCornerRadius(FromDIP(14));
    SetBackgroundColour(AC_COLOR_PANEL_BG);

    m_mainSizer = new wxBoxSizer(wxVERTICAL);
    m_mainSizer->AddSpacer(FromDIP(ListBoxItemsMargin));

    m_mainSizer->AddSpacer(FromDIP(ListBoxItemsMargin));

    SetSizerAndFit(m_mainSizer);

    messureSize();
}

void ACListBox::Rescale()
{
    for (ACButton* bt : m_buttons) {
        bt->Rescale();
    }

    messureSize();
    Refresh();
}


int ACListBox::AppendItem(const wxString& text, const wxString& imgName)
{
    m_bgColor_nor = ACStateColor(
        std::make_pair(AC_COLOR_LISTBOX_ITEM_BG_NOR, (int) ACStateColor::Normal), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_BG_HOV, (int) ACStateColor::Hovered), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_BG_PRE, (int) ACStateColor::Pressed));

    m_bgColor_sel = ACStateColor(
        std::make_pair(AC_COLOR_LISTBOX_ITEM_BG_SEL, (int) ACStateColor::Normal), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_BG_SEL, (int) ACStateColor::Hovered), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_BG_SEL, (int) ACStateColor::Pressed));

    m_textColor_nor = ACStateColor(
        std::make_pair(AC_COLOR_LISTBOX_ITEM_FG_NOR, (int) ACStateColor::Normal), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_FG_HOV, (int) ACStateColor::Hovered), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_FG_PRE, (int) ACStateColor::Pressed));

    m_textColor_sel = ACStateColor(
        std::make_pair(AC_COLOR_LISTBOX_ITEM_FG_SEL, (int) ACStateColor::Normal), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_FG_SEL, (int) ACStateColor::Hovered), 
        std::make_pair(AC_COLOR_LISTBOX_ITEM_FG_SEL, (int) ACStateColor::Pressed));


    ACButton* button = new ACButton(this, text, imgName, wxBORDER_NONE|wxALIGN_LEFT, FromDIP(9));
    button->SetId(ID_AC_BUTTON);
    button->SetCornerRadius(FromDIP(10));
    button->SetOffset(wxSize(FromDIP(10), FromDIP(10)));
    button->SetBackgroundColour(AC_COLOR_PANEL_BG);
    button->DisableFocusFromKeyboard();

    button->SetBackgroundColor(m_bgColor_nor);
    button->SetTextColor(m_textColor_nor);

    button->SetMinSize(wxSize(FromDIP(ListItemWidth), FromDIP(ListItemHeight)));

    button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [&,  button](wxEvent& event){
        int clickedID = -1;
        int i=0;
        for (auto it = m_buttons.begin(); it !=  m_buttons.end(); ++it, ++i) {
            if (button == *it) {
                clickedID = i;
            }
        }

        if (clickedID == -1)
            return;

        //if (m_cb_ItemSelected) {
        //    m_cb_ItemSelected(clickedID);
        //}

        //wxCommandEvent evt = wxCommandEvent(EVT_ACLISTBOX_SEL_CHANGED);
        //evt.SetId(clickedID);
        //
        //wxPostEvent(this->GetParent(), evt);

        SelectItem(clickedID);

    }, ID_AC_BUTTON);

    if (m_buttons.size() != 0)
        m_mainSizer->InsertSpacer(1, FromDIP(ListBoxItemsGap));
    m_mainSizer->Insert(1, button, 0, wxLEFT|wxRIGHT, FromDIP(ListBoxItemsGap));
    
    m_buttons.push_back(button);

    if (m_currentSelIndex == -1) {
        SelectItem(0);
    }

    return m_buttons.size()-1;
}

void ACListBox::onItemSelected(std::function<void(int)> cb_ItemSelected)
{
    m_cb_ItemSelected = cb_ItemSelected;
}

void ACListBox::SelectItem(int id)
{
    if (id < 0 || id > m_buttons.size()-1) {
        // out of range
        return;
    }
    ACButton* oldButton = m_currentSelIndex == -1 ? nullptr : m_buttons[m_currentSelIndex];
    if (oldButton) {
        oldButton->SetBackgroundColor(m_bgColor_nor);
        oldButton->SetTextColor(m_textColor_nor);
    }

    m_currentSelIndex = id;
    ACButton* button = m_buttons[m_currentSelIndex];

    button->SetBackgroundColor(m_bgColor_sel);
    button->SetTextColor(m_textColor_sel);

    if (m_cb_ItemSelected) {
        m_cb_ItemSelected(0);
    }

    Refresh();
}


int ACListBox::GetSelection()
{
    return m_currentSelIndex;
}

wxString ACListBox::GetItemText(int index)
{
    if(index < 0)
        return "";

    return m_buttons[index]->GetLabel();
}

void ACListBox::DeleteChildren()
{
    m_mainSizer->Clear(true);
    m_currentSelIndex = -1;
    m_buttons.clear();
}



//bool ACListBox::DoNavigateIn(int flags ) 
//{
//    switch (flags) {
//        case wxNavigationKeyEvent::IsForward:
//        case wxNavigationKeyEvent::IsBackward:
//    }
//}

//void ACListBox::SetMinSize(const wxSize& size)
//{
//    wxSize size2 = size;
//    if (size2.y < 0) {
//#ifdef __WXMAC__
//        if (GetPeer()) // peer is not ready in Create on mac
//#endif
//        size2.y = GetSize().y;
//    }
//    wxWindow::SetMinSize(size2);
//}

//void ACListBox::DoSetSize(int x, int y, int width, int height, int sizeFlags)
//{
//    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
//    if (sizeFlags & wxSIZE_USE_EXISTING) return;
//    wxSize size = GetSize();
//    wxPoint textPos = {5, 0};
//    if (this->icon.bmp().IsOk()) {
//        wxSize szIcon = this->icon.GetBmpSize();
//        textPos.x += szIcon.x;
//    }
//    bool align_right = GetWindowStyle() & wxRIGHT;
//    if (align_right)
//        textPos.x += labelSize.x;
//    if (text_ctrl) {
//        wxSize textSize = text_ctrl->GetSize();
//        textSize.x = size.x - textPos.x - labelSize.x - 10;
//        text_ctrl->SetSize(textSize);
//        text_ctrl->SetPosition({textPos.x, (size.y - textSize.y) / 2});
//    }
//}


//void ACListBox::paintEvent(wxPaintEvent &evt)
//{
//    // depending on your system you may need to look at double-buffered dcs
//    wxPaintDC dc(this);
//    render(dc);
//}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
//void ACListBox::render(wxDC& dc)
//{
//    ACStaticBox::render(dc);
//    int states = state_handler.states();
//    wxSize size = GetSize();
//    bool   align_right = GetWindowStyle() & wxRIGHT;
//    // start draw
//    wxPoint pt = {5, 0};
//    if (icon.bmp().IsOk()) {
//        wxSize szIcon = icon.GetBmpSize();
//        pt.y = (size.y - szIcon.y) / 2;
//        dc.DrawBitmap(icon.bmp(), pt);
//        pt.x += szIcon.x + 0;
//    }
//    auto text = wxWindow::GetLabel();
//    if (!text.IsEmpty()) {
//        wxSize textSize = text_ctrl->GetSize();
//        if (align_right) {
//            if (pt.x + labelSize.x > size.x)
//                text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, size.x - pt.x);
//            pt.y = (size.y - labelSize.y) / 2;
//        } else {
//            pt.x += textSize.x;
//            pt.y = (size.y + textSize.y) / 2 - labelSize.y;
//        }
//        dc.SetTextForeground(label_color.colorForStates(states));
//        dc.SetFont(GetFont());
//        dc.DrawText(text, pt);
//    }
//}

void ACListBox::messureSize()
{
    wxSize size = GetSize();
    //wxClientDC dc(this);
    //labelSize = dc.GetTextExtent(wxWindow::GetLabel());
    //wxSize textSize = text_ctrl->GetSize();
    //int h = textSize.y + 8;
    //if (size.y < h) {
    //    size.y = h;
    //}
    int nItems = m_buttons.size();
    int w = FromDIP(ListItemWidth+ListBoxItemsMargin);
    int h = FromDIP(nItems*ListItemHeight+(nItems-1)*ListBoxItemsGap+ListBoxItemsMargin*2);

    if (size.x < w) size.x = w;
    if (size.y < h) size.y = h;

    wxSize minSize = size;

    SetMinSize(minSize);
    SetSize(size);
}
