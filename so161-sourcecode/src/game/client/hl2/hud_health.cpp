#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "iclientmode.h"
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
#include "c_hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudHealthBar : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudHealthBar, Panel );
 
public:
   	CHudHealthBar( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateHealth( void );

protected:
    ImageProgressBar* m_pHealthProgressBar;

private:
	float CalcHealthDelay;
};

DECLARE_HUDELEMENT( CHudHealthBar );

CHudHealthBar::CHudHealthBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudHealthBar" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );  
	SetProportional(true);
    SetVisible( false );
    SetAlpha( 0 );
	SetZPos( 100 );
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pHealthProgressBar = new ImageProgressBar( this, "HealthProgressBar", "HUD/HealthProgressBar_top", "HUD/HealthProgressBar_bottom");
	m_pHealthProgressBar->SetProgressDirection( ProgressBar::PROGRESS_EAST );
	m_pHealthProgressBar->SetSize( 256, 32 );
}

void CHudHealthBar::Init()
{
	Reset();
}

void CHudHealthBar::Reset()
{
	CalcHealthDelay = 0;
}

void CHudHealthBar::VidInit()
{
	Reset();
}

void CHudHealthBar::OnThink()
{
	// We don't want to clog up the tubes, so let's check every tenth of a second instead of every frame.
	if ( gpGlobals->curtime >= CalcHealthDelay )
	{
		CalculateHealth();
		CalcHealthDelay = gpGlobals->curtime + 0.1f;
	}
}

void CHudHealthBar::CalculateHealth( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
	{
		int newHealth = pPlayer->GetHealth();
		int newMaxHealth = pPlayer->GetMaxHealth();

		if ( newHealth >= 0 && newMaxHealth >= 0 && newHealth <= newMaxHealth )
		{
			float ratio = (float)newHealth / (float)newMaxHealth;
			m_pHealthProgressBar->SetProgress( ratio );
		}
	}
}