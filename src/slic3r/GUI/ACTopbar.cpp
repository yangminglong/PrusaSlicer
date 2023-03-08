#include "ACTopbar.hpp"
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

#include "ACDefines.h"

#define TOPBAR_ICON_SIZE  26
//#define TOPBAR_TITLE_WIDTH  300

using namespace Slic3r;


class BBLTopbarArt : public wxAuiDefaultToolBarArt
{
public:
    BBLTopbarArt();
    virtual void DrawLabel(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect) wxOVERRIDE;
    virtual void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) wxOVERRIDE;
    virtual void DrawButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect) wxOVERRIDE;
private:
    wxColour m_bgColor;

    wxColour m_menuBgColorNor;
    wxColour m_menuBgColorHover;
    wxColour m_menuBgColorActive;

    wxColour m_textColorNor;
    wxColour m_textColorHover;
    wxColour m_textColorActive;

    wxColour m_btBgColorNor;
    wxColour m_btBgColorHover;
    wxColour m_btBgColorActive;
};

BBLTopbarArt::BBLTopbarArt()
    : wxAuiDefaultToolBarArt()
{
    m_bgColor = AC_COLOR_WHITE;
    
    m_btBgColorNor    = AC_COLOR_PANEL_BG;
    m_btBgColorHover  = AC_COLOR_MAIN_BLUE_HOVER;
    m_btBgColorActive = AC_COLOR_MAIN_BLUE;

    m_menuBgColorNor    = m_bgColor;
    m_menuBgColorHover   = m_bgColor;
    m_menuBgColorActive = AC_COLOR_MAIN_BLUE;

    m_textColorNor    = AC_COLOR_BLACK;
    m_textColorHover  = AC_COLOR_MAIN_BLUE_HOVER;
    m_textColorActive = AC_COLOR_WHITE;


}

void BBLTopbarArt::DrawLabel(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect)
{
    dc.SetFont(m_font);
    dc.SetTextForeground(m_textColorNor);
//#ifdef __WINDOWS__
//    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
//#else
//    dc.SetTextForeground(*wxWHITE);
//#endif

    wxAuiToolBar* toolbar = (wxAuiToolBar*)wnd;
    wxSize toolBarSize = toolbar->GetSize();

    int textWidth = 0, textHeight = 0;
    dc.GetTextExtent(item.GetLabel(), &textWidth, &textHeight);

    wxRect clipRect = rect;
    clipRect.width -= 1;
    dc.SetClippingRegion(clipRect);

    int textX, textY;
    int leftSpace = std::max(0, toolBarSize.x/2-rect.x);

    if (textWidth > leftSpace*2)  {
        textX = rect.x + 1;
    }
    else {
        textX = toolBarSize.GetWidth()/2- textWidth/2;
    }

    //if (textWidth < rect.GetWidth()) {
    //    textX = rect.x + 1 + (rect.width - textWidth) / 2;
    //}
    //else {
    //    textX = rect.x + 1;
    //}
    wxFont font = dc.GetFont();
    
    dc.SetFont(font.MakeBold());
    textY = rect.y + (rect.height - textHeight) / 2;
    dc.DrawText(item.GetLabel(), textX, textY);
    dc.DestroyClippingRegion();
}

void BBLTopbarArt::DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
    //dc.SetBrush(wxBrush(wxColour(38, 46, 48))); //
    dc.SetBrush(m_bgColor); //55, 66, 84
    dc.SetPen(*wxTRANSPARENT_PEN);  // without boardline
    wxRect clipRect = rect;
    clipRect.y -= 8;
    clipRect.height += 8;
    dc.SetClippingRegion(clipRect); // ???
    dc.DrawRectangle(rect);
    dc.DestroyClippingRegion();
}

void BBLTopbarArt::DrawButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect)
{
    int textWidth = 0, textHeight = 0;

    if (m_flags & wxAUI_TB_TEXT)
    {
        dc.SetFont(m_font);
        int tx, ty;

        dc.GetTextExtent(wxT("ABCDHgj"), &tx, &textHeight);
        textWidth = 0;
        dc.GetTextExtent(item.GetLabel(), &textWidth, &ty);
    }

    int bmpX = 0, bmpY = 0;
    int textX = 0, textY = 0;

    bool isHover = item.GetState() & wxAUI_BUTTON_STATE_HOVER;
    bool isDisable = item.GetState() & wxAUI_BUTTON_STATE_DISABLED;
    bool isPressed = item.GetState() & wxAUI_BUTTON_STATE_PRESSED;
    const wxBitmap& bmp = isDisable ? item.GetDisabledBitmap() :
        isHover ? item.GetHoverBitmap() : item.GetBitmap();

    const wxSize bmpSize = bmp.IsOk() ? bmp.GetScaledSize() : wxSize(0, 0);

    if (m_textOrientation == wxAUI_TBTOOL_TEXT_BOTTOM)
    {
        bmpX = rect.x +
            (rect.width / 2) -
            (bmpSize.x / 2);

        bmpY = rect.y +
            ((rect.height - textHeight) / 2) -
            (bmpSize.y / 2);

        textX = rect.x + (rect.width / 2) - (textWidth / 2) + 1;
        textY = rect.y + rect.height - textHeight - 1;
    }
    else if (m_textOrientation == wxAUI_TBTOOL_TEXT_RIGHT)
    {
        bmpX = rect.x + (textWidth == 0 ? (rect.width-bmpSize.x)/2 : wnd->FromDIP(3));

        bmpY = rect.y +
            (rect.height / 2) -
            (bmpSize.y / 2);

        textX = bmpX + wnd->FromDIP(3) + bmpSize.x;
        textY = rect.y +
            (rect.height / 2) -
            (textHeight / 2);
    }

    
    bool isMenu = (int)ID_TOP_FILE_MENU == item.GetId() ||
                  (int)ID_TOP_EDIT_MENU == item.GetId() ||
                  (int)ID_TOP_VIEW_MENU == item.GetId() ||
                  (int)ID_TOP_SETS_MENU == item.GetId() ||
                  (int)ID_TOP_HELP_MENU == item.GetId();

    //bool isLogo = (int)ID_LOGO == item.GetId();
    //if (isLogo) {
    //    // without background
    //}
    if (isMenu) 
    {
        dc.SetPen(*wxTRANSPARENT_PEN);
        if (isHover )
        {
            dc.SetBrush(wxBrush(m_menuBgColorHover));
             dc.SetTextForeground(m_textColorHover);
       } 
        else if (item.IsSticky())
        {
            dc.SetBrush(wxBrush(m_menuBgColorActive));
            dc.SetTextForeground(m_textColorActive);
        }
        else {
            dc.SetBrush(wxBrush(m_menuBgColorNor));
            dc.SetTextForeground(m_textColorNor);
        }
        dc.DrawRectangle(rect );
    }
    //else {
    //    dc.SetPen(*wxTRANSPARENT_PEN);
    //    if ((item.GetState() & wxAUI_BUTTON_STATE_HOVER) || item.IsSticky())
    //    {
    //        dc.SetBrush(wxBrush(m_btBgColorHover));
    //    }
    //    else if (item.GetState() & wxAUI_BUTTON_STATE_PRESSED)
    //    {
    //        dc.SetBrush(wxBrush(m_btBgColorActive));
    //    }
    //    else {
    //        dc.SetBrush(wxBrush(m_btBgColorNor));
    //    }
    //    dc.DrawRoundedRectangle(rect, wnd->FromDIP(4));
    //}

    if (bmp.IsOk())
        dc.DrawBitmap(bmp, bmpX, bmpY, true);


    if ((m_flags & wxAUI_TB_TEXT) && !item.GetLabel().empty())
    {
        dc.DrawText(item.GetLabel(), textX, textY);
    }
}
//
//TitleBar::TitleBar(wxWindow* parent)
//    : wxControl(parent)
//{
//#ifdef __WINDOWS__
//    SetDoubleBuffered(true);
//#endif //__WINDOWS__
//
//    m_sizer = new wxBoxSizer(wxHORIZONTAL);
//    this->SetSizer(m_sizer);
//}
//
//void TitleBar::Rescale()
//{
//    m_menuBar.Rescale();
//}


ACTopbar::ACTopbar(wxFrame* parent) 
    : wxAuiToolBar(parent, ID_TOP_BAR, wxDefaultPosition, wxDefaultSize, wxAUI_TB_TEXT | wxAUI_TB_HORZ_TEXT)
{ 
    Init(parent);
}

ACTopbar::ACTopbar(wxWindow* pwin, wxFrame* parent)
    : wxAuiToolBar(pwin, ID_TOP_BAR, wxDefaultPosition, wxDefaultSize, wxAUI_TB_TEXT | wxAUI_TB_HORZ_TEXT)
{ 
    Init(parent);
}

void ACTopbar::Init(wxFrame* parent) 
{
    SetArtProvider(new BBLTopbarArt());
    m_frame = parent;
    //m_skip_popup_file_menu = false;
    //m_skip_popup_edit_menu = false;
    //m_skip_popup_view_menu = false;
    //m_skip_popup_sets_menu = false;
    //m_skip_popup_help_menu = false;
    //m_skip_popup_dropdown_menu = false;

    //SetSizerAndFit();

    wxInitAllImageHandlers();

    this->AddSpacer(FromDIP(5));

    wxBitmap logo_bitmap = create_scaled_bitmap("logo", nullptr, TOPBAR_ICON_SIZE);
    wxAuiToolBarItem* logo_item = this->AddTool(ID_LOGO, "", logo_bitmap);
    logo_item->SetActive(false);
    logo_item->SetHoverBitmap(logo_bitmap);

    this->AddSpacer(FromDIP(5));

    m_file_menu_item = this->AddTool(ID_TOP_FILE_MENU, _L("File"), wxBitmap(), wxEmptyString, wxITEM_NORMAL);
    m_edit_menu_item = this->AddTool(ID_TOP_EDIT_MENU, _L("Edit"), wxBitmap(), wxEmptyString, wxITEM_NORMAL);
    m_view_menu_item = this->AddTool(ID_TOP_VIEW_MENU, _L("View"), wxBitmap(), wxEmptyString, wxITEM_NORMAL);
    m_sets_menu_item = this->AddTool(ID_TOP_SETS_MENU, _L("Settings"), wxBitmap(), wxEmptyString, wxITEM_NORMAL);
    m_help_menu_item = this->AddTool(ID_TOP_HELP_MENU, _L("Help"), wxBitmap(), wxEmptyString, wxITEM_NORMAL);

    this->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));



    m_title_item = this->AddLabel(ID_TITLE, "");
    m_title_item->SetAlignment(wxALIGN_CENTRE);
    m_title_item->SetProportion(1);

    this->AddSpacer(FromDIP(10));

#if !BBL_RELEASE_TO_PUBLIC
    /*wxBitmap m_publish_bitmap = create_scaled_bitmap("topbar_publish", nullptr, TOPBAR_ICON_SIZE);
    m_publish_item            = this->AddTool(ID_PUBLISH, "", m_publish_bitmap);
    wxBitmap m_publish_disable_bitmap = create_scaled_bitmap("topbar_publish_disable", nullptr, TOPBAR_ICON_SIZE);
    m_publish_item->SetDisabledBitmap(m_publish_disable_bitmap);
    this->AddSpacer(FromDIP(12));*/
#endif

    //this->AddSeparator();
    this->AddSpacer(FromDIP(6));

    wxBitmap iconize_bitmap = create_scaled_bitmap("software_minimization-nor", nullptr, TOPBAR_ICON_SIZE);
    wxBitmap iconize_bitmap_hover = create_scaled_bitmap("software_minimization-hover", nullptr, TOPBAR_ICON_SIZE);
    wxAuiToolBarItem* iconize_btn = this->AddTool(wxID_ICONIZE_FRAME, "", iconize_bitmap);
    iconize_btn->SetHoverBitmap(iconize_bitmap_hover);

    this->AddSpacer(FromDIP(6));

    maximize_bitmap = create_scaled_bitmap("software_maximization-nor", nullptr, TOPBAR_ICON_SIZE);
    window_bitmap = create_scaled_bitmap("software_window-nor", nullptr, TOPBAR_ICON_SIZE);
    maximize_bitmap_hover = create_scaled_bitmap("software_maximization-hover", nullptr, TOPBAR_ICON_SIZE);
    window_bitmap_hover = create_scaled_bitmap("software_window-hover", nullptr, TOPBAR_ICON_SIZE);

    if (m_frame->IsMaximized()) {
        maximize_btn = this->AddTool(wxID_MAXIMIZE_FRAME, "", window_bitmap);
        maximize_btn->SetHoverBitmap(window_bitmap_hover);
    } else {
        maximize_btn = this->AddTool(wxID_MAXIMIZE_FRAME, "", maximize_bitmap);
        maximize_btn->SetHoverBitmap(maximize_bitmap_hover);
    }

    this->AddSpacer(FromDIP(6));

    wxBitmap close_bitmap = create_scaled_bitmap("software_close-nor", nullptr, TOPBAR_ICON_SIZE);
    wxBitmap close_bitmap_hover = create_scaled_bitmap("software_close-hover", nullptr, TOPBAR_ICON_SIZE);
    wxAuiToolBarItem* close_btn = this->AddTool(wxID_CLOSE_FRAME, "", close_bitmap);
    close_btn->SetHoverBitmap(close_bitmap_hover);

    this->AddSpacer(FromDIP(6));

    Realize();
    // m_toolbar_h = this->GetSize().GetHeight();
    m_toolbar_h = FromDIP(30);

    int client_w = parent->GetClientSize().GetWidth();
    this->SetSize(client_w, m_toolbar_h);
    this->Layout();

    this->Bind(wxEVT_MOTION, &ACTopbar::OnMouseMotion, this);
    this->Bind(wxEVT_MOUSE_CAPTURE_LOST, &ACTopbar::OnMouseCaptureLost, this);
    this->Bind(wxEVT_MENU_CLOSE, &ACTopbar::OnMenuClose, this);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnFileToolItem, this, ID_TOP_FILE_MENU);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnEditToolItem, this, ID_TOP_EDIT_MENU);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnViewToolItem, this, ID_TOP_VIEW_MENU);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnSetsToolItem, this, ID_TOP_SETS_MENU);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnHelpToolItem, this, ID_TOP_HELP_MENU);
    //this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnDropdownToolItem, this, ID_TOP_DROPDOWN_MENU);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnIconize, this, wxID_ICONIZE_FRAME);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnFullScreen, this, wxID_MAXIMIZE_FRAME);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnCloseFrame, this, wxID_CLOSE_FRAME);
    this->Bind(wxEVT_LEFT_DCLICK, &ACTopbar::OnMouseLeftDClock, this);
    this->Bind(wxEVT_LEFT_DOWN, &ACTopbar::OnMouseLeftDown, this);
    this->Bind(wxEVT_LEFT_UP, &ACTopbar::OnMouseLeftUp, this);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnOpenProject, this, wxID_OPEN);
    //this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnSaveProject, this, wxID_SAVE);
    //this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnRedo, this, wxID_REDO);
    //this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnUndo, this, wxID_UNDO);
    //this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnModelStoreClicked, this, ID_MODEL_STORE);
    //this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACTopbar::OnPublishClicked, this, ID_PUBLISH);
}

ACTopbar::~ACTopbar()
{
    m_file_menu_item = nullptr;
    m_edit_menu_item = nullptr;
    m_view_menu_item = nullptr;
    m_sets_menu_item = nullptr;
    m_help_menu_item = nullptr;
    //m_dropdown_menu_item = nullptr;
    m_file_menu = nullptr;
}

void ACTopbar::OnOpenProject(wxAuiToolBarEvent& event)
{
    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
    Plater* plater = main_frame->plater();
    plater->load_project();
}
//
//void ACTopbar::OnSaveProject(wxAuiToolBarEvent& event)
//{
//    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
//    Plater* plater = main_frame->plater();
//    plater->save_project();
//}
//
//void ACTopbar::OnUndo(wxAuiToolBarEvent& event)
//{
//    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
//    Plater* plater = main_frame->plater();
//    plater->undo();
//}
//
//void ACTopbar::OnRedo(wxAuiToolBarEvent& event)
//{
//    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
//    Plater* plater = main_frame->plater();
//    plater->redo();
//}
//
//void ACTopbar::EnableUndoRedoItems()
//{
//    this->EnableTool(m_undo_item->GetId(), true);
//    this->EnableTool(m_redo_item->GetId(), true);
//    Refresh();
//}
//
//void ACTopbar::DisableUndoRedoItems()
//{
//    this->EnableTool(m_undo_item->GetId(), false);
//    this->EnableTool(m_redo_item->GetId(), false);
//    Refresh();
//}

void ACTopbar::SaveNormalRect()
{
    m_normalRect = m_frame->GetRect();
}

void ACTopbar::OnModelStoreClicked(wxAuiToolBarEvent& event)
{
    //GUI::wxGetApp().load_url(wxString(wxGetApp().app_config->get_web_host_url() + MODEL_STORE_URL));
}

//void ACTopbar::OnPublishClicked(wxAuiToolBarEvent& event)
//{
//    if (GUI::wxGetApp().plater()->model().objects.empty()) return;
//
//    if (!wxGetApp().is_user_login()) return;
//
//    wxGetApp().plater()->show_publish_dialog();
//}

void ACTopbar::SetFileMenu(wxMenu* file_menu)
{
    m_file_menu = file_menu;
}

void ACTopbar::SetEditMenu(wxMenu* edit_menu) 
{ 
    m_edit_menu = edit_menu;
}

void ACTopbar::SetViewMenu(wxMenu* view_menu) 
{
    m_view_menu = view_menu;
}

void ACTopbar::SetSetsMenu(wxMenu* sets_menu) 
{
    m_sets_menu = sets_menu;
}

void ACTopbar::SetHelpMenu(wxMenu* help_menu) 
{ 
    m_help_menu = help_menu;
}

//void ACTopbar::AddDropDownSubMenu(wxMenu* sub_menu, const wxString& title)
//{
//    m_top_menu.AppendSubMenu(sub_menu, title);
//}
//
//void ACTopbar::AddDropDownMenuItem(wxMenuItem* menu_item)
//{
//    m_top_menu.Append(menu_item);
//}

wxMenu* ACTopbar::GetTopMenu()
{
    return &m_top_menu;
}

void ACTopbar::SetTitle(wxString title)
{
    wxAuiToolBarItem* item = this->FindTool(ID_TITLE);
    wxSize itemSize = item->GetWindow()->GetSize();

    wxGCDC dc(this);
    title = wxControl::Ellipsize(title, dc, wxELLIPSIZE_END, itemSize.GetWidth());

    m_title_item->SetLabel(title);

    m_title_item->SetAlignment(wxALIGN_CENTRE);

    this->Layout();
    this->Refresh();
}

void ACTopbar::SetMaximizedSize()
{
    maximize_btn->SetBitmap(maximize_bitmap);
    maximize_btn->SetHoverBitmap(maximize_bitmap_hover);
}

void ACTopbar::SetWindowSize()
{
    maximize_btn->SetBitmap(window_bitmap);
    maximize_btn->SetHoverBitmap(window_bitmap_hover);
}

void ACTopbar::UpdateToolbarWidth(int width)
{
    this->SetSize(width, m_toolbar_h);
}

void ACTopbar::Rescale() {
    int em = em_unit(this);
    wxAuiToolBarItem* item;

    item = this->FindTool(ID_LOGO);
    wxBitmap logo_bitmap = create_scaled_bitmap("logo", nullptr, TOPBAR_ICON_SIZE);
    item->SetBitmap(logo_bitmap);
    item->SetHoverBitmap(logo_bitmap);

    item = this->FindTool(wxID_ICONIZE_FRAME);
    item->SetBitmap(create_scaled_bitmap("software_minimization-nor", this, TOPBAR_ICON_SIZE));
    item->SetHoverBitmap(create_scaled_bitmap("software_minimization-hover", this, TOPBAR_ICON_SIZE));

    item = this->FindTool(wxID_MAXIMIZE_FRAME);
    maximize_bitmap = create_scaled_bitmap("software_maximization-nor", this, TOPBAR_ICON_SIZE);
    window_bitmap   = create_scaled_bitmap("software_window-nor", this, TOPBAR_ICON_SIZE);
    maximize_bitmap_hover = create_scaled_bitmap("software_maximization-hover", nullptr, TOPBAR_ICON_SIZE);
    window_bitmap_hover = create_scaled_bitmap("software_window-hover", nullptr, TOPBAR_ICON_SIZE);
    if (m_frame->IsMaximized()) {
        item->SetBitmap(window_bitmap);
        item->SetHoverBitmap(window_bitmap_hover);
    }
    else {
        item->SetBitmap(maximize_bitmap);
        item->SetHoverBitmap(maximize_bitmap_hover);
    }

    item = this->FindTool(wxID_CLOSE_FRAME);
    item->SetBitmap(create_scaled_bitmap("software_close-nor", this, TOPBAR_ICON_SIZE));
    item->SetBitmap(create_scaled_bitmap("software_close-hover", this, TOPBAR_ICON_SIZE));

    Realize();
}

void ACTopbar::OnIconize(wxAuiToolBarEvent& event)
{
    m_frame->Iconize();
}

void ACTopbar::OnFullScreen(wxAuiToolBarEvent& event)
{
    if (m_frame->IsMaximized()) {
        m_frame->Restore();
    }
    else {
        wxDisplay display(this);
        auto      size = display.GetClientArea().GetSize();
        m_frame->SetMaxSize(size + wxSize{16, 16});
        m_normalRect = m_frame->GetRect();
        m_frame->Maximize();
    }
}

void ACTopbar::OnCloseFrame(wxAuiToolBarEvent& event)
{
    m_frame->Close();
}

void ACTopbar::OnMouseLeftDClock(wxMouseEvent& mouse)
{
    wxPoint mouse_pos = ::wxGetMousePosition();
    // check whether mouse is not on any tool item
    if (this->FindToolByCurrentPosition() != NULL &&
        this->FindToolByCurrentPosition() != m_title_item) {
        mouse.Skip();
        return;
    }
#ifdef __W1XMSW__
    ::PostMessage((HWND) m_frame->GetHandle(), WM_NCLBUTTONDBLCLK, HTCAPTION, MAKELPARAM(mouse_pos.x, mouse_pos.y));
    return;
#endif //  __WXMSW__

    if (m_frame->IsMaximized()) {
        m_frame->Restore();
    }
    else {
        wxDisplay display(this);
        auto      size = display.GetClientArea().GetSize();
        m_frame->SetMaxSize(size + wxSize{16, 16});
        m_normalRect = m_frame->GetRect();
        m_frame->Maximize();
    }
}

void ACTopbar::OnFileToolItem(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(evt.GetEventObject());

    tb->SetToolSticky(evt.GetId(), true);

        this->PopupMenu(m_file_menu, wxPoint(m_file_menu_item->GetSizerItem()->GetPosition().x, this->GetSize().GetHeight() - 2));
    //if (!m_skip_popup_file_menu) {
    //}
    //else {
    //    m_skip_popup_file_menu = false;
    //}

    // make sure the button is "un-stuck"
    tb->SetToolSticky(evt.GetId(), false);
}

void ACTopbar::OnEditToolItem(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(evt.GetEventObject());

    tb->SetToolSticky(evt.GetId(), true);

        this->PopupMenu(m_edit_menu, wxPoint(m_edit_menu_item->GetSizerItem()->GetPosition().x, this->GetSize().GetHeight() - 2));
    //if (!m_skip_popup_edit_menu) {
    //}
    //else {
    //    m_skip_popup_edit_menu = false;
    //}

    // make sure the button is "un-stuck"
    tb->SetToolSticky(evt.GetId(), false);
}

void ACTopbar::OnViewToolItem(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(evt.GetEventObject());

    tb->SetToolSticky(evt.GetId(), true);

        this->PopupMenu(m_view_menu, wxPoint(m_view_menu_item->GetSizerItem()->GetPosition().x, this->GetSize().GetHeight() - 2));
    //if (!m_skip_popup_view_menu) {
    //}
    //else {
    //    m_skip_popup_view_menu = false;
    //}

    // make sure the button is "un-stuck"
    tb->SetToolSticky(evt.GetId(), false);
}

void ACTopbar::OnSetsToolItem(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(evt.GetEventObject());

    tb->SetToolSticky(evt.GetId(), true);

        this->PopupMenu(m_sets_menu, wxPoint(m_sets_menu_item->GetSizerItem()->GetPosition().x, this->GetSize().GetHeight() - 2));
    //if (!m_skip_popup_sets_menu) {

    //}
    //else {
    //    m_skip_popup_sets_menu = false;
    //}

    // make sure the button is "un-stuck"
    tb->SetToolSticky(evt.GetId(), false);
}

void ACTopbar::OnHelpToolItem(wxAuiToolBarEvent& evt)
{
    wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(evt.GetEventObject());

    tb->SetToolSticky(evt.GetId(), true);

        this->PopupMenu(m_help_menu, wxPoint(m_help_menu_item->GetSizerItem()->GetPosition().x, this->GetSize().GetHeight() - 2));
    //if (!m_skip_popup_help_menu) {
    //}
    //else {
    //    m_skip_popup_help_menu = false;
    //}

    // make sure the button is "un-stuck"
    tb->SetToolSticky(evt.GetId(), false);
}


//void ACTopbar::OnDropdownToolItem(wxAuiToolBarEvent& evt)
//{
//    wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(evt.GetEventObject());
//
//    tb->SetToolSticky(evt.GetId(), true);
//
//    if (!m_skip_popup_dropdown_menu) {
//        PopupMenu(&m_top_menu, wxPoint(0, this->GetSize().GetHeight() - 2));
//    }
//    else {
//        m_skip_popup_dropdown_menu = false;
//    }
//
//    // make sure the button is "un-stuck"
//    tb->SetToolSticky(evt.GetId(), false);
//}

void ACTopbar::OnMouseLeftDown(wxMouseEvent& event)
{
    wxPoint mouse_pos = ::wxGetMousePosition();
    wxPoint frame_pos = m_frame->GetScreenPosition();
    m_delta = mouse_pos - frame_pos;

    if (FindToolByCurrentPosition() == NULL 
        || this->FindToolByCurrentPosition() == m_title_item)
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

void ACTopbar::OnMouseLeftUp(wxMouseEvent& event)
{
    wxPoint mouse_pos = ::wxGetMousePosition();
    if (HasCapture())
    {
        ReleaseMouse();
    }

    event.Skip();
}

void ACTopbar::OnMouseMotion(wxMouseEvent& event)
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

void ACTopbar::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
}

void ACTopbar::OnMenuClose(wxMenuEvent& event)
{
    wxAuiToolBarItem* item = this->FindToolByCurrentPosition();

    //if (item == m_file_menu_item ) m_skip_popup_file_menu = true;
    //if (item == m_edit_menu_item ) m_skip_popup_edit_menu = true;
    //if (item == m_view_menu_item ) m_skip_popup_view_menu = true;
    //if (item == m_sets_menu_item ) m_skip_popup_sets_menu = true;
    //if (item == m_help_menu_item ) m_skip_popup_help_menu = true;

    //else if (item == m_dropdown_menu_item) {
    //    m_skip_popup_dropdown_menu = true;
    //}
}

wxAuiToolBarItem* ACTopbar::FindToolByCurrentPosition()
{
    wxPoint mouse_pos = ::wxGetMousePosition();
    wxPoint client_pos = this->ScreenToClient(mouse_pos);
    return this->FindToolByPosition(client_pos.x, client_pos.y);
}
