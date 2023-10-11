//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// battery.cpp
//
// implementation of CHudMoney class
//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "view.h"
#include "hl2mp_gamerules.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudMoney : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudMoney, CHudNumericDisplay );

public:
	CHudMoney( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink( void );
};

DECLARE_HUDELEMENT( CHudMoney );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudMoney::CHudMoney( const char *pElementName ) : CHudElement( pElementName ), CHudNumericDisplay(NULL, "HudMoney")
{
	SetProportional(true);
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMoney::Init( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMoney::Reset( void )
{
	SetLabelText(L"POINTS");
	this->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMoney::VidInit( void )
{
	Reset();
}

void CHudMoney::OnThink( void )
{
	// for the lulz!!! =D
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MoneyHONEY");

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
		SetDisplayValue(pPlayer->m_iMoney);
}