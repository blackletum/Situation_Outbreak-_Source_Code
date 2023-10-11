#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "c_hl2mp_player.h"
#include "ammodef.h"

#include "iclientmode.h"
#include "iclientvehicle.h"
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

#define INIT_AMMO -1

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudAmmoBar : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudAmmoBar, Panel );
 
public:
   	CHudAmmoBar( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateAmmo( void );

protected:
    ImageProgressBar* m_pAmmoProgressBar;

private:
    int m_iAmmo;
	float CalcAmmoDelay;
};

DECLARE_HUDELEMENT( CHudAmmoBar );
CHudAmmoBar::CHudAmmoBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAmmoBar" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );  
	SetProportional(true);
    SetVisible( false );
    SetAlpha( 0 );
	SetZPos( 99 );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pAmmoProgressBar = new ImageProgressBar( this, "AmmoProgressBar", "HUD/Ammo1ProgressBar_top", "HUD/Ammo1ProgressBar_bottom");
	m_pAmmoProgressBar->SetProgressDirection( ProgressBar::PROGRESS_EAST );
	m_pAmmoProgressBar->SetSize( 256, 32 );
}
void CHudAmmoBar::Init()
{
	Reset();
}
void CHudAmmoBar::Reset()
{
	CalcAmmoDelay = 0;
	m_iAmmo = INIT_AMMO;
}
void CHudAmmoBar::VidInit()
{
	Reset();
}
void CHudAmmoBar::OnThink()
{
	// We don't want to clog up the tubes, so let's check every tenth of a second instead of every frame.
	if ( gpGlobals->curtime >= CalcAmmoDelay )
	{
		CalculateAmmo();
	}
}
void CHudAmmoBar::CalculateAmmo( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( !pPlayer )
		return;

	if ( (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) && pPlayer->IsAlive() )
	{
		C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon )
		{
			int	newAmmo = pWeapon->Clip1();

			if ( newAmmo != m_iAmmo )
			{
				m_iAmmo = newAmmo;

				if ( m_iAmmo > 0 )
				{
					m_pAmmoProgressBar->SetProgress( m_iAmmo/((float) pWeapon->GetMaxClip1()) );
					m_pAmmoProgressBar->SetVisible( true );
					m_pAmmoProgressBar->SetAlpha( 255 );
				}
				else
				{
					m_pAmmoProgressBar->SetProgress( 0.00f/1.00f );
					m_pAmmoProgressBar->SetVisible( false );
					m_pAmmoProgressBar->SetAlpha( 0 );
				}
			}
		}
	}
	else
	{
		m_pAmmoProgressBar->SetVisible( false );
		m_pAmmoProgressBar->SetAlpha( 0 );
	}

	CalcAmmoDelay = gpGlobals->curtime + 0.1f;
}

/////////////////////////
// S:O - Clip Display //
/////////////////////////

class CHudClipBar : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudClipBar, Panel );
 
public:
	CHudClipBar( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateClip( void );

protected:
    ImageProgressBar* m_pClipProgressBar;

private:
    int m_iAmmo2;
	float CalcClipDelay;
	int shittyhack;
};

DECLARE_HUDELEMENT( CHudClipBar );
CHudClipBar::CHudClipBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudClipBar" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );
	SetProportional(true);
    SetVisible( false );
    SetAlpha( 0 );
	SetZPos( 50 );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pClipProgressBar = new ImageProgressBar( this, "ClipProgressBar", "HUD/ClipProgressBar_top", "HUD/ClipProgressBar_bottom");
	m_pClipProgressBar->SetProgressDirection( ProgressBar::PROGRESS_NORTH );
	m_pClipProgressBar->SetSize( 32, 150 );
}
void CHudClipBar::Init()
{
	Reset();
}
void CHudClipBar::Reset()
{
	CalcClipDelay = 0;
	m_iAmmo2 = -1;
}
void CHudClipBar::VidInit()
{
	Reset();
}
void CHudClipBar::OnThink()
{
	// We don't want to clog up the tubes, so let's check every tenth of a second instead of every frame.
	if ( gpGlobals->curtime >= CalcClipDelay )
	{
		CalculateClip();
		CalcClipDelay = gpGlobals->curtime + 0.1f;
	}
}
void CHudClipBar::CalculateClip( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( !pPlayer )
		return;

	if ( (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) && pPlayer->IsAlive() )
	{
		C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon )
		{
			int	newClip = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );

			if ( newClip != m_iAmmo2 )
			{
				m_iAmmo2 = newClip;

				if ( m_iAmmo2 > 0 )
				{
					// JAMES @
					// This is probably the biggest HACK ever.
					// Well, maybe not the biggest.
					// We have to remember to update this whenever we add another weapon.
					/*const char *pWeaponName = pWeapon->GetName();
					if ( FStrEq( pWeaponName, "weapon_deagle" ) )
						shittyhack = 10;
					else if ( FStrEq( pWeaponName, "weapon_ak47" ) )
						shittyhack = 10;
					else if ( FStrEq( pWeaponName, "weapon_ar2" ) )
						shittyhack = 10;
					else if ( FStrEq( pWeaponName, "weapon_sniper" ) )
						shittyhack = 10;
					else if ( FStrEq( pWeaponName, "weapon_9mm" ) )
						shittyhack = 99;
					else if ( FStrEq( pWeaponName, "weapon_pumpshotty" ) )
						shittyhack = 50;
					else if ( FStrEq( pWeaponName, "weapon_mp5k" ) )
						shittyhack = 10;
					else if ( FStrEq( pWeaponName, "weapon_mac10" ) )
						shittyhack = 10;
					else
						shittyhack = 3;*/

					/*
						STEVE:
							This is how you really do it.
					*/
					shittyhack = GetAmmoDef()->MaxCarry( pWeapon->GetPrimaryAmmoType( ) );

					m_pClipProgressBar->SetProgress( ((float)m_iAmmo2)/((float) shittyhack) );
					m_pClipProgressBar->SetVisible( true );
					m_pClipProgressBar->SetAlpha( 255 );
				}
				else
				{
					m_pClipProgressBar->SetProgress( 0.00f/1.00f );
					m_pClipProgressBar->SetVisible( false );
					m_pClipProgressBar->SetAlpha( 0 );
				}
			}
		}
	}
	else
	{
		m_pClipProgressBar->SetVisible( false );
		m_pClipProgressBar->SetAlpha( 0 );
	}
}