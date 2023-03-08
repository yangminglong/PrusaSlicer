#pragma once

#include "wx/wxprec.h"
#include "wx/aui/auibar.h"

//#include "SelectMachine.hpp"
//#include "DeviceManager.hpp"


using namespace Slic3r::GUI;

class ACTopbar : public wxAuiToolBar
{
public:
    ACTopbar(wxWindow* pwin, wxFrame* parent);
    ACTopbar(wxFrame* parent);
    void Init(wxFrame *parent);
    ~ACTopbar();
    void UpdateToolbarWidth(int width);
    void Rescale();
    void OnIconize(wxAuiToolBarEvent& event);
    void OnFullScreen(wxAuiToolBarEvent& event);
    void OnCloseFrame(wxAuiToolBarEvent& event);
    void OnFileToolItem(wxAuiToolBarEvent& evt);
    void OnEditToolItem(wxAuiToolBarEvent& evt);
    void OnViewToolItem(wxAuiToolBarEvent& evt);
    void OnSetsToolItem(wxAuiToolBarEvent& evt);
    void OnHelpToolItem(wxAuiToolBarEvent& evt);
    //void OnDropdownToolItem(wxAuiToolBarEvent& evt);
    void OnMouseLeftDClock(wxMouseEvent& mouse);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMotion(wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void OnMenuClose(wxMenuEvent& event);
    void OnOpenProject(wxAuiToolBarEvent& event);
    //void OnSaveProject(wxAuiToolBarEvent& event);
    //void OnUndo(wxAuiToolBarEvent& event);
    //void OnRedo(wxAuiToolBarEvent& event);
    void OnModelStoreClicked(wxAuiToolBarEvent& event);
    //void OnPublishClicked(wxAuiToolBarEvent &event);

    wxAuiToolBarItem* FindToolByCurrentPosition();
	
    void SetFileMenu(wxMenu* file_menu);
    void SetEditMenu(wxMenu* edit_menu);
    void SetViewMenu(wxMenu* view_menu);
    void SetSetsMenu(wxMenu* sets_menu);
    void SetHelpMenu(wxMenu* help_menu);
    //void AddDropDownSubMenu(wxMenu* sub_menu, const wxString& title);
    //void AddDropDownMenuItem(wxMenuItem* menu_item);
    wxMenu *GetTopMenu();
    void SetTitle(wxString title);
    void SetMaximizedSize();
    void SetWindowSize();

    //void EnableUndoRedoItems();
    //void DisableUndoRedoItems();

    void SaveNormalRect();

private:
    wxFrame* m_frame;
    wxAuiToolBarItem* m_file_menu_item;
    wxAuiToolBarItem* m_edit_menu_item;
    wxAuiToolBarItem* m_view_menu_item;
    wxAuiToolBarItem* m_sets_menu_item;
    wxAuiToolBarItem* m_help_menu_item;
    //wxAuiToolBarItem* m_dropdown_menu_item;
    wxRect m_normalRect;
    wxPoint m_delta;
    wxMenu m_top_menu;
    wxMenu* m_file_menu;
    wxMenu* m_edit_menu;
    wxMenu* m_view_menu;
    wxMenu* m_sets_menu;
    wxMenu* m_help_menu;
    wxAuiToolBarItem* m_title_item;
    wxAuiToolBarItem* m_account_item;
    wxAuiToolBarItem* m_model_store_item;
    
    //wxAuiToolBarItem *m_publish_item;
    wxAuiToolBarItem* m_undo_item;
    wxAuiToolBarItem* m_redo_item;
    wxAuiToolBarItem* maximize_btn;

    wxBitmap maximize_bitmap;
    wxBitmap window_bitmap;
    wxBitmap maximize_bitmap_hover;
    wxBitmap window_bitmap_hover;

    int m_toolbar_h;
    bool m_skip_popup_file_menu;
    bool m_skip_popup_edit_menu;
    bool m_skip_popup_view_menu;
    bool m_skip_popup_sets_menu;
    bool m_skip_popup_help_menu;
    //bool m_skip_popup_dropdown_menu;
};

//
//class TitleBar : public wxControl
//{
//public:
//    TitleBar(wxWindow* parent);
//    ~TitleBar() {}
//
//
//    void Rescale();
//    ACTopbar* GetMenubar() { return &m_menuBar; }
//private:
//    ACTopbar m_menuBar;
//    wxBoxSizer* m_sizer;
//
//};