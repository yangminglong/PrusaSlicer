#pragma once

#include "wx/wxprec.h"
#include "wx/aui/auibar.h"

using namespace Slic3r::GUI;

class ACToolBar : public wxAuiToolBar
{
public:
    ACToolBar(wxWindow* pwin, wxFrame* parent);
    ACToolBar(wxFrame* parent);
    void Init(wxFrame *parent);
    ~ACToolBar();
    void UpdateToolbarWidth(int width);
    void Rescale();
 
    void OnAddToPlate(wxAuiToolBarEvent& event);
    void OnSaveProject(wxAuiToolBarEvent& event);
    void OnUndo(wxAuiToolBarEvent& event);
    void OnRedo(wxAuiToolBarEvent& event);
    void OnOpenConfigDialog(wxAuiToolBarEvent& event);

    //void EnableUndoRedoItems();
    //void DisableUndoRedoItems();

private:
    wxFrame* m_frame;

    wxAuiToolBarItem* m_undo_item;
    wxAuiToolBarItem* m_redo_item;
    wxAuiToolBarItem* m_config_item;


    int m_toolbar_h;

};

