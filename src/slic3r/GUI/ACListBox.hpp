#ifndef slic3r_GUI_ACListBox_hpp_
#define slic3r_GUI_ACListBox_hpp_

#include <wx/textctrl.h>
#include "ACStaticBox.hpp"
#include "ACButton.hpp"
#include <vector>
#include <wx/event.h>

wxDECLARE_EVENT(EVT_ACLISTBOX_SEL_CHANGED, wxCommandEvent);

class ACListBox : public wxNavigationEnabled<ACStaticBox>
{
    std::vector<ACButton*> m_buttons;

    ACStateColor m_bgColor_nor;
    ACStateColor m_bgColor_sel;

    ACStateColor m_textColor_nor;
    ACStateColor m_textColor_sel;

    static const int ListBoxItemsMargin = 12;
    static const int ListBoxItemsGap = 10;

    static const int ListItemWidth = 220;
    static const int ListItemHeight = 36;

    wxBoxSizer* m_mainSizer {nullptr};

    int m_currentSelIndex = -1;

    std::function<void(int)> m_cb_ItemSelected = nullptr;
public:
    ACListBox();

    ACListBox(wxWindow* parent,
             wxWindowID      id        = wxID_ANY,
             const wxPoint & pos       = wxDefaultPosition,
             const wxSize &  size      = wxDefaultSize, 
             long style = 0);

public:
    void Create(wxWindow *     parent,
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = 0);

    void SelectItem(int id);

    int AppendItem(const wxString& text, const wxString& imgName);

    //void AssignImageList(wxImageList* imgList);

    int GetButtonsCount() { return m_buttons.size(); }

    virtual void Rescale();

    //virtual bool Enable(bool enable = true) override;

    //virtual void SetMinSize(const wxSize& size) override;

    int GetSelection();

    wxString GetItemText(int index);

    void DeleteChildren();

    void onItemSelected(std::function<void(int)> cb_ItemSelected);
protected:

    //virtual void DoSetSize(
    //    int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO);
    //virtual bool DoNavigateIn(int flags = wxNavigationKeyEvent::IsForward) wxOVERRIDE;
private:
    //void paintEvent(wxPaintEvent& evt);

    //void render(wxDC& dc);

    void messureSize();

    //DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_TextInput_hpp_
