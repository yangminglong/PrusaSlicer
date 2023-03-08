#ifndef slic3r_GUI_ACButton_hpp_
#define slic3r_GUI_ACButton_hpp_

#include "wxExtensions.hpp"
#include "ACStaticBox.hpp"

class ACButton : public ACStaticBox
{
    wxSize textSize;
    wxSize minSize; // set by outer
    wxSize paddingSize;
    wxSize m_offset;
    ScalableBitmap active_icon;
    ScalableBitmap inactive_icon;

    ACStateColor   text_color;

    bool pressedDown = false;
    bool m_selected  = true;
    bool canFocus  = true;
    bool m_alignLeft = false;

    static const int buttonWidth = 200;
    static const int buttonHeight = 50;



public:
    ACButton();

    ACButton(wxWindow* parent, wxString text, wxString icon = "", long style = 0, int iconSize = 0);

    bool Create(wxWindow* parent, wxString text, wxString icon = "", long style = 0, int iconSize = 0);

    void SetLabel(const wxString& label) override;

    void SetIcon(const wxString& icon);

    void SetInactiveIcon(const wxString& icon);

    void SetMinSize(const wxSize& size) override;
    
    void SetPaddingSize(const wxSize& size);

    void SetOffset(const wxSize& size);
    
    void SetTextColor(ACStateColor const &color);

    void SetTextColorNormal(wxColor const &color);

    void SetSelected(bool selected = true) { m_selected = selected; }

    void SetAlignLeft(bool align) { m_alignLeft = align; }

    bool Enable(bool enable = true) override;

    void SetCanFocus(bool canFocus) override;

    void Rescale();

protected:
#ifdef __WIN32__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

    bool AcceptsFocus() const override;

private:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void messureSize();

    // some useful events
    void mouseDown(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void mouseCaptureLost(wxMouseCaptureLostEvent &event);
    void keyDownUp(wxKeyEvent &event);

    void sendButtonEvent();

    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_ACButton_hpp_
