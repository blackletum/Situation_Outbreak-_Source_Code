//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "hl2mp_player.h"
#include "hl2mp/hl2mp_gamerules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "team.h"
#include "viewport_panel_names.h"

// S:O - Additional Includes
#include "lua/zms_luagameplay.h"
#include "lua/zms_lua.h"
#include "tier0/vprof.h"
#include "lua/ge_luamanager.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say( edict_t *pEdict, bool teamonly );

extern CBaseEntity*	FindPickerEntityClass( CBasePlayer *pPlayer, char *classname );
extern bool	g_fGameOver;

// S:O - Additional ConVars
static ConVar so_grace_period( "so_grace_period", "60", FCVAR_ARCHIVE, "Defines the amount of time (in seconds) after a new map is loaded that players are allowed to join a round already in progress");

// S:O - Initialize External ConVars
extern ConVar so_spawnmoney;
extern ConVar so_gamemode;
extern ConVar so_respawn;

void FinishClientPutInServer( CHL2MP_Player *pPlayer )
{
	// S:O - Spawns the player appropriately.
	pPlayer->InitialSpawn();
	//pPlayer->m_iConnectSpawn = true;

	ClientPrint( pPlayer, HUD_PRINTTALK, "Welcome to Situation Outbreak!\n" );

	if ( so_grace_period.GetFloat() >= gpGlobals->curtime )
	{
		if ( HL2MPRules()->PlayersAreConnected )
			ClientPrint( pPlayer, HUD_PRINTTALK, "You have been spawned into a round already in progress.\n" );
		else
			ClientPrint( pPlayer, HUD_PRINTTALK, "You have been spawned.\n" );

		pPlayer->m_iLateJoiner = 1;
	}
	else
	{
		if ( so_respawn.GetBool() )
		{
			char szSpawnTimer[128];
			Q_snprintf( szSpawnTimer, sizeof( szSpawnTimer ), "%i", HL2MPRules()->GetReinforcementsTimerRemain2() );
			ClientPrint( pPlayer, HUD_PRINTTALK, "You will spawn in approximately %s1 seconds.\n", szSpawnTimer );
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "You will spawn next round.\n" );
		}

		pPlayer->m_iLateJoiner = 2;
	}

	pPlayer->Spawn();
	//pPlayer->m_iMoney = so_spawnmoney.GetInt();

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );

	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
			*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );

	/*if ( HL2MPRules()->IsTeamplay() == true )
	{
	ClientPrint( pPlayer, HUD_PRINTTALK, "You are on team %s1\n", pPlayer->GetTeam()->GetName() );
	}*/

	// S:O - Custom join messages and the like.
	/*hudtextparms_s tTextParam;
	tTextParam.x			= -1;
	tTextParam.y			= -1;
	tTextParam.effect		= 1;
	tTextParam.r1			= 255;
	tTextParam.g1			= 255;
	tTextParam.b1			= 255;
	tTextParam.a1			= 255;
	tTextParam.r2			= 255;
	tTextParam.g2			= 255;
	tTextParam.b2			= 255;
	tTextParam.a2			= 255;
	tTextParam.fadeinTime	= 1.0;
	tTextParam.fadeoutTime	= 1.0;
	tTextParam.holdTime		= 5.0;
	tTextParam.fxTime		= 1.0;
	tTextParam.channel		= 1;*/

	const ConVar *hostname = cvar->FindVar( "hostname" );
	const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

	KeyValues *data = new KeyValues("data");
	data->SetString( "title", title );		// info panel title
	data->SetString( "type", "1" );			// show userdata from stringtable entry
	data->SetString( "msg",	"motd" );		// use this stringtable entry

	pPlayer->ShowViewPortPanel( PANEL_INFO, true, data );

	data->deleteThis();

	// S:O - LUA
	ZMSLuaGamePlay *p = GetGP();
	CBasePlayer *pPlayer2 = dynamic_cast<CBasePlayer*>(pPlayer);
	if(p)
	{
		p->PlayerConnect(pPlayer2);
	}
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CHL2MP_Player *pPlayer = CHL2MP_Player::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CHL2MP_Player *pPlayer = ToHL2MPPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	/*if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
	return g_pGameRules->GetGameDescription();
	else
	return "Half-Life 2 Deathmatch";*/

	// S:O - Custom Game Definitions Based on Gamemode
	if( FStrEq(so_gamemode.GetString(), "Infection") )
	{
		return "Infection - SO";
	}
	else if( FStrEq(so_gamemode.GetString(), "Survival") )
	{
		return "Survival - SO";
	}
	else if( FStrEq(so_gamemode.GetString(), "Holdout") )
	{
		return "Holdout - SO";
	}
	else if( FStrEq(so_gamemode.GetString(), "Escape") )
	{
		return "Escape - SO";
	}
	else if ( FStrEq(so_gamemode.GetString(), "VIP") )
	{
		return "VIP - SO";
	}
	else if ( FStrEq(so_gamemode.GetString(), "Hybrid") )
	{
		return "Hybrid - SO";
	}
	else if ( FStrEq( so_gamemode.GetString(), "Objective" ) )
	{
		return "Objective - SO";
	}
	else if ( FStrEq( so_gamemode.GetString(), "Overlord" ) )
	{
		return "Overlord - SO";
	}
	else if ( FStrEq( so_gamemode.GetString(), "Extermination" ) )
	{
		return "Extermination - SO";
	}

	return "Situation Outbreak";
}

//-----------------------------------------------------------------------------
// Purpose: Given a player and optional name returns the entity of that 
//			classname that the player is nearest facing
//			
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity* FindEntity( edict_t *pEdict, char *classname)
{
	// If no name was given set bits based on the picked
	if (FStrEq(classname,"")) 
	{
		return (FindPickerEntityClass( static_cast<CBasePlayer*>(GetContainingEntity(pEdict)), classname ));
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	CBaseEntity::PrecacheModel("models/player.mdl");
	CBaseEntity::PrecacheModel( "models/gibs/agibs.mdl" );
	CBaseEntity::PrecacheModel ("models/weapons/v_hands.mdl");

	CBaseEntity::PrecacheScriptSound( "HUDQuickInfo.LowAmmo" );
	CBaseEntity::PrecacheScriptSound( "HUDQuickInfo.LowHealth" );

	CBaseEntity::PrecacheScriptSound( "FX_AntlionImpact.ShellImpact" );
	CBaseEntity::PrecacheScriptSound( "Missile.ShotDown" );
	CBaseEntity::PrecacheScriptSound( "Bullets.DefaultNearmiss" );
	CBaseEntity::PrecacheScriptSound( "Bullets.GunshipNearmiss" );
	CBaseEntity::PrecacheScriptSound( "Bullets.StriderNearmiss" );

	CBaseEntity::PrecacheScriptSound( "Geiger.BeepHigh" );
	CBaseEntity::PrecacheScriptSound( "Geiger.BeepLow" );

	// S:O - Precache Sounds
	CBaseEntity::PrecacheScriptSound( "ZombifyFirst" );
	CBaseEntity::PrecacheScriptSound( "Zombify" );
	CBaseEntity::PrecacheScriptSound( "Reload" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( pEdict );

	if ( pPlayer )
	{
		if ( gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME )
		{		
			// respawn player
			pPlayer->Spawn();			
		}
		else
		{
			pPlayer->SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}
}

void GameStartFrame( void )
{
	VPROF("GameStartFrame()");
	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = (teamplay.GetInt() != 0);

//#ifdef DEBUG
	extern void Bot_RunAll();
	Bot_RunAll();
//#endif
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	// vanilla deathmatch
	CreateGameRulesObject( "CHL2MPRules" );
}