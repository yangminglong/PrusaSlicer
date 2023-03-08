#ifndef slic3r_GUI_ACSpinInput_hpp_
#define slic3r_GUI_ACSpinInput_hpp_

#include <wx/textctrl.h>
#include "ACStaticBox.hpp"

class ACButton;

class ACSpinInput : public wxNavigationEnabled<ACStaticBox>
{
    wxSize labelSize;
    ACStateColor   label_color;
    ACStateColor   text_color;
    wxTextCtrl * text_ctrl;
    ACButton * ACButton_inc;
    ACButton * ACButton_dec;
    wxTimer timer;

    int val;
    int min;
    int max;
    int delta;

    static const int ACSpinInputWidth = 200;
    static const int ACSpinInputHeight = 50;

public:
    ACSpinInput();

    ACSpinInput(wxWindow *     parent,
              wxString       text,
              wxString       label = "",
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = 0,
              int min = 0, int max = 100, int initial = 0);

    void Create(wxWindow *     parent,
              wxString       text,
              wxString       label   = "",
              const wxPoint &pos     = wxDefaultPosition,
              const wxSize & size    = wxDefaultSize,
              long           style   = 0,
              int            min     = 0,
              int            max     = 100,
              int            initial = 0);

    void SetCornerRadius(double radius);

    void SetLabel(const wxString &label) wxOVERRIDE;

    void SetLabelColor(ACStateColor const &color);

    void SetTextColor(ACStateColor const &color);

    void SetSize(wxSize const &size);

    void Rescale();

    virtual bool Enable(bool enable = true) wxOVERRIDE;

    wxTextCtrl * GetTextCtrl() { return text_ctrl; }
    wxTextCtrl * GetText() { return text_ctrl; }

    void SetSelection(long from, long to) { text_ctrl->SetSelection(from, to); }

    void SetValue(const wxString &text);

    void SetValue (int value);

    int GetValue () const;

    void SetRange(int min, int max);

protected:
    void DoSetToolTipText(wxString const &tip) override;

private:
    //void paintEvent(wxPaintEvent& evt);

    //void render(wxDC& dc);

    void messureSize();

    ACButton *createACButton(bool inc);

    // some useful events
    void mouseWheelMoved(wxMouseEvent& event);
    void keyPressed(wxKeyEvent& event);
    void onTimer(wxTimerEvent &evnet);
    void onTextLostFocus(wxEvent &event);
    void onTextEnter(wxCommandEvent &event);

    void sendSpinEvent();

    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_ACSpinInput_hpp_
