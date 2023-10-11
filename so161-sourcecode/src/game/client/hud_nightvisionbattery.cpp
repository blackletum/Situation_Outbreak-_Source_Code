//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_nightvisionbattery.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// S:O - Tells us whether or not nightvision is enabled! (yay)
extern ConVar shader_nightvision;

DECLARE_HUDELEMENT( CHudNightvisionPower );

#define NIGHTVISIONPOWER_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudNightvisionPower::CHudNightvisionPower( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudNightvisionPower" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNightvisionPower::Init( void )
{
	m_bPowerWasLow = true;
	m_flSuitPower = 100.0f;
	m_flSuitPowerOld = 0.0f;
	m_fNightvisionPower = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNightvisionPower::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudNightvisionPower::ShouldDraw()
{
	bool bNeedsDraw = true;

	// S:O - This isn't optimal, but oh well, it's something.
	bNeedsDraw = ( m_flSuitPower != m_flSuitPowerOld );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	return ( bNeedsDraw && CHudElement::ShouldDraw() && pPlayer && (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) && pPlayer->IsAlive() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNightvisionPower::OnThink( void )
{
	float flCurrentPower = m_flSuitPower;

	if ( gpGlobals->curtime >= m_fNightvisionPower )
	{
		if ( shader_nightvision.GetInt() == 1 )
		{
			flCurrentPower = m_flSuitPower - 3.0f;
		}
		else
		{
			flCurrentPower = m_flSuitPower + 6.0f;
		}

		if ( flCurrentPower > 100.0f )
		{
			flCurrentPower = 100.0f;
		}
		else if ( flCurrentPower < 0.0f )
		{
			flCurrentPower = 0.0f;
		}

		if ( (shader_nightvision.GetInt() == 1) && (flCurrentPower < 10.0f) && (flCurrentPower > 0.0f) )
		{
			// S:O - Warn our player that their nightvision power is low!
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

			const Vector *vOrigin = &pLocalPlayer->GetAbsOrigin();
			CLocalPlayerFilter filter;
			pLocalPlayer->EmitSound(filter, 0, "Nightvision.PowerWarning", vOrigin);
		}
		else if ( (shader_nightvision.GetInt() == 1) && (flCurrentPower == 0.0f) )
		{
			// S:O - Turn off our nightvision since we're out of power!
			shader_nightvision.SetValue( 0 );

			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

			const Vector *vOrigin = &pLocalPlayer->GetAbsOrigin();
			CLocalPlayerFilter filter;
			pLocalPlayer->EmitSound(filter, 0, "Nightvision.ForceOff", vOrigin);
		}

		m_fNightvisionPower = gpGlobals->curtime + 3.0f;
	}

	// Only update if we've changed suit power
	if ( flCurrentPower == m_flSuitPower )
		return;

	if ( flCurrentPower >= 100.0f )
	{
		// we've reached max power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	}
	else
	{
		// we've lost power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
	}

	if ( shader_nightvision.GetInt() == 1 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerOneItemActive");
	}
	else
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNoItemsActive");
	}

	m_flSuitPowerOld = m_flSuitPower;
	m_flSuitPower = flCurrentPower;
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudNightvisionPower::Paint()
{
	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_flSuitPower * 1.0f/100.0f) + 0.5f );

	// see if we've changed power state
	int lowPower = 0;
	if (enabledChunks <= (chunkCount / 4))
	{
		lowPower = 1;
	}

	if ( lowPower && !m_bPowerWasLow )
	{
		if ( (shader_nightvision.GetInt() == 1) && (m_flSuitPower < 100.0f) )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
			m_bPowerWasLow = true;
		}
	}
	else if ( m_bPowerWasLow )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove25");
		m_bPowerWasLow = false;
	}

	// draw the suit power bar
	surface()->DrawSetColor( m_AuxPowerColor );
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( m_AuxPowerColor[0], m_AuxPowerColor[1], m_AuxPowerColor[2], m_iAuxPowerDisabledAlpha ) );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	surface()->DrawPrintText(L"LI-ON BATTERY", wcslen(L"LI-ON BATTERY"));

	if ( shader_nightvision.GetInt() == 1 )
	{
		// draw the additional text
		int ypos = text2_ypos;

		surface()->DrawSetTextPos(text2_xpos, ypos);
		surface()->DrawPrintText(L"NIGHTVISION", wcslen(L"NIGHTVISION"));
		ypos += text2_gap;
	}
}