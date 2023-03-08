#include "ACToolbar.hpp"
#include "wx/artprov.h"
#include "wx/aui/framemanager.h"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "GUI.hpp"
#include "wxExtensions.hpp"
#include "Plater.hpp"
#include "MainFrame.hpp"
#include "wx/defs.h"
//#include "WebViewDialog.hpp"
//#include "PartPlate.hpp"
#include "ACDefines.h"


using namespace Slic3r;


class ACToolBarArt : public wxAuiDefaultToolBarArt
{
public:
    virtual void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) wxOVERRIDE;
    virtual void DrawButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect) wxOVERRIDE;
private:

};

void ACToolBarArt::DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
    //dc.SetBrush(wxBrush(wxColour(38, 46, 48))); //
    dc.SetBrush(wxBrush(wxColour(255,255,255))); //55, 66, 84  255, 255, 255
    dc.SetPen(wxPen(wxColour(255, 255, 255))); //55, 66, 84
    wxRect clipRect = rect;
    clipRect.y -= 8;
    clipRect.height += 8;
    dc.SetClippingRegion(clipRect);
    dc.DrawRectangle(rect);
    dc.DestroyClippingRegion();
}

void ACToolBarArt::DrawButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect)
{
    int textWidth = 0, textHeight = 0;

    int btHeight = 32;

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

    bool can_undo = true; //wxGetApp().plater()->can_undo();
    bool can_redo = true; //wxGetApp().plater()->can_redo();

    bool isButtonDisable = false;

    if (item.GetId() == wxID_REDO && !can_redo ||
        item.GetId() == wxID_UNDO && !can_undo ||
        item.GetState() & wxAUI_BUTTON_STATE_DISABLED) {
        isButtonDisable = true;
    }

    const wxBitmap& bmp =  isButtonDisable ? item.GetDisabledBitmap() : item.GetBitmap();

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
        int iconPadding = 3;
        int itemSpaccing = 3;
        bmpX = rect.x + wnd->FromDIP(iconPadding);

        bmpY = rect.y +
            (rect.height / 2) -
            (bmpSize.y / 2);

        textX = bmpX + wnd->FromDIP(itemSpaccing) + bmpSize.x;
        textY = rect.y +
            (rect.height / 2) -
            (textHeight / 2);
    }
    if (item.GetId() == wxID_ADD || item.GetId() == ID_CONFIGURATION) {
        wxColour btBgPen   = (int)wxID_ADD == item.GetId() ? wxColour( 57, 134, 255, 1) : wxColour(188, 196, 212, 0);
        wxColour btBgBrush = (int)wxID_ADD == item.GetId() ? wxColour( 57, 134, 255, 1) : wxColour(255, 255, 255, 0);

        dc.SetPen(wxPen(btBgPen));
        dc.SetBrush(wxBrush(btBgBrush));
        dc.DrawRoundedRectangle(wxRect(rect.x, rect.y+(rect.height-btHeight)/2, rect.width, rect.height), wnd->FromDIP(8));  

        if (bmp.IsOk())
            dc.DrawBitmap(bmp, bmpX, bmpY, false);

        wxColour textFgColor = (int)wxID_ADD == item.GetId() ? wxColour(255, 255, 255) : wxColour(67, 87, 123);

        dc.SetTextForeground(textFgColor);

        if (item.GetState() & wxAUI_BUTTON_STATE_DISABLED)
        {
            dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
        }

        if ((m_flags & wxAUI_TB_TEXT) && !item.GetLabel().empty())
        {
            dc.DrawText(item.GetLabel(), textX, textY);
        }    
    } else {
        if (bmp.IsOk())
            dc.DrawBitmap(bmp, bmpX, bmpY, false);
    }


}



ACToolBar::ACToolBar(wxFrame* parent) 
    : wxAuiToolBar(parent, ID_TOOL_BAR, wxDefaultPosition, wxDefaultSize, wxAUI_TB_TEXT | wxAUI_TB_HORZ_TEXT)
    , m_toolbar_h(48)
{ 
    Init(parent);
}

ACToolBar::ACToolBar(wxWindow* pwin, wxFrame* parent)
    : wxAuiToolBar(pwin, ID_TOOL_BAR, wxDefaultPosition, wxDefaultSize, wxAUI_TB_TEXT | wxAUI_TB_HORZ_TEXT) 
    , m_toolbar_h(48)
{ 
    Init(parent);
}

void ACToolBar::Init(wxFrame* parent) 
{
    SetArtProvider(new ACToolBarArt());
    m_frame = parent;

    wxInitAllImageHandlers();

    this->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));

    wxBitmap add_bitmap = create_scaled_bitmap("icon-import", nullptr, 16); //57, 134, 255
    wxAuiToolBarItem* addToPlate_btn = this->AddTool(wxID_ADD, "Import", add_bitmap);

    this->AddSpacer(FromDIP(10));
    this->AddSeparator();
    this->AddSpacer(FromDIP(10));

    wxBitmap save_bitmap = create_scaled_bitmap("icon-save-nor", nullptr, 32);
    wxBitmap save_bitmap_inactive = create_scaled_bitmap("icon-save-disable", nullptr, 32);
    wxBitmap save_bitmap_hover = create_scaled_bitmap("icon-save-hover", nullptr, 32);
    wxAuiToolBarItem* save_btn = this->AddTool(wxID_SAVE, "", save_bitmap);
    save_btn->SetDisabledBitmap(save_bitmap_inactive);
    save_btn->SetHoverBitmap(save_bitmap_hover);

    this->AddSpacer(FromDIP(10));

    wxBitmap undo_bitmap = create_scaled_bitmap("icon-undo-nor", nullptr, 32);
    wxBitmap undo_bitmap_inactive = create_scaled_bitmap("icon-undo-disable", nullptr, 32);
    wxBitmap undo_bitmap_hover = create_scaled_bitmap("icon-undo-hover", nullptr, 32);
    m_undo_item = this->AddTool(wxID_UNDO, "", undo_bitmap);
    m_undo_item->SetDisabledBitmap(undo_bitmap_inactive);
    m_undo_item->SetHoverBitmap(undo_bitmap_hover);

    this->AddSpacer(FromDIP(10));

    wxBitmap redo_bitmap = create_scaled_bitmap("icon-redo-nor", nullptr, 32);
    wxBitmap redo_bitmap_inactive = create_scaled_bitmap("icon-redo-disable", nullptr, 32);
    wxBitmap redo_bitmap_hover = create_scaled_bitmap("icon-redo-hover", nullptr, 32);
    m_redo_item = this->AddTool(wxID_REDO, "", redo_bitmap);
    m_redo_item->SetDisabledBitmap(redo_bitmap_inactive);
    m_redo_item->SetHoverBitmap(redo_bitmap_hover);

    this->AddStretchSpacer(1);

    wxBitmap config_bitmap = create_scaled_bitmap("icon-configuration_manage-nor", nullptr, 16);
    wxBitmap config_bitmap_inactive = create_scaled_bitmap("icon-configuration_manage-disable", nullptr, 16);
    m_config_item = this->AddTool(ID_CONFIGURATION, "Configuration Manage", config_bitmap);
    m_config_item->SetDisabledBitmap(config_bitmap_inactive);

    Realize();

    int client_w = parent->GetClientSize().GetWidth();
    this->SetSize(client_w, FromDIP(m_toolbar_h));

    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACToolBar::OnAddToPlate, this, wxID_ADD);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACToolBar::OnSaveProject, this, wxID_SAVE);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACToolBar::OnRedo, this, wxID_REDO);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACToolBar::OnUndo, this, wxID_UNDO);
    this->Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, &ACToolBar::OnOpenConfigDialog, this, ID_CONFIGURATION);
}

ACToolBar::~ACToolBar()
{
    //m_file_menu_item = nullptr;
    //m_edit_menu_item = nullptr;
    //m_view_menu_item = nullptr;
    //m_sets_menu_item = nullptr;
    //m_help_menu_item = nullptr;
    ////m_dropdown_menu_item = nullptr;
    //m_file_menu = nullptr;
}
//
//void ACToolBar::OnOpenProject(wxAuiToolBarEvent& event)
//{
//    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
//    Plater* plater = main_frame->plater();
//    plater->load_project();
//}

void ACToolBar::OnAddToPlate(wxAuiToolBarEvent& event)
{
    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
    Plater* plater = main_frame->plater();
    plater->add_model();
}

void ACToolBar::OnSaveProject(wxAuiToolBarEvent& event)
{
    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
    Plater* plater = main_frame->plater();
    //plater->save_project();
}

void ACToolBar::OnUndo(wxAuiToolBarEvent& event)
{
    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
    Plater* plater = main_frame->plater();
    plater->undo();
}

void ACToolBar::OnRedo(wxAuiToolBarEvent& event)
{
    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
    Plater* plater = main_frame->plater();
    plater->redo();
}

void ACToolBar::OnOpenConfigDialog(wxAuiToolBarEvent& event)
{
    MainFrame* main_frame = dynamic_cast<MainFrame*>(m_frame);
    Plater* plater = main_frame->plater();
    //plater->openConfigDialog();
}

void ACToolBar::UpdateToolbarWidth(int width)
{
    this->SetSize(width, FromDIP(m_toolbar_h));
}

void ACToolBar::Rescale() {
    int em = em_unit(this);
    wxAuiToolBarItem* item;
  
    item = this->FindTool(wxID_ADD);
    item->SetBitmap(create_scaled_bitmap("icon-import", this, 16));

    item = this->FindTool(wxID_SAVE);
    item->SetBitmap(create_scaled_bitmap("icon-save-nor", this, 32));
    item->SetDisabledBitmap(create_scaled_bitmap("icon-save-disable", this, 32));
    item->SetHoverBitmap(create_scaled_bitmap("icon-save-hover", this, 32));

    item = this->FindTool(wxID_UNDO);
    item->SetBitmap(create_scaled_bitmap("icon-undo-nor", this, 32));
    item->SetDisabledBitmap(create_scaled_bitmap("icon-undo-disable", nullptr, 32));
    item->SetHoverBitmap(create_scaled_bitmap("icon-undo-hover", nullptr, 32));

    item = this->FindTool(wxID_REDO);
    item->SetBitmap(create_scaled_bitmap("icon-redo-nor", this, 32));
    item->SetDisabledBitmap(create_scaled_bitmap("icon-redo-disable", nullptr, 32));
    item->SetHoverBitmap(create_scaled_bitmap("icon-redo-hover", nullptr, 32));

    item = this->FindTool(ID_CONFIGURATION);
    item->SetBitmap(create_scaled_bitmap("icon-configuration_manage-nor", this, 16));
    item->SetDisabledBitmap(create_scaled_bitmap("icon-configuration_manage-disable", nullptr, 16));

    Realize();
}