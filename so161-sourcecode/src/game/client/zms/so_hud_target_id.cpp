//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include "vgui_EntityPanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "hl2mp_gamerules.h"
#include "hud_macros.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

using namespace vgui;

#include "hud_numericdisplay.h"
#include "ConVar.h"
#include "ImageProgressBar.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define PLAYER_HINT_DISTANCE	150
//#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)

//static ConVar hud_centerid( "hud_centerid", "1" );
//static ConVar hud_showtargetid( "hud_showtargetid", "1" );

//-----------------------------------------------------------------------------
// S:O - Target Health Bar
//-----------------------------------------------------------------------------
class CHudTargetHealthBar : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudTargetHealthBar, Panel );

public:
	CHudTargetHealthBar( const char *pElementName );
	virtual void Init( void );
	virtual void Reset( void );
	virtual void VidInit( void );
	virtual void OnThink();
	virtual void CalculateTargetHealth( void );

protected:
	ImageProgressBar* m_pHealthTargetBar;
	vgui::Label* m_pTextID;
	vgui::Label* m_pTextLevel;

private:
	float CalcTargetHealthDelay;
	int newHealth;
	int newMaxHealth;
	int x, y;;
	Vector PlayerOrigin;
	Vector VectorPos;
};

DECLARE_HUDELEMENT( CHudTargetHealthBar );

CHudTargetHealthBar::CHudTargetHealthBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTargetHealthBar" )
{
	Panel *pParent = g_pClientMode->GetViewport();

	SetParent( pParent );
	SetProportional( true );
	SetVisible( false );
	SetAlpha( 0 );

	// allow us to use this even if we're not alive or whatever
	//SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_pHealthTargetBar = new ImageProgressBar( this, "HealthTargetBar", "HUD/HealthTargetBar_top", "HUD/HealthTargetBar_bottom");
	m_pHealthTargetBar->SetProgressDirection( ProgressBar::PROGRESS_EAST );
	m_pHealthTargetBar->SetSize( 128, 16 );

	m_pTextID = new vgui::Label( this, "TargetIDText", "" );
	m_pTextID->SetContentAlignment( Label::a_center );
	m_pTextID->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextID->SetVisible( true );
	m_pTextID->SetAlpha( 255 );

	m_pTextLevel = new vgui::Label( this, "TargetLevelText", "" );
	m_pTextLevel->SetContentAlignment( Label::a_center );
	m_pTextLevel->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextLevel->SetVisible( true );
	m_pTextLevel->SetAlpha( 255 );
}

void CHudTargetHealthBar::Init()
{
	Reset();
}

void CHudTargetHealthBar::Reset()
{
	CalcTargetHealthDelay = 0;
}

void CHudTargetHealthBar::VidInit()
{
	Reset();
}

// This takes our world coordinates and turns them into HUD coordinates!
// UBER HANDY!
void WorldToScreen( const Vector &myOrig, const Vector& pos, int &x, int &y )
{
	GetVectorInScreenSpace( pos, x, y, (0,0,0) );
}

void CHudTargetHealthBar::OnThink()
{
	// We don't want to clog up the tubes, so let's check every tenth of a second instead of every frame.
	if ( gpGlobals->curtime >= CalcTargetHealthDelay )
	{
		CalculateTargetHealth();
		CalcTargetHealthDelay = gpGlobals->curtime + 0.1f;
	}
}

void CHudTargetHealthBar::CalculateTargetHealth( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( pPlayer /*&& (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) && pPlayer->IsAlive()*/ )	// allow us to use this even if we're not alive or whatever
	{
		trace_t traceHit;
		Vector swingStart = pPlayer->Weapon_ShootPosition();
		Vector forward;
		pPlayer->EyeVectors( &forward, NULL, NULL );
		Vector swingEnd = swingStart + forward * 500.0f;
		UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pPlayer, COLLISION_GROUP_NONE, &traceHit );

		CBaseEntity *pHitEntity = traceHit.m_pEnt;
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		if ( pHitEntity && pHitEntity->IsNPC() )
		{
			C_AI_BaseNPC *pTargetNPC = static_cast<C_AI_BaseNPC*>( pHitEntity );

			const char *NPCName = "NULL";

			if ( FClassnameIs( pTargetNPC, "class C_Zombie" ) )
				NPCName = "Creeper";
			else if ( FClassnameIs( pTargetNPC, "class C_FastZombie" ) )
				NPCName = "Reaper";
			else if ( FClassnameIs( pTargetNPC, "class C_ReaperNoJump" ) )
				NPCName = "Reaper";
			else if ( FClassnameIs( pTargetNPC, "class C_Cloud" ) )
				NPCName = "Cloud";
			else if ( FClassnameIs( pTargetNPC, "class C_NPC_PoisonZombie" ) )
				NPCName = "Seeker";
			else if ( FClassnameIs( pTargetNPC, "class C_SploderZombie" ) )
				NPCName = "'Sploder";
			else if ( FClassnameIs( pTargetNPC, "class C_NPC_Citizen" ) )
				NPCName = "Survivor";
			else if ( FClassnameIs( pTargetNPC, "class C_NPC_FloorTurret" ) )
				NPCName = "Turret";
			else if ( FClassnameIs( pTargetNPC, "class C_NPC_CombineS" ) )
				NPCName = "Soldier";
			else if ( FClassnameIs( pTargetNPC, "class C_Ghost" ) )
				NPCName = "Ghost";

			// If we didn't match the NPC with a valid name, don't show our text
			if ( V_strcmp( NPCName, "NULL" ) == 0 )
			{
				m_pTextID->SetVisible( false );
				m_pTextID->SetAlpha( 0 );
			}
			else
			{
				m_pTextID->SetText( NPCName );
				m_pTextID->SizeToContents();
				m_pTextID->SetPos( ((ScreenWidth()/2) - (m_pTextID->GetWide()/2)),((ScreenHeight()/2) + 75) );
				m_pTextID->SetFgColor( GameResources()->GetTeamColor( pTargetNPC->GetTeamNumber() ) );
				m_pTextID->SetVisible( true );
				m_pTextID->SetAlpha( 255 );
			}

			newHealth = pTargetNPC->GetHealth();
			newMaxHealth = pTargetNPC->GetMaxHealth();

			if ( newHealth >= 0 && newMaxHealth >= 0 && newHealth <= newMaxHealth )
			{
				float ratio = (float)newHealth / (float)newMaxHealth;
				m_pHealthTargetBar->SetProgress( ratio );
				m_pHealthTargetBar->SetPos( ((ScreenWidth()/2) - 64), ((ScreenHeight()/2) + 50) );

				m_pHealthTargetBar->SetVisible( true );
				m_pHealthTargetBar->SetAlpha( 255 );
			}

			// NPCs don't have levels, so don't display level text
			m_pTextLevel->SetVisible( false );
			m_pTextLevel->SetAlpha( 0 );
		}
		else if ( pHitEntity && pHitEntity->IsPlayer() )
		{
			C_HL2MP_Player *pTargetPlayer = static_cast<C_HL2MP_Player*>( pHitEntity );

			const char *PlayerName = "NULL";
			if ( pTargetPlayer->GetPlayerName() )
				PlayerName = pTargetPlayer->GetPlayerName();

			// If we didn't match the player with a valid name, don't show our text
			if ( V_strcmp( PlayerName, "NULL" ) == 0 )
			{
				m_pTextID->SetVisible( false );
				m_pTextID->SetAlpha( 0 );
			}
			else
			{
				m_pTextID->SetText( PlayerName );
				m_pTextID->SizeToContents();
				m_pTextID->SetPos( ((ScreenWidth()/2) - (m_pTextID->GetWide()/2)),((ScreenHeight()/2) + 75) );
				m_pTextID->SetFgColor( GameResources()->GetTeamColor( pTargetPlayer->GetTeamNumber() ) );
				m_pTextID->SetVisible( true );
				m_pTextID->SetAlpha( 255 );
			}

			int PlayerLevel = 0;
			if ( pTargetPlayer->GetLevel() )
					PlayerLevel = pTargetPlayer->GetLevel();

			char PlayerLevelText[10];
			sprintf( PlayerLevelText, "Level %i", PlayerLevel );

			// If we didn't match the player with a valid level, don't show our text
			if ( PlayerLevel == 0 )
			{
				m_pTextLevel->SetVisible( false );
				m_pTextLevel->SetAlpha( 0 );
			}
			else
			{
				m_pTextLevel->SetText( PlayerLevelText );
				m_pTextLevel->SizeToContents();
				m_pTextLevel->SetPos( ((ScreenWidth()/2) - (m_pTextLevel->GetWide()/2)),((ScreenHeight()/2) + 100) );
				m_pTextLevel->SetFgColor( GameResources()->GetTeamColor( pTargetPlayer->GetTeamNumber() ) );
				m_pTextLevel->SetVisible( true );
				m_pTextLevel->SetAlpha( 255 );
			}

			newHealth = pTargetPlayer->GetHealth();
			newMaxHealth = pTargetPlayer->GetMaxHealth();

			if ( newHealth >= 0 && newMaxHealth >= 0 && newHealth <= newMaxHealth )
			{
				float ratio = (float)newHealth / (float)newMaxHealth;
				m_pHealthTargetBar->SetProgress( ratio );
				m_pHealthTargetBar->SetPos( ((ScreenWidth()/2) - 64), ((ScreenHeight()/2) + 50) );

				m_pHealthTargetBar->SetVisible( true );
				m_pHealthTargetBar->SetAlpha( 255 );
			}
		}
		else
		{
			m_pHealthTargetBar->SetProgress( 0.0f );
			m_pHealthTargetBar->SetVisible( false );
			m_pHealthTargetBar->SetAlpha( 0 );

			m_pTextID->SetVisible( false );
			m_pTextID->SetAlpha( 0 );

			m_pTextLevel->SetVisible( false );
			m_pTextLevel->SetAlpha( 0 );
		}
	}
	else
	{
		m_pHealthTargetBar->SetProgress( 0.0f );
		m_pHealthTargetBar->SetVisible( false );
		m_pHealthTargetBar->SetAlpha( 0 );

		m_pTextID->SetVisible( false );
		m_pTextID->SetAlpha( 0 );

		m_pTextLevel->SetVisible( false );
		m_pTextLevel->SetAlpha( 0 );
	}
}

/*//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetID : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTargetID, vgui::Panel );

public:
	CTargetID( const char *pElementName );
	void Init( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void VidInit( void );

private:
	Color			GetColorForTargetTeam( int iTeamNumber );

	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	float			m_flLastChangeTime;
};

DECLARE_HUDELEMENT( CTargetID );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
CHudElement( pElementName ), BaseClass( NULL, "TargetID" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Init( void )
{
};

void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_hFont = scheme->GetFont( "TargetID", IsProportional() );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

Color CTargetID::GetColorForTargetTeam( int iTeamNumber )
{
	return GameResources()->GetTeamColor( iTeamNumber );
} 

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CTargetID::Paint()
{
#define MAX_ID_STRING 256
	wchar_t sIDString[ MAX_ID_STRING ];
	sIDString[0] = 0;

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( !pPlayer )
		return;

	Color c;

	// Get our target's ent index
	int iEntIndex = pPlayer->GetIDTarget();
	// Didn't find one?
	if ( !iEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && (gpGlobals->curtime > (m_flLastChangeTime + 0.5)) )
		{
			m_flLastChangeTime = 0;
			sIDString[0] = 0;
			m_iLastEntIndex = 0;
		}
		else
		{
			// Keep re-using the old one
			iEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
	}

	// Is this an entindex sent by the server?
	if ( iEntIndex )
	{
		C_HL2MP_Player *pPlayer = ToHL2MPPlayer(cl_entitylist->GetEnt( iEntIndex ));
		C_HL2MP_Player *pLocalPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

		const char *printFormatString = NULL;
		wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
		bool bShowPlayerName = false;

		// Some entities we always want to check, cause the text may change
		// even while we're looking at it
		// Is it a player?
		if ( IsPlayerIndex( iEntIndex ) )
		{
			c = GetColorForTargetTeam( pPlayer->GetTeamNumber() );

			bShowPlayerName = true;
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(),  wszPlayerName, sizeof(wszPlayerName) );

			if ( HL2MPRules()->IsTeamplay() == true && pPlayer->InSameTeam(pLocalPlayer) )
			{
				printFormatString = "%s1";
			}
			else
			{
				printFormatString = "%s1";
			}
		}

		if ( printFormatString )
		{
			if ( bShowPlayerName )
			{
				g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString),
					g_pVGuiLocalize->Find(printFormatString), 1, wszPlayerName );
			}
		}

		if ( sIDString[0] )
		{
			int wide, tall;
			int ypos = YRES(260);
			int xpos = XRES(10);

			vgui::surface()->GetTextSize( m_hFont, sIDString, wide, tall );

			if( hud_centerid.GetInt() == 0 )
			{
				ypos = YRES(420);
			}
			else
			{
				xpos = (ScreenWidth() - wide) / 2;
			}

			vgui::surface()->DrawSetTextFont( m_hFont );
			vgui::surface()->DrawSetTextPos( xpos, ypos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );
		}
	}
}*/