//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BUYMENUNEWMIL_H
#define BUYMENUNEWMIL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <UtlVector.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <game/client/iviewport.h>
#include <vgui_controls/ImagePanel.h>

#include <vgui_controls/Button.h>

#include "buymenu_new.h"

namespace vgui
{
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CBuyMenuNewMil : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CBuyMenuNewMil, vgui::Frame );

public:
	CBuyMenuNewMil(IViewPort *pViewPort);
	CBuyMenuNewMil(IViewPort *pViewPort, const char *panelName );
	virtual ~CBuyMenuNewMil();

	virtual const char *GetName( void ) { return PANEL_BUYMIL; }
	virtual void SetData(KeyValues *data);
	virtual void Reset();
	virtual void Update() {};
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	void OnThink();

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

protected:

	void UpdateShownWeaponModel( int buttonToRead );
	const char *GetWeaponFromButton( int buttonToRead );

	virtual vgui::Panel *CreateControlByName(const char *controlName);

	//vgui2 overrides
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// helper functions
	void SetLabelText(const char *textEntryName, const char *text);
	void SetVisibleButton(const char *textEntryName, bool state);

	// command callbacks
	void OnCommand( const char *command );

	IViewPort	*m_pViewPort;
	ButtonCode_t m_iScoreBoardKey;
	int			m_iTeam;
	vgui::EditablePanel *m_pPanel;

	CUtlVector< vgui::Button * > m_Buttons;
};

#endif // BUYMENUNEWMIL_H