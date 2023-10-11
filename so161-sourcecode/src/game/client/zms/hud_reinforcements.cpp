#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#ifndef CLIENT_DLL
#include "util.h"
#endif
#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Label.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hl2mp_gamerules.h"
#include "ConVar.h"

#include "ImageProgressBar.h"

extern ConVar so_respawn;

#define INIT_TIMER -1

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudReinforcementsTimer : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudReinforcementsTimer, Panel );
 
public:
   	CHudReinforcementsTimer( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateReinforcementsTime( void);

protected:
	ImageProgressBar* m_pReinforcementsProgressBar;
	vgui::Label* m_pTextTime; // text representation of timer

private:
	int rtime;
	int dur;
	float dura;
	float CalcReinforcementsDelay;
};

DECLARE_HUDELEMENT( CHudReinforcementsTimer );
CHudReinforcementsTimer::CHudReinforcementsTimer( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudReinforcementsTimer" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );  
  
    SetVisible( false );
    SetAlpha( 0 );
	SetProportional(true);

	m_pReinforcementsProgressBar = new ImageProgressBar( this, "ReinforcementsProgressBar", "HUD/ReinforcementsProgressBar_top", "HUD/ReinforcementsProgressBar_bottom");
	m_pReinforcementsProgressBar->SetProgressDirection( ProgressBar::PROGRESS_EAST);
	m_pReinforcementsProgressBar->SetSize( 256, 32 );

	m_pTextTime = new vgui::Label( this, "ReinforcementsText", "" );
	m_pTextTime->SetContentAlignment( Label::a_west );
	m_pTextTime->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextTime->SetVisible( true );
	m_pTextTime->SetAlpha( 255 );
}
void CHudReinforcementsTimer::Init()
{
	Reset();
}
void CHudReinforcementsTimer::Reset()
{
	CalcReinforcementsDelay = 0;
	dura = INIT_TIMER;

	m_pTextTime->SetPos( 40, 16 );
}
void CHudReinforcementsTimer::VidInit()
{
	Reset();
}
void CHudReinforcementsTimer::OnThink()
{
	// We don't want to clog up the tubes, so let's check every half-second instead of every frame.
	if ( gpGlobals->curtime >= CalcReinforcementsDelay )
	{
		CalculateReinforcementsTime();
		CalcReinforcementsDelay = gpGlobals->curtime + 0.5f;
	}
}
void CHudReinforcementsTimer::CalculateReinforcementsTime( void )
{
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();

	if ( local )
	{	
		CHL2MPRules *pRules = HL2MPRules();

		if (!pRules)
			return;

		// Don't show this if we don't use it, right?
		if ( so_respawn.GetInt() == 0 )
		{
			m_pReinforcementsProgressBar->SetVisible( false );
			m_pReinforcementsProgressBar->SetAlpha( 0 );
			m_pTextTime->SetVisible( false );
			m_pTextTime->SetAlpha( 0 );
		}
		else
		{
			m_pReinforcementsProgressBar->SetVisible( true );
			m_pReinforcementsProgressBar->SetAlpha( 255 );
			m_pTextTime->SetVisible( true );
			m_pTextTime->SetAlpha( 255 );
		}

		rtime = pRules->GetReinforcementsTimerRemain();
		dur = pRules->m_iReinforcementsDuration;
		dura = dur;

		m_pReinforcementsProgressBar->SetProgress( rtime/dura );
	
		// update text
		wchar_t reinforcementstimer[35];
		_snwprintf( reinforcementstimer, sizeof( reinforcementstimer ), L"%d:%02d", ( rtime / 60 ), ( rtime % 60 ) );

		m_pTextTime->SetText( reinforcementstimer );
		m_pTextTime->SizeToContents();
		m_pTextTime->SetPos( 40, 16 - ( m_pTextTime->GetTall() / 2 ) );
	}
}