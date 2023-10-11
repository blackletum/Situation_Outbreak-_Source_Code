//////////////////////////////////////////
// S:O - Level & Experience HUD Element//
//////////////////////////////////////////

// It doesn't get any simpler than this, folks!	// I lied, it does

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudLevel : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudLevel, CHudNumericDisplay );

public:
	CHudLevel( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink( void );
	void Paint( void );
};

DECLARE_HUDELEMENT( CHudLevel );

CHudLevel::CHudLevel( const char *pElementName ) : BaseClass(NULL, "HudLevel"), CHudElement( pElementName )
{
	SetProportional( true );
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudLevel::Init( void )
{
	Reset();
}

void CHudLevel::Reset( void )
{
	SetLabelText(L"LEVEL");
	this->SetVisible(false);
}

void CHudLevel::VidInit( void )
{
	Reset();
}

void CHudLevel::OnThink( void )
{
	// This gives us a nice black background - nothing more, nothing less.
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MoneyHONEY");

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
		SetDisplayValue( pPlayer->GetLevel() );
}

void CHudLevel::Paint()
{
	BaseClass::Paint();

	C_HL2MP_Player *pLocalPlayer = dynamic_cast<C_HL2MP_Player*>( C_BasePlayer::GetLocalPlayer() );

	if ( !pLocalPlayer )
		return;

	int maxXPTillLevel = pLocalPlayer->GetXPNeededForNextLevel();
	if ( maxXPTillLevel != -1 )
	{
		int x, y, wide, tall;
		GetBounds( x, y, wide, tall );

		Color clr = Color( 255, 255, 255, GetAlpha() );

		int flExp = pLocalPlayer->GetXP() - pLocalPlayer->GetXPNeededForCurrentLevel();
		float flPercentage = clamp( (float)flExp / (float)maxXPTillLevel, 0, 1 );

		surface()->DrawSetColor( clr );
		surface()->DrawOutlinedRect( wide-10, 0, wide, tall );
		surface()->DrawSetColor( clr );
		surface()->DrawFilledRect( wide-8, 2, wide-2, (int)(flPercentage*tall)-2 );
	}
}