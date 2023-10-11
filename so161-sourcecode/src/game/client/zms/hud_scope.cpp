//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Im just gonna use this, no point reinventing the wheel. 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "vgui/ISurface.h"
#include "input.h"

#include "weapon_sniperbase.h"

#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

#define SCOPE_INITIAL_SIZE 256

// memdbgon must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"

class CHudScope : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudScope, vgui::Panel );

public:
	CHudScope( const char *pElementName );

	void Init();
	void MsgFunc_ShowScope( bf_read &msg );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint( void );
	virtual void PaintBackground();
	virtual void Think( void );
	bool	ShouldDraw();
	bool	m_bShow;
	int		SCOPE_SIZE;

private:
	bool m_bStopCheck;
	CHudTexture*	m_pScopeArc;
};


DECLARE_HUDELEMENT( CHudScope );
DECLARE_HUD_MESSAGE( CHudScope, ShowScope );

using namespace vgui;

CHudScope::CHudScope( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudScope")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_bShow = false;
	m_bStopCheck = false;

	// Scope will not show when the player is dead
	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

void CHudScope::Init()
{
	HOOK_HUD_MESSAGE( CHudScope, ShowScope );
}

void CHudScope::Think( void )
{
	if ( ShouldDraw() )
	{
		if ( ::input->CAM_IsThirdPerson() )
		{
			::input->CAM_ToFirstPerson();

			if ( !m_bStopCheck )
				m_bStopCheck = true;
		}
	}
	else if ( !ShouldDraw() )
	{
		if ( m_bStopCheck )
		{
			::input->CAM_ToThirdPerson();
			m_bStopCheck = false;
		}
	}
}

void CHudScope::PaintBackground()
{  
    SetBgColor(Color(0,0,0,0));
   
	int s_wide, s_tall;
	surface()->GetScreenSize(s_wide, s_tall);
	SCOPE_SIZE = scheme()->GetProportionalScaledValue( SCOPE_INITIAL_SIZE );
	
    SetPaintBorderEnabled(false);
	BaseClass::PaintBackground();
}

extern ConVar vm_ironsight_adjust;
bool CHudScope::ShouldDraw()
{
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if( pLocalPlayer )
	{
		CWeaponSOSniperBase *pSniperWeapon = dynamic_cast< CWeaponSOSniperBase* >( pLocalPlayer->GetActiveWeapon() );

		if( pSniperWeapon && pSniperWeapon->ShouldDrawScope() && !vm_ironsight_adjust.GetBool() )
			return true;
	}

	// default behaviour for legacy support
	return m_bShow;
}

void CHudScope::Paint( void )
{
	if ( ShouldDraw() )
	{
		int s_wide, s_tall;
		surface()->GetScreenSize(s_wide, s_tall);
		SCOPE_SIZE = scheme()->GetProportionalScaledValue( SCOPE_INITIAL_SIZE );

		int scope_width = s_tall; // force 4:3
		int scope_x = ( s_wide / 2 ) - ( scope_width / 2 ); // very left side of the scope

		m_pScopeArc->DrawSelf( scope_x, 0, scope_width, s_tall, Color( 255, 255, 255, 255 ) );

		// lines on scope:
		surface()->DrawSetColor( Color( 0, 0, 0, 150 ) );
		surface()->DrawLine( s_wide / 2, 0, s_wide / 2, s_tall ); // vertical
		surface()->DrawLine( scope_x, s_tall / 2, scope_x + scope_width, s_tall / 2 ); // horizontal

		// side fillers
		if( scope_x > 0 )
		{
			surface()->DrawSetColor( 0, 0, 0, 255 );
			surface()->DrawFilledRect( 0, 0, scope_x, s_tall ); // left
			surface()->DrawFilledRect( scope_x + scope_width, 0, s_wide, s_tall );
		}

		/*m_pScopeArc->DrawSelf((s_wide/2)-(SCOPE_SIZE/2),(s_tall/2)-(SCOPE_SIZE/2),SCOPE_SIZE,SCOPE_SIZE,Color(255,255,255,255)); 

		vgui::surface()->DrawSetColor( Color( 0, 0, 0, 150 ));
		//vertical line
		surface()->DrawLine( s_wide/2, (s_tall/2)-(SCOPE_SIZE/2), s_wide/2, (s_tall/2)+(SCOPE_SIZE/2));
		//horizontal line
		surface()->DrawLine( (s_wide/2)-(SCOPE_SIZE/2), s_tall/2, (s_wide/2)+(SCOPE_SIZE/2), s_tall/2);

		vgui::surface()->DrawSetColor( Color( 0, 0, 0, 255 ));
		//top filler
		vgui::surface()->DrawFilledRect(0,0,s_wide,(s_tall/2)-(SCOPE_SIZE/2));
		//left filler
		vgui::surface()->DrawFilledRect(0,(s_tall/2)-(SCOPE_SIZE/2),(s_wide/2)-(SCOPE_SIZE/2), (s_tall/2)+(SCOPE_SIZE/2));
		//right filler
		vgui::surface()->DrawFilledRect((s_wide/2)+(SCOPE_SIZE/2),(s_tall/2)-(SCOPE_SIZE/2),s_wide,(s_tall/2)+(SCOPE_SIZE/2));
		//bottom filler
		vgui::surface()->DrawFilledRect( 0,(s_tall/2)+(SCOPE_SIZE/2),s_wide,s_tall );*/
	}
}

void CHudScope::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);

	if (!m_pScopeArc) m_pScopeArc = gHUD.GetIcon("scope");
}

void CHudScope::MsgFunc_ShowScope(bf_read &msg)
{
	m_bShow = msg.ReadByte();
}