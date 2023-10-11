#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "c_hl2mp_player.h"

#include "iclientmode.h"
#include "basecombatweapon_shared.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "ConVar.h"

#include "ImageProgressBar.h"

#define INIT_AMMO_SECONDARY -1

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudAmmoSecondaryBar : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudAmmoSecondaryBar, Panel );
 
public:
   	CHudAmmoSecondaryBar( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateAmmoSecondary( void );

protected:
    ImageProgressBar* m_pAmmoSecondaryProgressBar;

private:
    int m_iAmmoSecondary;
	float CalcAmmoSecondaryDelay;
};

DECLARE_HUDELEMENT( CHudAmmoSecondaryBar );
CHudAmmoSecondaryBar::CHudAmmoSecondaryBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAmmoSecondaryBar" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );  
  
    SetVisible( false );
    SetAlpha( 0 );
	SetZPos( 98 );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pAmmoSecondaryProgressBar = new ImageProgressBar( this, "AmmoSecondaryProgressBar", "HUD/Ammo2ProgressBar_top", "HUD/Ammo2ProgressBar_bottom");
	m_pAmmoSecondaryProgressBar->SetProgressDirection( ProgressBar::PROGRESS_EAST );
	m_pAmmoSecondaryProgressBar->SetSize( 256, 32 );
}
void CHudAmmoSecondaryBar::Init()
{
	Reset();
}
void CHudAmmoSecondaryBar::Reset()
{
	CalcAmmoSecondaryDelay = 0;
	m_iAmmoSecondary = INIT_AMMO_SECONDARY;
}
void CHudAmmoSecondaryBar::VidInit()
{
	Reset();
}
void CHudAmmoSecondaryBar::OnThink()
{
	// We don't want to clog up the tubes, so let's check every tenth of a second instead of every frame.
	if ( gpGlobals->curtime >= CalcAmmoSecondaryDelay )
	{
		CalculateAmmoSecondary();
		CalcAmmoSecondaryDelay = gpGlobals->curtime + 0.1f;
	}
}
void CHudAmmoSecondaryBar::CalculateAmmoSecondary( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( pPlayer && (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) && pPlayer->IsAlive() )
	{
		C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon )
		{
			int	newAmmoSecondary = pPlayer->GetAmmoCount( pWeapon->GetSecondaryAmmoType() );

			if ( newAmmoSecondary != m_iAmmoSecondary )
			{
				m_iAmmoSecondary = newAmmoSecondary;
				
				if ( m_iAmmoSecondary > 0 )
				{
					m_pAmmoSecondaryProgressBar->SetProgress( m_iAmmoSecondary/3.00f );
					m_pAmmoSecondaryProgressBar->SetVisible( true );
					m_pAmmoSecondaryProgressBar->SetAlpha( 255 );
				}
				else
				{
					m_pAmmoSecondaryProgressBar->SetProgress( 0.00f/1.00f );
					m_pAmmoSecondaryProgressBar->SetVisible( false );
					m_pAmmoSecondaryProgressBar->SetAlpha( 0 );
				}
			}
		}
	}
	else if ( pPlayer && (pPlayer->GetTeamNumber() != 3 || 4) )
	{
		m_pAmmoSecondaryProgressBar->SetVisible( false );
		m_pAmmoSecondaryProgressBar->SetAlpha( 0 );
	}
}