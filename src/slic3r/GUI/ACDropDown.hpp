#ifndef slic3r_GUI_ACDropDown_hpp_
#define slic3r_GUI_ACDropDown_hpp_

#include <wx/stattext.h>
#include "wxExtensions.hpp"
#include "ACStateHandler.hpp"
#include "wx/popupwin.h"

#define DD_NO_CHECK_ICON    0x0001
#define DD_NO_TEXT          0x0002
#define DD_STYLE_MASK       0x0003

wxDECLARE_EVENT(EVT_DISMISS, wxCommandEvent);

class ACDropDown : public wxPopupTransientWindow
{
    std::vector<wxString> &       texts;
    std::vector<wxBitmap> &     icons;
    bool                          need_sync  = false;
    int                         selection = -1;
    int                         hover_item = -1;

    double radius = 0;
    bool   use_content_width = false;
    bool   align_icon        = false;
    bool   text_off          = false;

    wxSize textSize;
    wxSize iconSize;
    wxSize rowSize;

    ACStateHandler state_handler;
    ACStateColor   text_color;
    ACStateColor   border_color;
    ACStateColor   selector_border_color;
    ACStateColor   selector_background_color;
    ScalableBitmap check_bitmap;

    bool pressedDown = false;
    boost::posix_time::ptime dismissTime;
    wxPoint                  offset; // x not used
    wxPoint                  dragStart;

public:
    ACDropDown(std::vector<wxString> &texts,
             std::vector<wxBitmap> &icons);
    
    ACDropDown(wxWindow *     parent,
             std::vector<wxString> &texts,
             std::vector<wxBitmap> &icons,
             long           style     = 0);
    
    void Create(wxWindow *     parent,
             long           style     = 0);
    
public:
    void Invalidate(bool clear = false);

    int GetSelection() const { return selection; }

    void SetSelection(int n);

    wxString GetValue() const;
    void     SetValue(const wxString &value);

public:
    void SetCornerRadius(double radius);

    void SetBorderColor(ACStateColor const & color);

    void SetSelectorBorderColor(ACStateColor const & color);

    void SetSelectorBackgroundColor(ACStateColor const &color);

    void SetTextColor(ACStateColor const &color);

    void SetUseContentWidth(bool use);

    void SetAlignIcon(bool align);
    
    virtual bool HasTransparentBackground() { return true; }
    virtual bool IsTransparentBackgroundSupported( wxString *reason = NULL) const { return true; }
    virtual bool CanSetTransparent() { return true; }
    //virtual bool SetTransparent(wxByte WXUNUSED(alpha)) { return true; }
public:
    void Rescale();

    bool HasDismissLongTime();
    
protected:
    void OnDismiss() override;

private:
    void paintEvent(wxPaintEvent& evt);
    void paintNow();

    void render(wxDC& dc);

    friend class ACComboBox;
    void messureSize();
    void autoPosition();

    // some useful events
    void mouseDown(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent &event);
    void mouseCaptureLost(wxMouseCaptureLostEvent &event);
    void mouseMove(wxMouseEvent &event);
    void mouseWheelMoved(wxMouseEvent &event);

    void sendDropDownEvent();


    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_ACDropDown_hpp_
