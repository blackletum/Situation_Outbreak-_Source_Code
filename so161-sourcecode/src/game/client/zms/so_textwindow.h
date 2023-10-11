//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CSTEXTWINDOW_H
#define CSTEXTWINDOW_H
#ifdef _WIN32
#pragma once
#endif

#include "VguiTextWindow.h"
#include <spectatorgui.h>

//-----------------------------------------------------------------------------
// Purpose: displays the MOTD
//-----------------------------------------------------------------------------

class CSOTextWindow : public CTextWindow
{
private:
	DECLARE_CLASS_SIMPLE( CSOTextWindow, CTextWindow );

public:
	CSOTextWindow(IViewPort *pViewPort);
	virtual ~CSOTextWindow();

	virtual void Update();
	virtual void SetVisible(bool state);
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodePressed(vgui::KeyCode code);

protected:
	ButtonCode_t m_iScoreBoardKey;

	// Background panel -------------------------------------------------------

public:
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	bool m_backgroundLayoutFinished;

	// End background panel ---------------------------------------------------
};

class CSOSpectatorGUI : public CSpectatorGUI
{
private:
	DECLARE_CLASS_SIMPLE( CSOSpectatorGUI, CSpectatorGUI );

public:
	CSOSpectatorGUI( IViewPort *pViewPort );

	virtual void Update( void );
	virtual bool NeedsUpdate( void );

protected:
	int		m_nLastSpecMode;
	CBaseEntity	*m_nLastSpecTarget;
};



#endif // CSTEXTWINDOW_H
