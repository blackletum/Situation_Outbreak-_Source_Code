#include "cbase.h"
#include "iclientmode.h"
#include "hudelement.h"
#include "ImageProgressBar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudSpecialAbilities : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudSpecialAbilities, Panel );
 
public:
   	CHudSpecialAbilities( const char *pElementName );
    virtual void Init( void );
    virtual void Reset( void );
    virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateSpecialAbilities( void);

protected:
	ImageProgressBar* m_pSpecialAbilitiesProgressBar;
};

DECLARE_HUDELEMENT( CHudSpecialAbilities );
CHudSpecialAbilities::CHudSpecialAbilities( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSpecialAbilities" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );  
  
    SetVisible( false );
    SetAlpha( 0 );
	SetProportional( true );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pSpecialAbilitiesProgressBar = new ImageProgressBar( this, "SpecialAbilitiesProgressBar", "HUD/SAProgressBar_top", "HUD/SAProgressBar_bottom" );
	m_pSpecialAbilitiesProgressBar->SetProgressDirection( ProgressBar::PROGRESS_EAST );
	m_pSpecialAbilitiesProgressBar->SetSize( 256, 32 );
}

void CHudSpecialAbilities::Init()
{
	Reset();
}

void CHudSpecialAbilities::Reset()
{
	// Nothing needed here
}

void CHudSpecialAbilities::VidInit()
{
	Reset();
}

void CHudSpecialAbilities::OnThink()
{
	CalculateSpecialAbilities();
}

void CHudSpecialAbilities::CalculateSpecialAbilities( void )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// We should only see this if we're a Stalker since they're the only ones who really need this right now
	if ( pLocalPlayer && pLocalPlayer->IsStalker() )
	{
		m_pSpecialAbilitiesProgressBar->SetProgress( (float)pLocalPlayer->m_Local.m_flSuperJumpTime / (float)5000.0f );
		m_pSpecialAbilitiesProgressBar->SetVisible( true );
		m_pSpecialAbilitiesProgressBar->SetAlpha( 255 );
	}
	else
	{
		m_pSpecialAbilitiesProgressBar->SetVisible( false );
		m_pSpecialAbilitiesProgressBar->SetAlpha( 0 );
	}
}