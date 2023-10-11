//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( CLIENTMODE_SONORMAL_H )
#define CLIENTMODE_SONORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeSONormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeSONormal, ClientModeShared );

	ClientModeSONormal();
	~ClientModeSONormal();

	virtual void	Init();
	virtual int		GetDeathMessageStartHeight( void );
	void PostRenderVGui();
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

extern ClientModeSONormal* GetClientModeSONormal();

#endif // CLIENTMODE_HLNORMAL_H
