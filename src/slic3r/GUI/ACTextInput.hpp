#ifndef slic3r_GUI_ACTextInput_hpp_
#define slic3r_GUI_ACTextInput_hpp_

#include <wx/textctrl.h>
#include "ACStaticBox.hpp"

class ACTextInput : public wxNavigationEnabled<ACStaticBox>
{

    wxSize labelSize;
    ScalableBitmap icon;
    ACStateColor     label_color;
    ACStateColor     text_color;
    wxTextCtrl * text_ctrl;

    static const int TextInputWidth = 200;
    static const int TextInputHeight = 50;

public:
    ACTextInput();

    ACTextInput(wxWindow *     parent,
              wxString       text,
              wxString       label = "",
              wxString       icon  = "",
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = 0);

public:
    void Create(wxWindow *     parent,
              wxString       text,
              wxString       label = "",
              wxString       icon  = "",
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = 0);

    void SetCornerRadius(double radius);

    void SetLabel(const wxString& label);

    void SetValue(const wxString& lable) { SetLabel(lable); }

    void SetIcon(const wxBitmap & icon);

    void SetLabelColor(ACStateColor const &color);

    void SetTextColor(ACStateColor const &color);

    virtual void Rescale();

    virtual bool Enable(bool enable = true) override;

    virtual void SetMinSize(const wxSize& size) override;

    wxTextCtrl *GetTextCtrl() { return text_ctrl; }

    wxTextCtrl const *GetTextCtrl() const { return text_ctrl; }

    wxString GetValue() const { return  wxWindow::GetLabel(); }

    void SetEditable(bool) const { }

protected:
    virtual void OnEdit() {}

    virtual void DoSetSize(
        int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO);

    void DoSetToolTipText(wxString const &tip) override;

private:
    void paintEvent(wxPaintEvent& evt);

    void render(wxDC& dc);

    void messureSize();

    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_TextInput_hpp_
