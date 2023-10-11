//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//====================================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>


#include "hl2mp/c_hl2mp_player.h"
#include "hl2mp/hl2mp_gamerules.h"
#include "clientmode_sonormal.h"

#include "hud_basedeathnotice.h"

#include "engine/ienginesound.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CSOHudDeathNotice : public CHudBaseDeathNotice
{
	DECLARE_CLASS_SIMPLE( CSOHudDeathNotice, CHudBaseDeathNotice );
public:
	CSOHudDeathNotice( const char *pElementName ) : CHudBaseDeathNotice( pElementName ) {};
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool IsVisible( void );
	virtual void Init( void );

protected:	
	virtual void OnGameEvent( IGameEvent *event, DeathNoticeItem &msg );
	virtual Color GetTeamColor( int iTeamNumber );

private:
	void AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey );

	//CHudTexture		*m_iconDomination;

	//CPanelAnimationVar( Color, m_clrZombieText, "TeamZombie", "204 0 0 255 255" );
	//CPanelAnimationVar( Color, m_clrSurvivorText, "TeamSurvivor", "208 208 208 255" );
	//CPanelAnimationVar( Color, m_clrMilitaryText, "TeamMilitary", "TODO: FIND EXACT MILITARY TEAM COLOR" );

};

DECLARE_HUDELEMENT( CSOHudDeathNotice );

void CSOHudDeathNotice::Init()
{
	//ListenForGameEvent( "zone_captured" );
	//ListenForGameEvent( "zone_pointunderattack" );
	BaseClass::Init();
}

void CSOHudDeathNotice::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	//m_iconDomination = gHUD.GetIcon( "leaderboard_dominated" );
}

bool CSOHudDeathNotice::IsVisible( void )
{
	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a game event happens and a death notice is about to be 
//			displayed.  This method can examine the event and death notice and
//			make game-specific tweaks to it before it is displayed
//-----------------------------------------------------------------------------
void CSOHudDeathNotice::OnGameEvent( IGameEvent *event, DeathNoticeItem &msg )
{
	const char *pszEventName = event->GetName();

	/*if( FStrEq( "zone_captured", event->GetName() ) )
	{
		HandlePointCapture( event, msg );
	}
	else if( FStrEq( "zone_underattack", event->GetName() ) ) 
	{
		HandlePointAttack( event, msg );
	}
	else*/ if ( FStrEq( pszEventName, "player_death" ))
	{
		//int iCustomDamage = event->GetInt( "customkill", 0 );
		//int iLocalPlayerIndex = GetLocalPlayerIndex();

		// if this death involved a player dominating another player or getting revenge on another player, add an additional message
		// mentioning that
		//int iKillerID = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		//int iVictimID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

		// this sees if the killer was a non-player ent and finds a suitable string to show next to it
		if( msg.wzInfoText[0] == '\0' && !FStrEq( event->GetString( "inflictor_class", "player" ), "player" ) ) // suicide
		{
			char localizedname[150];
			Q_snprintf( localizedname, sizeof( localizedname ), "#DeathMsg_Ent_%s", event->GetString( "inflictor_class", "worldspawn" ) );

			const wchar_t *pkMsg = g_pVGuiLocalize->Find( localizedname );

			if ( pkMsg )
			{
				char killername[150];
				g_pVGuiLocalize->ConvertUnicodeToANSI( pkMsg, killername, sizeof( killername ) );

				if( killername )
				{
					Q_strcpy( msg.Killer.szName, killername );

					/*if( event->GetBool( "zombiekill" ) == true )
						msg.Killer.iTeam = TEAM_ZOMBIES;
					else if( event->GetBool( "npckill" ) == true )
						msg.Killer.iTeam = TEAM_SURVIVORS;
					else
						msg.Killer.iTeam = TEAM_UNASSIGNED;*/

					msg.Killer.iTeam = event->GetInt( "killerteam" );
				}
				else
				{
					Q_strcpy( msg.Killer.szName, event->GetString( "inflictor_class", "worldspawn" ) );
					Msg( "No translation found for #DeathMsg_Ent_%s\n", event->GetString( "inflictor_class", "worldspawn" ) );
				}
			}
		}


		// if there was an assister, put both the killer's and assister's names in the death message
		/*int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
		if ( assister_name )
		{
			char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];

			if( iKillerID == iVictimID || msg.Killer.iTeam == TEAM_UNASSIGNED ) // TEAM_UNASSIGNED is used for ent kills
			{
				Q_snprintf( szKillerBuf, ARRAYSIZE( szKillerBuf ), "%s", assister_name );
				msg.Killer.iTeam = g_PR->GetTeam( iAssisterID );
				msg.bSelfInflicted = false;
			}
			else
				Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", msg.Killer.szName, assister_name );

			Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );

			if ( iLocalPlayerIndex == iAssisterID )
			{
				msg.bLocalPlayerInvolved = true;
			}
		}

		if ( event->GetInt( "dominated" ) > 0 )
		{
			AddAdditionalMsg( iKillerID, iVictimID, "#Msg_Dominating" );
		}
		if ( event->GetInt( "assister_dominated" ) > 0 && ( iAssisterID > 0 ) )
		{
			AddAdditionalMsg( iAssisterID, iVictimID, "#Msg_Dominating" );
		}
		if ( event->GetInt( "revenge" ) > 0 ) 
		{
			AddAdditionalMsg( iKillerID, iVictimID, "#Msg_Revenge" );
		}
		if ( event->GetInt( "assister_revenge" ) > 0 && ( iAssisterID > 0 ) ) 
		{
			AddAdditionalMsg( iAssisterID, iVictimID, "#Msg_Revenge" );
		}*/
	} 
}

//-----------------------------------------------------------------------------
// Purpose: Adds an additional death message
//-----------------------------------------------------------------------------
void CSOHudDeathNotice::AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey )
{
	DeathNoticeItem &msg2 = m_DeathNotices[AddDeathNoticeItem()];
	Q_strncpy( msg2.Killer.szName, g_PR->GetPlayerName( iKillerID ), ARRAYSIZE( msg2.Killer.szName ) );
	Q_strncpy( msg2.Victim.szName, g_PR->GetPlayerName( iVictimID ), ARRAYSIZE( msg2.Victim.szName ) );
	const wchar_t *wzMsg =  g_pVGuiLocalize->Find( pMsgKey );
	if ( wzMsg )
	{
		V_wcsncpy( msg2.wzInfoText, wzMsg, sizeof( msg2.wzInfoText ) );
	}
	//msg2.iconDeath = m_iconDomination;
	int iLocalPlayerIndex = GetLocalPlayerIndex();
	if ( iLocalPlayerIndex == iVictimID || iLocalPlayerIndex == iKillerID )
	{
		msg2.bLocalPlayerInvolved = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the color to draw text in for this team.  
//-----------------------------------------------------------------------------
Color CSOHudDeathNotice::GetTeamColor( int iTeamNumber )
{
	/*switch( iTeamNumber )
	{
		case TEAM_SURVIVORS:
			return m_clrSurvivorText;
			break;

		case TEAM_ZOMBIES:
			return m_clrZombieText;
			break;
		
		//case TEAM_MILITARY:
			//reutrn m_clrMilitaryText;
			//break;
	}*/

	return BaseClass::GetTeamColor( iTeamNumber );
}
