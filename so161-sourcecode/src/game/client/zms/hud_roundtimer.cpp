#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
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

#define INIT_TIMER -1

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudRoundTimer : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudRoundTimer, Panel );
 
public:
   	CHudRoundTimer( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateRoundtime( void);

protected:
	ImageProgressBar* m_pRoundProgressBar;
	vgui::Label* m_pTextTime; // text representation of timer

private:
	int rtime;
	int dur;
	float dura;
	float CalcRoundtimeDelay;
};

DECLARE_HUDELEMENT( CHudRoundTimer );
CHudRoundTimer::CHudRoundTimer( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudRoundTimer" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );  
  
    SetVisible( false );
    SetAlpha( 0 );
	SetProportional(true);

	m_pRoundProgressBar = new ImageProgressBar( this, "RoundTimerProgressBar", "HUD/TimerProgressBar_top", "HUD/TimerProgressBar_bottom");
	m_pRoundProgressBar->SetProgressDirection( ProgressBar::PROGRESS_EAST);
	m_pRoundProgressBar->SetSize( 256, 32 );

	m_pTextTime = new vgui::Label( this, "RoundTimerText", "" );
	m_pTextTime->SetContentAlignment( Label::a_west );
	m_pTextTime->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextTime->SetVisible( true );
	m_pTextTime->SetAlpha( 255 );

}
void CHudRoundTimer::Init()
{
	Reset();
}
void CHudRoundTimer::Reset()
{
	CalcRoundtimeDelay = 0;
	dura = INIT_TIMER;

	m_pTextTime->SetPos( 40, 16 );
}
void CHudRoundTimer::VidInit()
{
	Reset();
}
void CHudRoundTimer::OnThink()
{
	// We don't want to clog up the tubes, so let's check every half-second instead of every frame.
	if ( gpGlobals->curtime >= CalcRoundtimeDelay )
	{
		CalculateRoundtime();
		CalcRoundtimeDelay = gpGlobals->curtime + 0.5f;
	}
}
void CHudRoundTimer::CalculateRoundtime( void )
{
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();

	if ( local )
	{	
		CHL2MPRules *pRules = HL2MPRules();

		if (!pRules)
			return;

		rtime = pRules->GetRoundtimerRemain();
		dur = pRules->m_iDuration;
		dura = dur;

		m_pRoundProgressBar->SetProgress( rtime/dura );
	
		// update text
		wchar_t roundtimer[35];
		_snwprintf( roundtimer, sizeof( roundtimer ), L"%d:%02d", ( rtime / 60 ), ( rtime % 60 ) );

		m_pTextTime->SetText( roundtimer );
		m_pTextTime->SizeToContents();
		m_pTextTime->SetPos( 40, 16 - ( m_pTextTime->GetTall() / 2 ) );
	}
}