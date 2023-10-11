//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Label.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "text_message.h"
#include "c_baseplayer.h"
#include "IGameUIFuncs.h"
#include "inputsystem/iinputsystem.h"
#include "c_hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar shader_nightvision;

//-----------------------------------------------------------------------------
// Purpose: Displays hints across the center of the screen
//-----------------------------------------------------------------------------
class CHudNightvisionDisplay : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudNightvisionDisplay, vgui::Panel );

public:
	CHudNightvisionDisplay( const char *pElementName );

	void Init();
	void Reset();
	void DisplayText( char *szString );

	bool SetHintNightvisionText( wchar_t *text );

	virtual void PerformLayout();

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();

protected:
	vgui::HFont m_hFont;
	Color		m_bgColor;
	vgui::Label *m_pLabel;
	CUtlVector<vgui::Label *> m_Labels;
	CPanelAnimationVarAliasType( int, m_iTextX, "text_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextY, "text_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCenterX, "center_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCenterY, "center_y", "0", "proportional_int" );

	bool m_bLastLabelUpdateHack;
	bool DisplayedText;

	float m_flNextMessage;
	float BleedingDelay2;

	bool NextMessageDisplayed1;
	bool NextMessageDisplayed2;
	bool NextMessageDisplayed3;
	bool NextMessageDisplayed4;
	bool NextMessageDisplayed5;
	bool NextMessageDisplayed6;
	bool NextMessageDisplayed7;
	bool NextMessageDisplayed8;

	CPanelAnimationVar( float, m_flLabelSizePercentage, "HintSize", "0" );
};

DECLARE_HUDELEMENT( CHudNightvisionDisplay );

#define MAX_HINT_STRINGS 5


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudNightvisionDisplay::CHudNightvisionDisplay( const char *pElementName ) : BaseClass(NULL, "HudNightvisionDisplay"), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	m_pLabel = new vgui::Label( this, "HudNightvisionDisplayLabel", "" );
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNightvisionDisplay::Init()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNightvisionDisplay::Reset()
{
	SetHintNightvisionText( NULL );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionHide" );

	m_bLastLabelUpdateHack = true;

	m_flNextMessage = 0.0;
	BleedingDelay2 = 0.0;

	DisplayedText = false;
	NextMessageDisplayed1 = false;
	NextMessageDisplayed2 = false;
	NextMessageDisplayed3 = false;
	NextMessageDisplayed4 = false;
	NextMessageDisplayed5 = false;
	NextMessageDisplayed6 = false;
	NextMessageDisplayed7 = false;
	NextMessageDisplayed8 = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNightvisionDisplay::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( GetSchemeColor("HintMessageNightvisionFg", pScheme) );
	m_hFont = pScheme->GetFont( "HudNightvisionText", true );
	m_pLabel->SetBgColor( GetSchemeColor("HintMessageNightvisionBg", pScheme) );
	m_pLabel->SetPaintBackgroundType( 2 );
	m_pLabel->SetSize( 0, GetTall() );		// Start tiny, it'll grow.
}

//-----------------------------------------------------------------------------
// Purpose: Sets the hint text, replacing variables as necessary
//-----------------------------------------------------------------------------
bool CHudNightvisionDisplay::SetHintNightvisionText( wchar_t *text )
{
	// clear the existing text
	for (int i = 0; i < m_Labels.Count(); i++)
	{
		m_Labels[i]->MarkForDeletion();
	}
	m_Labels.RemoveAll();

	wchar_t *p = text;

	while ( p )
	{
		wchar_t *line = p;
		wchar_t *end = wcschr( p, L'\n' );
		int linelengthbytes = 0;
		if ( end )
		{
			//*end = 0;	//eek
			p = end+1;
			linelengthbytes = ( end - line ) * 2;
		}
		else
		{
			p = NULL;
		}		

		// replace any key references with bound keys
		wchar_t buf[512];
		UTIL_ReplaceKeyBindings( line, linelengthbytes, buf, sizeof( buf ) );

		// put it in a label
		vgui::Label *label = vgui::SETUP_PANEL(new vgui::Label(this, NULL, buf));
		label->SetFont( m_hFont );
		label->SetPaintBackgroundEnabled( false );
		label->SetPaintBorderEnabled( false );
		label->SizeToContents();
		label->SetContentAlignment( vgui::Label::a_west );
		label->SetFgColor( GetFgColor() );
		m_Labels.AddToTail( vgui::SETUP_PANEL(label) );
	}

	InvalidateLayout( true );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Resizes the label
//-----------------------------------------------------------------------------
void CHudNightvisionDisplay::PerformLayout()
{
	BaseClass::PerformLayout();
	int i;

	int wide, tall;
	GetSize( wide, tall );

	// find the widest line
	int iDesiredLabelWide = 0;
	for ( i=0; i < m_Labels.Count(); ++i )
	{
		iDesiredLabelWide = max( iDesiredLabelWide, m_Labels[i]->GetWide() );
	}

	// find the total height
	int fontTall = vgui::surface()->GetFontTall( m_hFont );
	int labelTall = fontTall * m_Labels.Count();

	iDesiredLabelWide += m_iTextX*2;
	labelTall += m_iTextY*2;

	// Now clamp it to our animation size
	iDesiredLabelWide = (iDesiredLabelWide * m_flLabelSizePercentage);

	int x, y;
	if ( m_iCenterX < 0 )
	{
		x = 0;
	}
	else if ( m_iCenterX > 0 )
	{
		x = wide - iDesiredLabelWide;
	}
	else
	{
		x = (wide - iDesiredLabelWide) / 2;
	}

	if ( m_iCenterY > 0 )
	{
		y = 0;
	}
	else if ( m_iCenterY < 0 )
	{
		y = tall - labelTall;
	}
	else
	{
		y = (tall - labelTall) / 2;
	}

	x = max(x,0);
	y = max(y,0);

	iDesiredLabelWide = min(iDesiredLabelWide,wide);
	m_pLabel->SetBounds( x, y, iDesiredLabelWide, labelTall );

	// now lay out the sub-labels
	for ( i=0; i<m_Labels.Count(); ++i )
	{
		int xOffset = (wide - m_Labels[i]->GetWide()) * 0.5;
		m_Labels[i]->SetPos( xOffset, y + m_iTextY + i*fontTall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the label color each frame
//-----------------------------------------------------------------------------
void CHudNightvisionDisplay::OnThink()
{
	m_pLabel->SetFgColor(GetFgColor());
	for (int i = 0; i < m_Labels.Count(); i++)
	{
		m_Labels[i]->SetFgColor(GetFgColor());
	}

	// If our label size isn't at the extreme's, we're sliding open / closed
	// This is a hack to get around InvalideLayout() not getting called when
	// m_flLabelSizePercentage is changed via a HudAnimation.
	if ( m_flLabelSizePercentage != 0.0 && m_flLabelSizePercentage != 1.0 || m_bLastLabelUpdateHack )
	{
		m_bLastLabelUpdateHack = (m_flLabelSizePercentage != 0.0 && m_flLabelSizePercentage != 1.0);
		InvalidateLayout();
	}

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( (shader_nightvision.GetInt() == 1) && (pPlayer->IsAlive()) && (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) ) // yes, we're using numbers here to prevent us from using another include
	{
		if ( !DisplayedText )
		{
			DisplayText( "nightvision active. running diagnostic check..." );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );

			m_flNextMessage = gpGlobals->curtime + 5.0;

			DisplayedText = true;
		}

		if ( (DisplayedText) && (gpGlobals->curtime >= m_flNextMessage) )
		{
			if ( !NextMessageDisplayed1 )
			{
				DisplayText( "scanning digital display for artifacts..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed1 = true;
				m_flNextMessage = gpGlobals->curtime + 5.0;
			}
			else if ( !NextMessageDisplayed2 )
			{
				DisplayText( "warning! overbrightness detected...patching 0..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed2 = true;
				m_flNextMessage = gpGlobals->curtime + 3.0;
			}
			else if ( !NextMessageDisplayed3 )
			{
				DisplayText( "warning! overbrightness detected...patching 42..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed3 = true;
				m_flNextMessage = gpGlobals->curtime + 1.5;
			}
			else if ( !NextMessageDisplayed4 )
			{
				DisplayText( "warning! overbrightness detected...patching 76..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed4 = true;
				m_flNextMessage = gpGlobals->curtime + 2.0;
			}
			else if ( !NextMessageDisplayed5 )
			{
				DisplayText( "overbrightness patched. scanning for false pixels..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed5 = true;
				m_flNextMessage = gpGlobals->curtime + 5.0;
			}
			else if ( !NextMessageDisplayed6 )
			{
				DisplayText( "dropping false pixels..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed6 = true;
				m_flNextMessage = gpGlobals->curtime + 7.0;
			}
			else if ( !NextMessageDisplayed7 )
			{
				DisplayText( "pixels successfully dropped. rescanning..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed7 = true;
				m_flNextMessage = gpGlobals->curtime + 5.0;
			}
			else if ( !NextMessageDisplayed8 )
			{
				DisplayText( "diagnostic scan complete. terminating display..." );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
				NextMessageDisplayed8 = true;
				m_flNextMessage = gpGlobals->curtime + 60.0;
			}
		}

		SetVisible( true );
	}
	else
	{
		if ( DisplayedText )
		{
			DisplayText( "terminated." );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionHide" );
			DisplayedText = false;
			NextMessageDisplayed1 = false;
			NextMessageDisplayed2 = false;
			NextMessageDisplayed3 = false;
			NextMessageDisplayed4 = false;
			NextMessageDisplayed5 = false;
			NextMessageDisplayed6 = false;
			NextMessageDisplayed7 = false;
			NextMessageDisplayed8 = false;
		}

		SetVisible( false );
	}

	/*if ( gpGlobals->curtime >= BleedingDelay2 )
	{
		C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

		if ( (pPlayer) && (pPlayer->GetHealth() <= 20) && (pPlayer->GetHealth() != 0) )
		{
			DisplayText( "warning! health critical. seek medical attention immediately." );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionShow" );
			SetVisible( true );
		}
		else
		{
			DisplayText( "terminated." );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HintMessageNightvisionHide" );
			SetVisible( false );
		}

		BleedingDelay2 = gpGlobals->curtime + 5.0f;
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Activates the hint display
//-----------------------------------------------------------------------------
void CHudNightvisionDisplay::DisplayText( char *szString )
{	
	// Convert to a wchar_t*
	size_t origsize = strlen(szString) + 1;
	const size_t newsize = 255;
	size_t convertedChars = 0;
	wchar_t wcstring[newsize];
	mbstowcs_s(&convertedChars, wcstring, origsize, szString, _TRUNCATE);

	if ( !wcstring )
		DevMsg("WARNING: A HUD nightvision hint message failed to display!\n");

	// make it visible
	if ( SetHintNightvisionText( wcstring ) )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Hud.Hint" );

			if ( pLocalPlayer->Hints() )
			{
				pLocalPlayer->Hints()->PlayedAHint();
			}
		}
	}
}