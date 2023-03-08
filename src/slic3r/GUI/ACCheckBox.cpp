#include "ACCheckBox.hpp"

#include "wxExtensions.hpp"

ACCheckBox::ACCheckBox(wxWindow* parent)
	: wxBitmapToggleButton(parent, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_on(this, "checkbox-on-nor", 16)
    , m_half(this, "checkbox-half_on-nor", 16)
    , m_off(this, "checkbox-off-nor", 16)
    , m_on_disabled(this, "checkbox-on-disable", 16)
    , m_half_disabled(this, "checkbox-half_on-disable", 16)
    , m_off_disabled(this, "checkbox-off-disable", 16)
    , m_on_focused(this, "checkbox-on-focused", 16)
    , m_half_focused(this, "checkbox-half_on-focused", 16)
    , m_off_focused(this, "checkbox-off-focused", 16)
{
	//SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
	if (parent)
		SetBackgroundColour(parent->GetBackgroundColour());
	Bind(wxEVT_TOGGLEBUTTON, [this](auto& e) {
        m_half_checked = false;
        update(); 
        e.Skip(); 
        });
#ifdef __WXOSX__ // State not fully implement on MacOS
    Bind(wxEVT_SET_FOCUS, &ACCheckBox::updateBitmap, this);
    Bind(wxEVT_KILL_FOCUS, &ACCheckBox::updateBitmap, this);
    Bind(wxEVT_ENTER_WINDOW, &ACCheckBox::updateBitmap, this);
    Bind(wxEVT_LEAVE_WINDOW, &ACCheckBox::updateBitmap, this);
#endif
	SetSize(m_on.GetBmpSize());
	SetMinSize(m_on.GetBmpSize());
	update();
}

void ACCheckBox::SetValue(bool value)
{
	wxBitmapToggleButton::SetValue(value);
	update();
}

void ACCheckBox::SetHalfChecked(bool value)
{
	m_half_checked = value;
	update();
}

void ACCheckBox::Rescale()
{
    m_on.msw_rescale();
    m_half.msw_rescale();
    m_off.msw_rescale();
    m_on_disabled.msw_rescale();
    m_half_disabled.msw_rescale();
    m_off_disabled.msw_rescale();
    m_on_focused.msw_rescale();
    m_half_focused.msw_rescale();
    m_off_focused.msw_rescale();
    SetSize(m_on.GetBmpSize());
	update();
}

void ACCheckBox::update()
{
	SetBitmapLabel((m_half_checked ? m_half : GetValue() ? m_on : m_off).bmp());
    SetBitmapDisabled((m_half_checked ? m_half_disabled : GetValue() ? m_on_disabled : m_off_disabled).bmp());
#ifdef __WXMSW__
    SetBitmapFocus((m_half_checked ? m_half_focused : GetValue() ? m_on_focused : m_off_focused).bmp());
#endif
    SetBitmapCurrent((m_half_checked ? m_half_focused : GetValue() ? m_on_focused : m_off_focused).bmp());
#ifdef __WXOSX__
    wxCommandEvent e(wxEVT_UPDATE_UI);
    updateBitmap(e);
#endif
}

#ifdef __WXMSW__

ACCheckBox::State ACCheckBox::GetNormalState() const { return State_Normal; }

#endif


#ifdef __WXOSX__

bool ACCheckBox::Enable(bool enable)
{
    bool result = wxBitmapToggleButton::Enable(enable);
    if (result) {
        m_disable = !enable;
        wxCommandEvent e(wxEVT_ACTIVATE);
        updateBitmap(e);
    }
    return result;
}

wxBitmap ACCheckBox::DoGetBitmap(State which) const
{
    if (m_disable) {
        return wxBitmapToggleButton::DoGetBitmap(State_Disabled);
    }
    if (m_focus) {
        return wxBitmapToggleButton::DoGetBitmap(State_Current);
    }
    return wxBitmapToggleButton::DoGetBitmap(which);
}

void ACCheckBox::updateBitmap(wxEvent & evt)
{
    evt.Skip();
    if (evt.GetEventType() == wxEVT_ENTER_WINDOW) {
        m_focused = true;
    } else if (evt.GetEventType() == wxEVT_LEAVE_WINDOW) {
        m_focused = false;
    } else {
        if (evt.GetEventType() == wxEVT_SET_FOCUS) {
            m_focus = true;
        } else if (evt.GetEventType() == wxEVT_KILL_FOCUS) {
            m_focus = false;
        }
        wxMouseEvent e;
        if (m_focused)	
            OnEnterWindow(e);
        else
            OnLeaveWindow(e);
    }
}
	
#endif
