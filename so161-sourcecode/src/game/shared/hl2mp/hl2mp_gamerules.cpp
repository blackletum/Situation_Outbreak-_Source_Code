//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "ammodef.h"
#include "hl2_shareddefs.h" //AI Patch Addition.

// S:O - Additional Includes
#include "view_shared.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"

// S:O - Additional ConVar Declarations
//ConVar so_noblock( "so_noblock", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "If enabled, players will not collide with NPCs, and vise versa");
ConVar so_zombie_health( "so_zombie_health", "500", FCVAR_NOTIFY | FCVAR_REPLICATED, "Amount of health Zombies start with when they spawn");
ConVar has_round_ended( "has_round_ended", "0", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar so_gamemode( "so_gamemode", "Infection", FCVAR_NOTIFY | FCVAR_REPLICATED, "Sets the default gamemode to use if a particular map does not define one for one reason or another");
ConVar sk_survivor_weapon_damage( "sk_survivor_weapon_damage", "5", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar sk_floorturret_damage( "sk_floorturret_damage", "3", FCVAR_NOTIFY | FCVAR_REPLICATED );
//ConVar so_radar( "so_radar", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "If enabled, living Survivors will be able to use SO's built-in map overview/radar system");
ConVar so_respawn("so_respawn", "1", FCVAR_REPLICATED, "Allow players to respawn (cannot be altered)");
ConVar so_survivor_count("so_survivor_count", "0", FCVAR_HIDDEN | FCVAR_REPLICATED, "Total survivor count - NPCs and players - on the map");
ConVar so_zombie_count("so_zombie_count", "0", FCVAR_HIDDEN | FCVAR_REPLICATED, "Total zombie count - NPCs and players - on the map");
ConVar so_military_count("so_military_count", "0", FCVAR_HIDDEN | FCVAR_REPLICATED, "Total military count - NPCs and players - on the map");
ConVar so_music( "so_music", "1", FCVAR_REPLICATED, "Enables/disables default S:O background and ambient music");

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else

	#include "eventqueue.h"
	#include "player.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "mapentities.h"
	#include "in_buttons.h"
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "hl2mp_player.h"
	#include "weapon_hl2mpbasehlmpcombatweapon.h"
	#include "team.h"
	#include "voice_gamemgr.h"
	#include "hl2mp_gameinterface.h"
	#include "hl2mp_cvars.h"

	// S:O - Additional Includes
	#include "monstermaker.h"
	#include "ai_behavior_assault.h"
	#include "lua/zms_lua.h"
	#include "lua/zms_luagameplay.h"
//#ifdef DEBUG	
	#include "hl2mp_bot_temp.h"
//#endif

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

// Utility function
bool FindInList( const char **pStrings, const char *pToFind )
{
	int i = 0;
	while ( pStrings[i][0] != 0 )
	{
		if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
			return true;
		i++;
	}

	return false;
}

// S:O - Additional ConVar Declarations
ConVar did_zombie_spawn( "did_zombie_spawn", "0", FCVAR_GAMEDLL | FCVAR_CHEAT );
ConVar zombification_max ( "zombification_max", "30", FCVAR_GAMEDLL | FCVAR_NOTIFY, "" );
ConVar zombification_min ( "zombification_min", "15", FCVAR_GAMEDLL | FCVAR_NOTIFY, "" );

ConVar so_reaper_health( "so_reaper_health", "25", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health reapers (fast zombies) are given when they spawn.");
ConVar so_reaper_nojump_health( "so_reaper_nojump_health", "25", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health non-jumping reapers (fast zombies that don't jump) are given when they spawn.");
ConVar so_cloud_health( "so_cloud_health", "5000", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health clouds are given when they spawn.");
ConVar so_seeker_health( "so_seeker_health", "75", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health seekers (heavy/poison zombies) are given when they spawn.");
ConVar so_creeper_health( "so_creeper_health", "50", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health creepers (normal/classic zombies) are given when they spawn.");
ConVar so_sploder_health( "so_sploder_health", "100", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health 'sploders (normal/classic zombies that explode) are given when they spawn.");
ConVar so_soldier_health( "so_soldier_health", "125", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health military soldier NPCs are given when they spawn." );
ConVar so_survivor_health( "so_survivor_health", "100", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health survivor NPCs are given when they spawn." );
ConVar so_ghost_health( "so_ghost_health", "1000", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much health ghost are given when they spawn.");

ConVar so_reaper_health_dd( "so_reaper_health_dd", "25", FCVAR_GAMEDLL, "How much health reapers (fast zombies) are given when they spawn.");
ConVar so_reaper_nojump_health_dd( "so_reaper_nojump_health_dd", "25", FCVAR_GAMEDLL, "How much health non-jumping reapers (fast zombies that don't jump) are given when they spawn.");
ConVar so_seeker_health_dd( "so_seeker_health_dd", "75", FCVAR_GAMEDLL, "How much health seekers (heavy/poison zombies) are given when they spawn.");
ConVar so_creeper_health_dd( "so_creeper_health_dd", "50", FCVAR_GAMEDLL, "How much health creepers (normal/classic zombies) are given when they spawn.");
ConVar so_sploder_health_dd( "so_sploder_health_dd", "100", FCVAR_GAMEDLL, "How much health 'sploders (normal/classic zombies that explode) are given when they spawn.");

ConVar so_dynamic_difficulty( "so_dynamic_difficulty", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Enable/Disable primary dynamic difficulty functionality." );
ConVar so_dyndiff_level( "so_dyndiff_level", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Used to adjust dynamic difficulty. Only used if so_dynamic_difficulty is set to 1." );
ConVar so_respawn_delay("so_respawn_delay", "0.5", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines how often (in minutes) reinforcements are deployed if respawn is enabled.");
ConVar so_boss_system("so_boss_system", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Enables/disables the boss system for Survival, Holdout and Objective gamemodes.");

ConVar so_infection_system("so_infection_system", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Enables/disables the infection system");
ConVar so_infection_chance("so_infection_chance", "50", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines the inverse of the chance of infection (EX: a value of 50 here means 1/50 chance of infection)");
ConVar so_infection_delay("so_infection_delay", "10.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "How much time should elapse before a player's infection is complete");

extern ConVar so_roundtime;

ConVar sv_hl2mp_weapon_respawn_time( "sv_hl2mp_weapon_respawn_time", "20", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_hl2mp_item_respawn_time( "sv_hl2mp_item_respawn_time", "30", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_report_client_settings("sv_report_client_settings", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );

extern ConVar mp_chattime;

// S:O - Misc.
//extern CBaseEntity	 *g_pLastCombineSpawn;
//extern CBaseEntity	 *g_pLastRebelSpawn;
extern CBaseEntity	 *g_pLastPlayerSpawn;

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

#endif


REGISTER_GAMERULES_CLASS( CHL2MPRules );

BEGIN_NETWORK_TABLE_NOBASE( CHL2MPRules, DT_HL2MPRules )

	#ifdef CLIENT_DLL
		RecvPropBool( RECVINFO( m_bTeamPlayEnabled ) ),
		RecvPropFloat( RECVINFO( m_fStart) ),
		RecvPropInt( RECVINFO( m_iDuration) ),
		RecvPropFloat( RECVINFO( m_fReinforcementsStart) ),
		RecvPropInt( RECVINFO( m_iReinforcementsDuration) ),
	#else
		SendPropBool( SENDINFO( m_bTeamPlayEnabled ) ),
		SendPropFloat( SENDINFO( m_fStart) ),
		SendPropInt( SENDINFO( m_iDuration) ),
		SendPropFloat( SENDINFO( m_fReinforcementsStart) ),
		SendPropInt( SENDINFO( m_iReinforcementsDuration) ),
	#endif

END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( hl2mp_gamerules, CHL2MPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )

static HL2MPViewVectors g_HL2MPViewVectors(
	Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)

	// S:O - Make it so the eye height while ducking is more realistic. (ORIGINAL VALUE: 28)
	Vector( 0, 0, 32 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)

	// S:O - For the sake of consistency, change this to a more reasonable value. (ORIGINAL VALUE: 14)
	Vector( 0, 0, 16 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 )	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

static const char *s_PreserveEnts[] =
{
	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"zms_spawnpoint",	// S:O - Additional spawnpoint entity (legacy support)
	"so_spawnpoint",	// S:O - Additional spawnpoint entity
	"so_ext_survivor_spawnpoint",	// S:O - Additional spawnpoint entity
	"so_ext_military_spawnpoint",	// S:O - Additional spawnpoint entity
	"info_player_deathmatch",
	"info_player_start",	// S:O - Additional spawnpoint entity (legacy support)
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"so_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"assault_rallypoint", // S:O - We want rallypoints to stay as well so NPCs can still use them.
	"", // END Marker
};

#ifdef CLIENT_DLL
	void RecvProxy_HL2MPRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		RecvPropDataTable( "hl2mp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPRules ), RecvProxy_HL2MPRules )
	END_RECV_TABLE()
#else
	void* SendProxy_HL2MPRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		SendPropDataTable( "hl2mp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2MPRules ), SendProxy_HL2MPRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			return ( pListener->GetTeamNumber() == pTalker->GetTeamNumber() );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif

// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"The Deceased",
	"Undead",
	"Survivors",
	"Military",
	"VIP",
};

CHL2MPRules::CHL2MPRules()
{
#ifndef CLIENT_DLL
	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));
		pTeam->Init( sTeamNames[i], i );

		g_Teams.AddToTail( pTeam );
	}
	InitDefaultAIRelationships(); //AI Patch Addition.

	// S:O - We ALWAYS have teamplay enabled for this mod.
	m_bTeamPlayEnabled = true;
	m_flIntermissionEndTime = 0.0f;
	m_flGameStartTime = 0;

	m_hRespawnableItemsAndWeapons.RemoveAll();
	m_tmNextPeriodicThink = 0;
	m_tmNextStateThink = 0.0f;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bHeardAllPlayersReady = false;
	m_bAwaitingReadyRestart = false;

	// S:O - Misc.
	m_fReforceTimer = 0;
	m_fDifficultyCalcTimer = 0;
	m_fDangerCalcTimer = 0;
	m_fSlowMotionTimer = 0;
	m_fStart=-1;
	m_fReinforcementsStart=-1;
	zombie_timer = 0;
	bossspawn_timer = 0;
	m_bBossSpawned = false;

	// S:O - Clear any and all lists.
	gEntList.m_SurvivorList.Purge();
	gEntList.m_ZombieList.Purge();
	gEntList.m_MilitaryList.Purge();
	gEntList.m_PlayerList.Purge();
	gEntList.m_EscapedList.Purge();

	// S:O - Stores the number of players on each team if the gamemode is Extermination, otherwise both are likely 0
	// We're using our own variables here to ensure we have the correct number of players on each team at any given moment
	//m_iNumSurvivors = 0;
	//m_iNumMilitary = 0;

	// S:O - LUA
	/*
	if(!GetLuaHandle())
	{
		//Msg("Making new LUA!\n");
		new MyLuaHandle();
	}

	if(!GetGP())
	{
		//Msg("Making new gameplay!\n");
		new ZMSLuaGamePlay();
	}
*/
#endif
}

const CViewVectors* CHL2MPRules::GetViewVectors()const
{
	return &g_HL2MPViewVectors;
}

const HL2MPViewVectors* CHL2MPRules::GetHL2MPViewVectors()const
{
	return &g_HL2MPViewVectors;
}
	
CHL2MPRules::~CHL2MPRules( void )
{
#ifndef CLIENT_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
#endif
}

void CHL2MPRules::CreateStandardEntities( void )
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.

	BaseClass::CreateStandardEntities();

	// S:O - Misc.
	//g_pLastCombineSpawn = NULL;
	//g_pLastRebelSpawn = NULL;
	g_pLastPlayerSpawn = NULL;

#ifdef _DEBUG
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create( "hl2mp_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHL2MPRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( weaponstay.GetInt() > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return 0;		// weapon respawns almost instantly
		}
	}

	return sv_hl2mp_weapon_respawn_time.GetFloat();
#endif

	return 0;		// weapon respawns almost instantly
}


bool CHL2MPRules::IsIntermission( void )
{
#ifndef CLIENT_DLL
	return m_flIntermissionEndTime > gpGlobals->curtime;
#endif

	return false;
}

void CHL2MPRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	if ( IsIntermission() )
		return;

	BaseClass::PlayerKilled( pVictim, info );
#endif
}

// S:O - Do a bunch of shit before we initialize a new round.
#ifndef CLIENT_DLL
void CHL2MPRules::RoundRestart()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			// S:O - Taken from RestartGame()
			if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			{
				// If they're in a vehicle, make sure they get out!
				if ( pPlayer->IsInAVehicle() )
					pPlayer->LeaveVehicle();

				QAngle angles = pPlayer->GetLocalAngles();

				angles.x = 0;
				angles.z = 0;

				pPlayer->SetLocalAngles( angles );
			}

			respawn( pPlayer, false );

			// S:O - Stop those stupid looping sounds
			// TODO: Figure out if this is still needed at this point...
			pPlayer->StopSound("sound/npc/fast_zombie/gurgle_loop1.wav");
			pPlayer->StopSound("sound/npc/fast_zombie/breathe_loop1.wav");
			pPlayer->StopSound("sound/npc/turret_floor/alarm.wav");
			pPlayer->StopSound("sound/npc/turret_floor/alert.wav");
		}
	}

	CleanUpMap();

	///// Round Timer /////
	int iRound = so_roundtime.GetInt();
	int result = (iRound * 60 );
	StartRoundtimer(result);
	///////////////////////

	// Zombie Stuff
	// Jordan @ Feb 10th
	zombie_min = zombification_min.GetInt();
	zombie_max = zombification_max.GetInt();
	zombie_result = random->RandomInt(zombie_min, zombie_max);
	zombie_timer = GetRoundtimerRemain2() - zombie_result;

	// Boss Stuff
	bossspawn_min = 60;
	bossspawn_max = result - 120;
	bossspawn_result = random->RandomInt(bossspawn_min, bossspawn_max);
	bossspawn_timer = GetRoundtimerRemain2() - bossspawn_result;

	DevMsg("A new round has started!\n");

	IGameEvent * event = gameeventmanager->CreateEvent( "round_start" );

	if ( event )
	{
		event->SetInt("fraglimit", 0 );
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted

		if (FStrEq(so_gamemode.GetString(), "Infection"))
		{
			event->SetString("objective","Infection");
		}
		else if (FStrEq(so_gamemode.GetString(), "Survival"))
		{
			event->SetString("objective","Survival");
		}
		else if (FStrEq(so_gamemode.GetString(), "Holdout"))
		{
			event->SetString("objective","Holdout");
		}
		else if (FStrEq(so_gamemode.GetString(), "Escape"))
		{
			event->SetString("objective","Escape");
		}
		else if (FStrEq(so_gamemode.GetString(), "VIP"))
		{
			event->SetString("objective","VIP");
		}
		else if (FStrEq(so_gamemode.GetString(), "Hybrid"))
		{
			event->SetString("objective","Hybrid");
		}
		else if ( FStrEq( so_gamemode.GetString(), "Objective" ) )
		{
			event->SetString("objective","Objective");
		}
		else if ( FStrEq( so_gamemode.GetString(), "Overlord" ) )
		{
			event->SetString("objective","Overlord");
		}
		else if (FStrEq(so_gamemode.GetString(), "Extermination"))
		{
			event->SetString("objective","Extermination");
		}
		else
		{
			event->SetString("objective","UNKNOWN");
		}

		gameeventmanager->FireEvent( event );
	}

	// S:O - Clear Lists
	gEntList.m_SurvivorList.Purge();
	gEntList.m_ZombieList.Purge();
	gEntList.m_MilitaryList.Purge();
	//gEntList.m_PlayerList.Purge();
	gEntList.m_EscapedList.Purge();

	SetRoundState( RoundStarted, (float)result );
}
#endif

#ifndef CLIENT_DLL
// S:O - Spawns a boss that the Survivors must fight
void CHL2MPRules::SpawnBoss( void )
{
	CAI_BaseNPC *pCurrentBossCloud = dynamic_cast< CAI_BaseNPC * >( gEntList.FindEntityByName( NULL, "npc_cloud" ) );
	CAI_BaseNPC *pCurrentBossGhost = dynamic_cast< CAI_BaseNPC * >( gEntList.FindEntityByName( NULL, "npc_ghost" ) );

	// If there's already a boss somewhere on the map, don't spawn another one. (this is a double-check to ensure we didn't miss anything)
	if ( !pCurrentBossCloud && !pCurrentBossGhost )
	{
		CAI_BaseNPC *pNewBoss = NULL;

		int RandBoss = random->RandomInt( 1, 2 );
		if ( RandBoss == 1 )
			pNewBoss = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName( "npc_cloud" ) );
		else
			pNewBoss = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName( "npc_ghost" ) );

		if ( !pNewBoss )
		{
			Warning( "BOSS SPAWNER: Unable to spawn boss. Invalid NPC defined!" );
			return;
		}

		pNewBoss->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
		pNewBoss->AddSpawnFlags( SF_NPC_FADE_CORPSE );

		// All boss NPCs have a very specific spawn function that will take care of the rest...
		// In other words, nothing more should have to be done here! Spawn the NPC!
		DispatchSpawn( pNewBoss );
	}
}
#endif

void CHL2MPRules::Think( void )
{
#ifndef CLIENT_DLL
	
	CGameRules::Think();

	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			ChangeLevel(); // intermission is over
		}

		return;
	}

//	float flTimeLimit = mp_timelimit.GetFloat() * 60;
	float flFragLimit = fraglimit.GetFloat();
	
	if ( GetMapRemainingTime() < 0 )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		if( IsTeamplay() == true )
		{
			CTeam *pCombine = g_Teams[TEAM_ZOMBIES];
			CTeam *pRebels = g_Teams[TEAM_SURVIVORS];
			CTeam *pMilitary = g_Teams[TEAM_MILITARY];

			if ( pCombine->GetScore() >= flFragLimit || pRebels->GetScore() >= flFragLimit || pMilitary->GetScore() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
		else
		{
			// check if any player is over the frag limit
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
				{
					GoToIntermission();
					return;
				}
			}
		}
	}

	// S:O - Recalculates the amount of zombies and military units - NPCs and players - there are in a map at any given time
	// This is useful for HUD elements or other client-sided stuff that cannot use entity lists to determine these kinds of things
	if ( gpGlobals->curtime > m_fDangerCalcTimer )
	{
		so_survivor_count.SetValue( CalculateSurvivorCount() );
		so_zombie_count.SetValue( CalculateZombieCount() );
		so_military_count.SetValue( CalculateMilitaryCount() );

		m_fDangerCalcTimer = gpGlobals->curtime + 2.5;
	}

	if ( gpGlobals->curtime > m_tmNextStateThink )
	{		
		CheckAllPlayersReady();
		CheckRestartGame();
		HandleRoundState();

		m_tmNextStateThink = gpGlobals->curtime + 1.0;
	}

	if ( m_flRestartGameTime > 0.0f && m_flRestartGameTime <= gpGlobals->curtime )
	{
		RestartGame();
	}

	if( m_bAwaitingReadyRestart && m_bHeardAllPlayersReady )
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "All players ready. Game will restart in 5 seconds" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "All players ready. Game will restart in 5 seconds" );

		m_flRestartGameTime = gpGlobals->curtime + 5;
		m_bAwaitingReadyRestart = false;
	}

	ManageObjectRelocation();

	// S:O - Slow Motion Effect
	// TODO: Make this work without the need for sv_cheats to be enabled.
	/*if ( gpGlobals->curtime >= m_fSlowMotionTimer )
	{
		if ( SlowMotion )
		{
			ConVar *pHostTimescale = cvar->FindVar("host_timescale");
			pHostTimescale->SetValue( "0.3" );

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer )
				{
					engine->ClientCommand( pPlayer->edict(), "client_timescale 0.3" );

					const Vector *vOrigin = &pPlayer->GetAbsOrigin();
					CSingleUserRecipientFilter filter( pPlayer );
					pPlayer->EmitSound(filter, 0, "ZMS.BulletTime", vOrigin);
				}
			}

			// Set to false and check in three seconds to return to normal next check.
			// 3.0 seconds is approximately the length of our sound effect bit.
			// HOWEVER, remember, our slow motion slows down TIMERS too, which is why the value is 0.9 (3.0 x 0.3 ... omg math), NOT 3.0!!
			// Finally, start our anti-spam timer.

			SlowMotion = false;
			m_fSlowMotionTimer = gpGlobals->curtime + 0.9;
			m_SlowMotionAntiSpamTimer.Start( 60.0 );
		}
		else
		{
			ConVar *pHostTimescale = cvar->FindVar("host_timescale");
			pHostTimescale->SetValue( "1.0" );

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer )
				{
					engine->ClientCommand( pPlayer->edict(), "client_timescale 1.0" );
				}
			}

			// Check every sixty seconds, just to be sure nothing has gone wrong.
			m_fSlowMotionTimer = gpGlobals->curtime + 60.0;
		}
	}*/

	// S:O -
	// The Survival gamemode requires player respawn to be disabled at all times.
	// UPDATE: We're restricting respawn for the Objective gamemode as well, just to keep things interesting.
	// ANOTHER UPDATE: The Extermination gamemode does not allow respawning, either.
	if ( ( (FStrEq(so_gamemode.GetString(), "Survival")) || (FStrEq(so_gamemode.GetString(), "Objective")) || (FStrEq(so_gamemode.GetString(), "Extermination")) ) && (so_respawn.GetInt() == 1) )
	{
		so_respawn.SetValue(0);
	}
	else if ( ( (FStrEq(so_gamemode.GetString(), "Infection")) || (FStrEq(so_gamemode.GetString(), "Holdout")) || (FStrEq(so_gamemode.GetString(), "Escape")) || (FStrEq(so_gamemode.GetString(), "Overlord")) ) && (so_respawn.GetInt() == 0) )
	{
		so_respawn.SetValue(1);
	}

	// S:O - Do not allow players to use their radar in the Extermination gamemode (it's cheap)
	/*if ( FStrEq(so_gamemode.GetString(), "Extermination") )
	{
		so_radar.SetValue(0);
	}*/

	// S:O - Reinforcements Timer
	if( gpGlobals->curtime > m_fReforceTimer )
	{
		if ( so_respawn.GetInt() != 0 && !g_fGameOver && !RoundNowEnding && !RoundEndOverlayFired )
		{
			DevMsg( "REINFORCEMENTS SYSTEM: Respawning dead players...\n" );

			for (int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

				if ( pPlayer && pPlayer->GetTeamNumber() == 1 )
				{
					UTIL_ClientPrintAll( HUD_PRINTCENTER, "Reinforcements have arrived!" );
					pPlayer->PickDefaultSpawnTeam();

					// Do another check after picking our default spawn team.
					// We don't want to spawn spectators, now do we?
					if ( pPlayer->GetTeamNumber() != 1 )
					{
						pPlayer->Spawn();
					}
				}
			}

			DevMsg( "REINFORCEMENTS SYSTEM: ...all dead players respawned successfully!\n" );
		}

		float iReRound = so_respawn_delay.GetFloat();
		int Reresult = (iReRound * 60);
		StartReinforcementsTimer(Reresult);

		m_fReforceTimer = gpGlobals->curtime + 30.0;
	}

	// S:O - Recalculate Dynamic Difficulty
	if ( gpGlobals->curtime >= m_fDifficultyCalcTimer )
	{
		// Don't allow us to do this under the Extermination gamemode!
		if ( !FStrEq(so_gamemode.GetString(), "Extermination") && (so_dynamic_difficulty.GetInt() == 1) )
		{
			RecalcDifficulty();
		}

		m_fDifficultyCalcTimer = gpGlobals->curtime + 10.0f;
	}

	// S:O - Misc.
	// Jordan @ Feb 10th
	if ( (FStrEq(so_gamemode.GetString(), "Infection") || FStrEq(so_gamemode.GetString(), "Escape") || FStrEq(so_gamemode.GetString(), "Overlord")) && !g_fGameOver && !RoundNowEnding && !RoundEndOverlayFired )
	{
		if ( (zombie_spawned == false) && (GetRoundtimerRemain2() <= zombie_timer) )
		{
			if( GetAlivePlayers(2) == 0 && GetAlivePlayers(3) != 0 )
			{
				ZombifyFirstPlayer();
				//PickVIP();
			}
		}
		else if ( (zombie_spawned == true) && (did_zombie_spawn.GetInt() == 1) && (GetRoundtimerRemain2() <= zombie_timer) && (g_Teams[TEAM_ZOMBIES]->GetNumPlayers() == 0) && (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() == 0) && (g_Teams[TEAM_SURVIVORS]->GetNumPlayers() != 0) )
		{
			// A zombie spawned at some point this round, but there are none to be found and there aren't any players waiting to respawn, either.
			// In this case, we don't want players to have to wait until a new player joins the game, so let's spawn another zombie or two.
			ZombifyFirstPlayer();
		}
	}

	// S:O -
	// If no players were not connected last time we checked but now there are some, restart our current round as soon as possible.
	if ( (!PlayersAreConnected) && ( (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() != 0) || (g_Teams[TEAM_ZOMBIES]->GetNumPlayers() != 0) || (g_Teams[TEAM_SURVIVORS]->GetNumPlayers() != 0) || (g_Teams[TEAM_MILITARY]->GetNumPlayers() != 0) ) )
	{
		SetRoundState( RoundEnding, 0.1f );
	}

	// S:O - Check to see if there are even players on the server before we do anything else.
	if ( (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() != 0) || (g_Teams[TEAM_ZOMBIES]->GetNumPlayers() != 0) || (g_Teams[TEAM_SURVIVORS]->GetNumPlayers() != 0) || (g_Teams[TEAM_MILITARY]->GetNumPlayers() != 0) )
		PlayersAreConnected = true;
	else
		PlayersAreConnected = false;

	// S:O - Round-specific stuff.
	if ( PlayersAreConnected && !g_fGameOver && !RoundNowEnding && !RoundEndOverlayFired )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
		if ( event )
		{
			event->SetBool("premature",false);
			gameeventmanager->FireEvent( event );
		}

		if ( GetRoundtimerRemain2() <= 0 )
		{
			if ( FStrEq( so_gamemode.GetString(), "Infection" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 204;
				tTextParam.g1			= 204;
				tTextParam.b1			= 204;
				tTextParam.a1			= 255;
				tTextParam.r2			= 204;
				tTextParam.g2			= 204;
				tTextParam.b2			= 204;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Survivors fought off the Undead!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-survivorswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_SURVIVORS )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Survival" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 204;
				tTextParam.g1			= 204;
				tTextParam.b1			= 204;
				tTextParam.a1			= 255;
				tTextParam.r2			= 204;
				tTextParam.g2			= 204;
				tTextParam.b2			= 204;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Survivors fought off the zombie horde!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-survivorswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_SURVIVORS )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Overlord" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 204;
				tTextParam.g1			= 204;
				tTextParam.b1			= 204;
				tTextParam.a1			= 255;
				tTextParam.r2			= 204;
				tTextParam.g2			= 204;
				tTextParam.b2			= 204;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Survivors defended themselves against the zombie overlord!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-survivorswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_SURVIVORS )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Holdout" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 204;
				tTextParam.g1			= 204;
				tTextParam.b1			= 204;
				tTextParam.a1			= 255;
				tTextParam.r2			= 204;
				tTextParam.g2			= 204;
				tTextParam.b2			= 204;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Survivors defended the holdout zone from the zombie horde!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-survivorswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_SURVIVORS )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Escape" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Undead prevented the Survivors from escaping!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Objective" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Survivors failed to complete their objectives!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Extermination" ) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "Round Draw!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-rounddraw.mp3" );
					}
				}

				GetGlobalTeam( TEAM_SURVIVORS )->AddScore( 1 );
				GetGlobalTeam( TEAM_MILITARY )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",false);
					gameeventmanager->FireEvent( event );
				}
			}
		}
		else if ( GetRoundtimerRemain2() > 0 )
		{
			if ( (FStrEq( so_gamemode.GetString(), "Survival" ) || FStrEq( so_gamemode.GetString(), "Holdout" )) && GetAlivePlayers(3) == 0 )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The zombie horde annihilated the Survivors!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",true);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Extermination" ) )
			{
				if ( (GetAlivePlayers(3) == 0) && (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() > 0) )
				{
					hudtextparms_s tTextParam;
					tTextParam.x			= -1;
					tTextParam.y			= -1;
					tTextParam.effect		= 1;
					tTextParam.r1			= 210;
					tTextParam.g1			= 105;
					tTextParam.b1			= 30;
					tTextParam.a1			= 255;
					tTextParam.r2			= 210;
					tTextParam.g2			= 105;
					tTextParam.b2			= 30;
					tTextParam.a2			= 255;
					tTextParam.fadeinTime	= 1.0;
					tTextParam.fadeoutTime	= 1.0;
					tTextParam.holdTime		= 5.0;
					tTextParam.fxTime		= 1.0;
					tTextParam.channel		= 1;
					UTIL_HudMessageAll( tTextParam, "The Military have crushed the Survivors!\n" );

					color32 fadeblack = {0,0,0,200};
					UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

					for ( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

						if ( pPlayer )
						{
							engine->ClientCommand( pPlayer->edict(), "play events/so-militarywin.mp3" );
						}
					}

					GetGlobalTeam( TEAM_MILITARY )->AddScore( 1 );
					SetRoundState( RoundEnding, 10.0f );
					RoundEndOverlayFired = true;

					IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
					if ( event )
					{
						event->SetBool("premature",true);
						gameeventmanager->FireEvent( event );
					}
				}
				else if ( (GetAlivePlayers(4) == 0) && (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() > 0) )
				{
					hudtextparms_s tTextParam;
					tTextParam.x			= -1;
					tTextParam.y			= -1;
					tTextParam.effect		= 1;
					tTextParam.r1			= 204;
					tTextParam.g1			= 204;
					tTextParam.b1			= 204;
					tTextParam.a1			= 255;
					tTextParam.r2			= 204;
					tTextParam.g2			= 204;
					tTextParam.b2			= 204;
					tTextParam.a2			= 255;
					tTextParam.fadeinTime	= 1.0;
					tTextParam.fadeoutTime	= 1.0;
					tTextParam.holdTime		= 5.0;
					tTextParam.fxTime		= 1.0;
					tTextParam.channel		= 1;
					UTIL_HudMessageAll( tTextParam, "The Survivors have eliminated the Military!\n" );

					color32 fadeblack = {0,0,0,200};
					UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

					for ( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

						if ( pPlayer )
						{
							engine->ClientCommand( pPlayer->edict(), "play events/so-survivorswin.mp3" );
						}
					}

					GetGlobalTeam( TEAM_SURVIVORS )->AddScore( 1 );
					SetRoundState( RoundEnding, 10.0f );
					RoundEndOverlayFired = true;

					IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
					if ( event )
					{
						event->SetBool("premature",true);
						gameeventmanager->FireEvent( event );
					}
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Objective" ) && GetAlivePlayers(3) == 0 )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The zombie horde annihilated the Survivors!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",true);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Infection" ) && zombie_spawned && GetAlivePlayers(3) == 0 && GetAlivePlayers(2) > 0 && (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() + g_Teams[TEAM_SURVIVORS]->GetNumPlayers() + g_Teams[TEAM_ZOMBIES]->GetNumPlayers() > 1) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Undead annihilated the Survivors!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",true);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Escape" ) && zombie_spawned && GetAlivePlayers(3) == 0 && GetAlivePlayers(2) > 0 && (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() + g_Teams[TEAM_SURVIVORS]->GetNumPlayers() + g_Teams[TEAM_ZOMBIES]->GetNumPlayers() > 1) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The Undead annihilated the Survivors!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",true);
					gameeventmanager->FireEvent( event );
				}
			}
			else if ( FStrEq( so_gamemode.GetString(), "Overlord" ) && zombie_spawned && GetAlivePlayers(3) == 0 && GetAlivePlayers(2) > 0 && (g_Teams[TEAM_SPECTATOR]->GetNumPlayers() + g_Teams[TEAM_SURVIVORS]->GetNumPlayers() + g_Teams[TEAM_ZOMBIES]->GetNumPlayers() > 1) )
			{
				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= -1;
				tTextParam.effect		= 1;
				tTextParam.r1			= 255;
				tTextParam.g1			= 0;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 0;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 1.0;
				tTextParam.fadeoutTime	= 1.0;
				tTextParam.holdTime		= 5.0;
				tTextParam.fxTime		= 1.0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, "The zombie overlord annihilated the Survivors!\n" );

				color32 fadeblack = {0,0,0,200};
				UTIL_ScreenFadeAll( fadeblack, 3.0, 15.0, FFADE_IN );

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						engine->ClientCommand( pPlayer->edict(), "play events/so-zombieswin.mp3" );
					}
				}

				GetGlobalTeam( TEAM_ZOMBIES )->AddScore( 1 );
				SetRoundState( RoundEnding, 10.0f );
				RoundEndOverlayFired = true;

				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					event->SetBool("premature",true);
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
#endif
}

void CHL2MPRules::GoToIntermission( void )
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		pPlayer->AddFlag( FL_FROZEN );

		// S:O - Additional Intermission Stuff
		pPlayer->ChangeTeam( 1 );
		engine->ClientCommand( pPlayer->edict(), "play zms/zmsmapend.mp3" );
	}

	// S:O - Save some resources and clean everything up while waiting for a new map to be loaded during intermission.
	CleanUpMap();
#endif
}

bool CHL2MPRules::CheckGameOver()
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			ChangeLevel(); // intermission is over			
		}

		return true;
	}
#endif

	return false;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHL2MPRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}
#endif
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	CWeaponHL2MPBase *pHL2Weapon = dynamic_cast< CWeaponHL2MPBase*>( pWeapon );

	if ( pHL2Weapon )
	{
		return pHL2Weapon->GetOriginalSpawnOrigin();
	}
#endif
	
	return pWeapon->GetAbsOrigin();
}

#ifndef CLIENT_DLL

CItem* IsManagedObjectAnItem( CBaseEntity *pObject )
{
	return dynamic_cast< CItem*>( pObject );
}

CWeaponHL2MPBase* IsManagedObjectAWeapon( CBaseEntity *pObject )
{
	return dynamic_cast< CWeaponHL2MPBase*>( pObject );
}

bool GetObjectsOriginalParameters( CBaseEntity *pObject, Vector &vOriginalOrigin, QAngle &vOriginalAngles )
{
	if ( CItem *pItem = IsManagedObjectAnItem( pObject ) )
	{
		if ( pItem->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;
		
		vOriginalOrigin = pItem->GetOriginalSpawnOrigin();
		vOriginalAngles = pItem->GetOriginalSpawnAngles();

		pItem->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_item_respawn_time.GetFloat();
		return true;
	}
	else if ( CWeaponHL2MPBase *pWeapon = IsManagedObjectAWeapon( pObject )) 
	{
		if ( pWeapon->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;

		vOriginalOrigin = pWeapon->GetOriginalSpawnOrigin();
		vOriginalAngles = pWeapon->GetOriginalSpawnAngles();

		pWeapon->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_weapon_respawn_time.GetFloat();
		return true;
	}

	return false;
}

void CHL2MPRules::ManageObjectRelocation( void )
{
	int iTotal = m_hRespawnableItemsAndWeapons.Count();

	if ( iTotal > 0 )
	{
		for ( int i = 0; i < iTotal; i++ )
		{
			CBaseEntity *pObject = m_hRespawnableItemsAndWeapons[i].Get();
			
			if ( pObject )
			{
				Vector vSpawOrigin;
				QAngle vSpawnAngles;

				if ( GetObjectsOriginalParameters( pObject, vSpawOrigin, vSpawnAngles ) == true )
				{
					float flDistanceFromSpawn = (pObject->GetAbsOrigin() - vSpawOrigin ).Length();

					if ( flDistanceFromSpawn > WEAPON_MAX_DISTANCE_FROM_SPAWN )
					{
						bool shouldReset = false;
						IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

						if ( pPhysics )
						{
							shouldReset = pPhysics->IsAsleep();
						}
						else
						{
							shouldReset = (pObject->GetFlags() & FL_ONGROUND) ? true : false;
						}

						if ( shouldReset )
						{
							pObject->Teleport( &vSpawOrigin, &vSpawnAngles, NULL );
							pObject->EmitSound( "AlyxEmp.Charge" );

							IPhysicsObject *pPhys = pObject->VPhysicsGetObject();

							if ( pPhys )
							{
								pPhys->Wake();
							}
						}
					}
				}
			}
		}
	}
}

//=========================================================
//AddLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::AddLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) == -1 )
	{
		m_hRespawnableItemsAndWeapons.AddToTail( pEntity );
	}
}

//=========================================================
//RemoveLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) != -1 )
	{
		m_hRespawnableItemsAndWeapons.FindAndRemove( pEntity );
	}
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

//=========================================================
// What angles should this item use to respawn?
//=========================================================
QAngle CHL2MPRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHL2MPRules::FlItemRespawnTime( CItem *pItem )
{
	return sv_hl2mp_item_respawn_time.GetFloat();
}


//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHL2MPRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
	if ( weaponstay.GetInt() > 0 )
	{
		if ( pPlayer->Weapon_OwnsThisType( pItem->GetClassname(), pItem->GetSubType() ) )
			 return false;
	}

	return BaseClass::CanHavePlayerItem( pPlayer, pItem );
}

#endif

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHL2MPRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
	// S:O - Weapons never respawn in this mod until the start of a new round.
/*#ifndef CLIENT_DLL
	if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
	{
		return GR_WEAPON_RESPAWN_NO;
	}
#endif

	return GR_WEAPON_RESPAWN_YES;*/

	return GR_WEAPON_RESPAWN_NO;
}

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CHL2MPRules::ClientDisconnected( edict_t *pClient )
{
#ifndef CLIENT_DLL
	// Msg( "CLIENT DISCONNECTED, REMOVING FROM TEAM.\n" );

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( pPlayer )
	{
		// Remove the player from his team
		if ( pPlayer->GetTeam() )
		{
			pPlayer->GetTeam()->RemovePlayer( pPlayer );
		}
	}

	BaseClass::ClientDisconnected( pClient );

#endif
}


//=========================================================
// Deathnotice. 
//=========================================================
void CHL2MPRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	// Custom kill type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( strstr( killer_weapon_name, "physics" ) )
		{
			killer_weapon_name = "physics";
		}

		if ( strcmp( killer_weapon_name, "prop_combine_ball" ) == 0 )
		{
			killer_weapon_name = "combine_ball";
		}
		else if ( strcmp( killer_weapon_name, "grenade_ar2" ) == 0 )
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if ( strcmp( killer_weapon_name, "satchel" ) == 0 || strcmp( killer_weapon_name, "tripmine" ) == 0)
		{
			killer_weapon_name = "slam";
		}

		// S:O - If the following is true, we're dealing with an NPC of some kind.
		if ( pKiller->IsNPC() )
		{
			bNPCKill = true;

			if ( V_strcmp( pKiller->GetClassname(), "npc_zombie" ) == 0)
				killer_weapon_name = "Creeper";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_fastzombie" ) == 0)
				killer_weapon_name = "Reaper";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_reaper_nojump" ) == 0)
				killer_weapon_name = "Reaper";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_poisonzombie" ) == 0)
				killer_weapon_name = "Seeker";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_sploderzombie" ) == 0)
				killer_weapon_name = "'Sploder";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_citizen" ) == 0)
				killer_weapon_name = "Survivor";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_turret_floor" ) == 0)
				killer_weapon_name = "Turret";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_headcrab" ) == 0)
				killer_weapon_name = "Headcrab";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_headcrab_black" ) == 0)
				killer_weapon_name = "Poison Headcrab";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_headcrab_fast" ) == 0)
				killer_weapon_name = "Fast Headcrab";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_combine_s" ) == 0)
				killer_weapon_name = "Soldier";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_cloud" ) == 0)
				killer_weapon_name = "Cloud";
			else if ( V_strcmp( pKiller->GetClassname(), "npc_ghost" ) == 0)
				killer_weapon_name = "Ghost";
			else
				killer_weapon_name = "Spooky Thing of Doom";	// when in doubt, go crazy!!! >=D

			if ( pKiller->GetTeamNumber() == 2 )
				bZombieNPCKill = true;
			else if ( pKiller->GetTeamNumber() == 3 )
				bSurvivorNPCKill = true;
			else if ( pKiller->GetTeamNumber() == 4 )
				bMilitaryNPCKill = true;
		}
		else
		{
			bNPCKill = false;
			bZombieNPCKill = false;
			bSurvivorNPCKill = false;
			bMilitaryNPCKill = false;
		}
	}

	// S:O - Modified "player_death" event
	IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted

		event->SetInt( "attackerhealth", max(0, pKiller->m_iHealth) );

		// Have we been hurt by a player?
		if ( pKiller->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pKiller );
			event->SetInt("attacker", pPlayer->GetUserID() );
			event->SetString("attackernpc", "null" );	// 0 when attacker is not a NPC
		}
		// Have we been hurt by an NPC?
		else if ( pKiller->IsNPC() )
		{
			event->SetInt("attacker", 0 );	// 0 when attacker is not a player
			event->SetString("attackernpc", pKiller->GetClassname() );	// returns the classname of the attacker when we're dealing with an npc
		}
		// None of the above? Then we were probably killed by the world or a physics prop.
		else
		{
			// hurt by "world"
			event->SetInt("attacker", 0 );
			event->SetString("attackernpc", "world" );
		}

		event->SetString( "inflictor_class", pKiller->GetClassname() );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );

		event->SetBool( "npckill", bNPCKill );			// S:O - True if killed by an NPC
		event->SetBool( "zombienpckill", bZombieNPCKill );	// S:O - True if killed by a zombie NPC
		event->SetBool( "survivornpckill", bSurvivorNPCKill );	// S:O - True if killed by a survivor NPC
		event->SetBool( "militarynpckill", bMilitaryNPCKill );	// S:O - True if killed by a military NPC

		event->SetString( "weapon", killer_weapon_name ); // S:O - See modevents.res...
		event->SetInt( "killerteam", pKiller->GetTeamNumber() ); // S:O - Returns the team of the killer, regardless of whether the killer is player or NPC

        gameeventmanager->FireEvent( event );
	}
#endif

}

void CHL2MPRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
/*#ifndef CLIENT_DLL
	
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer( pPlayer );

	if ( pHL2Player == NULL )
		return;

	const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
	const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

	//If we're different.
	if ( stricmp( szModelName, pCurrentModel ) )
	{
		//Too soon, set the cvar back to what it was.
		//Note: this will make this function be called again
		//but since our models will match it'll just skip this whole dealio.
		if ( pHL2Player->GetNextModelChangeTime() >= gpGlobals->curtime )
		{
			char szReturnString[512];

			Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pCurrentModel );
			engine->ClientCommand ( pHL2Player->edict(), szReturnString );

			//Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch.\n", (int)(pHL2Player->GetNextModelChangeTime() - gpGlobals->curtime) );
			//ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
			return;
		}

		if ( HL2MPRules()->IsTeamplay() == false )
		{
			pHL2Player->SetPlayerModel();

			const char *pszCurrentModelName = modelinfo->GetModelName( pHL2Player->GetModel() );

			char szReturnString[128];
			Q_snprintf( szReturnString, sizeof( szReturnString ), "Your player model is: %s\n", pszCurrentModelName );

			ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
		}
		else
		{
			if ( Q_stristr( szModelName, "models/player/Humans") )
			{
				pHL2Player->ChangeTeam( TEAM_SURVIVORS );
			}
			else
			{
				pHL2Player->ChangeTeam( TEAM_ZOMBIES );
			}
		}
	}
	if ( sv_report_client_settings.GetInt() == 1 )
	{
		UTIL_LogPrintf( "\"%s\" cl_cmdrate = \"%s\"\n", pHL2Player->GetPlayerName(), engine->GetClientConVarValue( pHL2Player->entindex(), "cl_cmdrate" ));
	}

	BaseClass::ClientSettingsChanged( pPlayer );
#endif
*/	
}

int CHL2MPRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( pPlayer->GetTeamNumber() == pTarget->GetTeamNumber() )
	{
		return GR_TEAMMATE;
	}
#endif

	return GR_NOTTEAMMATE;
}

const char *CHL2MPRules::GetGameDescription( void )
{ 
	/*if ( IsTeamplay() )
		return "Team Deathmatch"; 

	return "Deathmatch";*/

	// S:O - Default to this as a description for our game:
	return "Situation Outbreak";
} 


float CHL2MPRules::GetMapRemainingTime()
{
	// if timelimit is disabled, return 0
	if ( mp_timelimit.GetInt() <= 0 )
		return 0;

	// timelimit is in minutes

	float timeleft = (m_flGameStartTime + mp_timelimit.GetInt() * 60.0f ) - gpGlobals->curtime;

	return timeleft;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPRules::Precache( void )
{
	CBaseEntity::PrecacheScriptSound( "AlyxEmp.Charge" );

	// S:O - Precache Additional Sounds
	//CBaseEntity::PrecacheScriptSound( "Zombify" );
	CBaseEntity::PrecacheScriptSound( "ZombifyFirst" );
	CBaseEntity::PrecacheScriptSound( "Infection.First.Zombie" );
	
	// S:O - Slow Motion Effect
	//CBaseEntity::PrecacheScriptSound( "ZMS.BulletTime" );
}

bool CHL2MPRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// S:O -
	// James @ April 23, 2009
	// This prevents any combination of players and NPCs from getting stuck together.
	/*if ( so_noblock.GetInt() == 1 )
	{
		// S:O - If so_noblock is enabled, disable player/player and player/NPC collisions.
		if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup0 == COLLISION_GROUP_NPC_ACTOR || collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED || collisionGroup0 == COLLISION_GROUP_NPC ) &&
			 ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR || collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED || collisionGroup1 == COLLISION_GROUP_NPC ) )
		{
			return false;
		}
	}
	else
	{*/
		/*// S:O - Don't allow Survivors and Military to collide with other Survivors and Military
		if ( (collisionGroup0 == COLLISION_GROUP_PLAYER) && (collisionGroup1 == COLLISION_GROUP_PLAYER) )
		{
			return false;
		}

		// S:O - Don't allow Undead to collide with other Undead
		if ( (collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) && (collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT) )
		{
			return false;
		}

		// S:O - Force Undead to collide with Survivors and Military - and vise versa
		if ( (collisionGroup0 == COLLISION_GROUP_PLAYER) && (collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT) )
		{
			return true;
		}*/
	//}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 );

}

bool CHL2MPRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
#ifndef CLIENT_DLL
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;


	CHL2MP_Player *pPlayer = (CHL2MP_Player *) pEdict;

	if ( pPlayer->ClientCommand( args ) )
		return true;
#endif

	return false;
}

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;

//===================		
// AI Patch Addition.
//===================
//																								plr dmg		npc dmg								max carry	impulse
		def.AddAmmoType("9mm",				DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		99,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("AK47",				DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("TURRET",			DMG_BULLET,					TRACER_LINE,			0,			"sk_floorturret_damage",			10,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("Deagle",			DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(800, 5000),	0 );
		def.AddAmmoType("MP5K",				DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("Sniper",			DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(800, 8000),	0 );
		def.AddAmmoType("Buckshot2",		DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			"sk_survivor_weapon_damage",		50,			BULLET_IMPULSE(400, 1200),	0 );
		def.AddAmmoType("Buckshot3",		DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			"sk_survivor_weapon_damage",		50,			BULLET_IMPULSE(400, 1200),	0 );
		def.AddAmmoType("IncendiaryGrenade",DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		1,			0,							0 );
		def.AddAmmoType("Zombait",			DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		1,			0,							0 );

		def.AddAmmoType("AR2",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("AR2AltFire",		DMG_DISSOLVE,				TRACER_NONE,			0,			"sk_survivor_weapon_damage",		3,			0,							0 );
		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		99,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("MAC10",			DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("357",				DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			BULLET_IMPULSE(800, 5000),	0 );
		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			"sk_survivor_weapon_damage",		50,			BULLET_IMPULSE(400, 1200),	0 );
		def.AddAmmoType("RPG_Round",		DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		1,			0,							0 );
		def.AddAmmoType("SMG1_Grenade",		DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		3,			0,							0 );
		def.AddAmmoType("AR2_Grenade",		DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		3,			0,							0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		1,			0,							0 );
		def.AddAmmoType("slam",				DMG_BURN,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		3,			0,							0 );

		def.AddAmmoType("AlyxGun",				DMG_BULLET,					TRACER_LINE,			5,			"sk_survivor_weapon_damage",		5,			BULLET_IMPULSE(200, 1225),  0 );
		def.AddAmmoType("SniperRound",			DMG_BULLET | DMG_SNIPER,	TRACER_NONE,			5,			"sk_survivor_weapon_damage",		5,			BULLET_IMPULSE(650, 6000),  0 );
		def.AddAmmoType("SniperPenetratedRound",DMG_BULLET | DMG_SNIPER,	TRACER_NONE,			5,			"sk_survivor_weapon_damage",		5,			BULLET_IMPULSE(150, 6000),  0 );
		def.AddAmmoType("Grenade",				DMG_BURN,					TRACER_NONE,			5,			"sk_survivor_weapon_damage",		5,			0,							0 );
		def.AddAmmoType("Thumper",				DMG_SONIC,					TRACER_NONE,			10,			"sk_survivor_weapon_damage",		2,			0,							0 );
		def.AddAmmoType("Gravity",				DMG_CLUB,					TRACER_NONE,			0,			"sk_survivor_weapon_damage",		8,			0,							0 );
		def.AddAmmoType("Battery",				DMG_CLUB,					TRACER_NONE,			NULL,		NULL,								NULL,		0,							0 );
		def.AddAmmoType("GaussEnergy",			DMG_SHOCK,					TRACER_NONE,			5,			"sk_survivor_weapon_damage",		5,			BULLET_IMPULSE(650, 8000),	0 );
		def.AddAmmoType("CombineCannon",		DMG_BULLET,					TRACER_LINE,			5,			"sk_survivor_weapon_damage",		NULL,		1.5 * 750 * 12,				0 );
		def.AddAmmoType("AirboatGun",			DMG_AIRBOAT,				TRACER_LINE,			5,			"sk_survivor_weapon_damage",		NULL,		BULLET_IMPULSE(10, 600),	0 );
		def.AddAmmoType("StriderMinigun",		DMG_BULLET,					TRACER_LINE,			5,			"sk_survivor_weapon_damage",		15,			1.0 * 750 * 12,				AMMO_FORCE_DROP_IF_CARRIED );
		def.AddAmmoType("StriderMinigunDirect",	DMG_BULLET,					TRACER_LINE,			2,			"sk_survivor_weapon_damage",		15,			1.0 * 750 * 12,				AMMO_FORCE_DROP_IF_CARRIED );
		def.AddAmmoType("HelicopterGun",		DMG_BULLET,					TRACER_LINE_AND_WHIZ,	5,			"sk_survivor_weapon_damage",		5,			BULLET_IMPULSE(400, 1225),	AMMO_FORCE_DROP_IF_CARRIED | AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER );
#ifdef HL2_EPISODIC
		def.AddAmmoType("Hopwire",				DMG_BLAST,					TRACER_NONE,			5,			"sk_survivor_weapon_damage",		5,			0,							0 );
		def.AddAmmoType("CombineHeavyCannon",	DMG_BULLET,					TRACER_LINE,			40,			"sk_survivor_weapon_damage",		NULL,		10 * 750 * 12,				AMMO_FORCE_DROP_IF_CARRIED );
		def.AddAmmoType("ammo_proto1",			DMG_BULLET,					TRACER_LINE,			0,			"sk_survivor_weapon_damage",		10,			0,							0 );
#endif // HL2_EPISODIC
//========== End Of AI Patch =====
	}

	return &def;
}

#ifdef CLIENT_DLL

	ConVar cl_autowepswitch(
		"cl_autowepswitch",
		"1",
		FCVAR_ARCHIVE | FCVAR_USERINFO,
		"Automatically switch to picked up weapons (if more powerful)" );

#else

//#ifdef DEBUG

	// Handler for the "bot" command.
	void Bot_f()
	{		
		// Look at -count.
		int count = 1;
		count = clamp( count, 1, 16 );

		int iTeam = TEAM_ZOMBIES;
				
		// Look at -frozen.
		bool bFrozen = false;
			
		// Ok, spawn all the bots.
		while ( --count >= 0 )
		{
			BotPutInServer( bFrozen, iTeam );
		}
	}


	ConCommand cc_Bot( "bot", Bot_f, "Add a bot.", FCVAR_CHEAT );

//#endif

	bool CHL2MPRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{		
		if ( pPlayer->GetActiveWeapon() && pPlayer->IsNetClient() )
		{
			// Player has an active item, so let's check cl_autowepswitch.
			const char *cl_autowepswitch = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_autowepswitch" );
			if ( cl_autowepswitch && atoi( cl_autowepswitch ) <= 0 )
			{
				return false;
			}
		}

		return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
	}

#endif

#ifndef CLIENT_DLL

// S:O - Default Zombification Function
void CHL2MPRules::ZombifyPlayer(CBasePlayer *pPlayer)
{
	CHL2MP_Player *pZombie = dynamic_cast<CHL2MP_Player*>( pPlayer );
	if( !pZombie )
		return;

	ClientPrint( pZombie, HUD_PRINTTALK, "You begin to feel very, very strange...\n" );

	pZombie->MakeInfected( true );
	pZombie->ChangeTeam( 2 );
	pZombie->StartZombifyTimer( so_infection_delay.GetFloat() );

	// Reward the Undead team for their successful infection
	GetGlobalTeam( 2 )->AddScore( 1 );

	CBaseViewModel *pViewModel = pZombie->GetViewModel();
	pViewModel->RemoveEffects( EF_NODRAW );

	IGameEvent *event = gameeventmanager->CreateEvent( "ZombifyPlayer" );

	if( event )
	{
		event->SetInt("userid", pZombie->GetUserID() );
		event->SetInt("attacker", pZombie->GetUserID() );
		event->SetString("weapon", "blood" );
		event->SetInt( "priority", 7 );
		gameeventmanager->FireEvent( event );
	}
}

// S:O - VIP Selection Function
void CHL2MPRules::PickVIP()
{
	/*
		CUtlLinkedList< CHL2MP_Player* > possiblePlayers;

        for (int i = 1; i <= gpGlobals->maxClients; i++ )
        {
			CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->GetTeamNumber() == TEAM_SURVIVORS )
			{
				possiblePlayers.AddToTail( pPlayer );
			}
        }

		CHL2MP_Player *pRandomPlayer = NULL;
         if ( possiblePlayers.Count() > 0 )
			{
				// randomly pick a player from all of those we have collected
                int iRandomPicked = random->RandomInt(0,possiblePlayers.Count()-1);
                pRandomPlayer = possiblePlayers[iRandomPicked];
				iTeam = pRandomPlayer->GetTeamNumber();
				if(iTeam == TEAM_SURVIVORS)
				{
					CBasePlayer *pVIP = dynamic_cast<CBasePlayer*>( pRandomPlayer );
					pRandomPlayer->ChangeTeam(TEAM_VIP);
					pRandomPlayer->SetModel("");
				}
		 }
		 */
}

// S:O - First Zombification Function
void CHL2MPRules::ZombifyFirstPlayer()
{
	DevMsg("First zombification function called! Running random infection function...\n");

FirstInfectionRetry:

	CUtlLinkedList< CHL2MP_Player* > possiblePlayers;
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() == 3 )
		{
			possiblePlayers.AddToTail( pPlayer );
		}
	}

	if ( possiblePlayers.Count() > 0 )
	{
		// randomly pick a player from all of those we have collected
		int iRandomPicked = random->RandomInt(0,possiblePlayers.Count()-1);

		CHL2MP_Player *pZombieCheck = dynamic_cast<CHL2MP_Player*>( possiblePlayers[iRandomPicked] );

		// Someday I'll get this working! - James @ June 12, 2009
		/*if ( (pZombieCheckLast == pZombieCheck) && (g_Teams[TEAM_SURVIVORS]->GetNumPlayers() > 1) )
		{
			// We don't want to infect the same player twice unless we absolutely have to.
			// If there's only one living survivor to choose from though, we don't really have a choice, now do we?
			goto FirstInfectionRetry;
		}

		CHL2MP_Player *pZombieCheckLast = pZombieCheck;*/

		if ( !pZombieCheck || pZombieCheck->GetTeamNumber() != 3 || pZombieCheck->IsDead() )
		{
			// We cannot choose someone from a team other than the survivors or from someone who is dead, so try again!
			DevMsg("Unable to find a viable infection host. Will continue to retry until host(s) have been found...\n");
			goto FirstInfectionRetry;
		}

		DevMsg("Successfully found an infection host. Infecting host!\n");

		zombie_spawned = true;
		did_zombie_spawn.SetValue( 1 );

		if ( FStrEq(so_gamemode.GetString(), "Escape") )
			pZombieCheck->CHL2MP_Player::Spawn();
		else
			pZombieCheck->ChangeTeam(2);

		pZombieCheck->EmitSound("ZombifyFirst");

		if ( FStrEq(so_gamemode.GetString(), "Overlord") )
		{
			ClientPrint( pZombieCheck, HUD_PRINTTALK, "You are now the zombie overlord!" );
			ClientPrint( pZombieCheck, HUD_PRINTTALK, "Hold your 'r' key to spawn zombie minions." );
			ClientPrint( pZombieCheck, HUD_PRINTTALK, "Right-click to command them." );
		}
		else
		{
			ClientPrint( pZombieCheck, HUD_PRINTTALK, "You have been turned into a %s1. Eat some brains!\n", pZombieCheck->GetZombieClassName() );
		}

		CBaseViewModel *pViewModel = pZombieCheck->GetViewModel();
		pViewModel->RemoveEffects( EF_NODRAW );

		if ( zombie_spawned )
		{
			IGameEvent * event = gameeventmanager->CreateEvent( "ZombifyFirstZombie" );
			if ( event )
			{
				event->SetInt( "userid", pZombieCheck->GetUserID() );
				gameeventmanager->FireEvent( event );
			}
		}

		for (int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CHL2MP_Player *pSoundEmit = (CHL2MP_Player*) UTIL_PlayerByIndex( i );
			if ( pSoundEmit )
			{
				pSoundEmit->EmitSound("Infection.First.Zombie");
			}
		}

		if ( (g_Teams[TEAM_SURVIVORS]->GetNumPlayers() >= 6) && (g_Teams[TEAM_ZOMBIES]->GetNumPlayers() <= 1) && (!FStrEq( so_gamemode.GetString(), "Overlord" )) )
		{
			// If there are a lot of living humans, spawn another zombie in an attempt to make the current round more interesting.
			// If we're running the Commander gamemode though, ignore this.
			goto FirstInfectionRetry;
		}

		UTIL_ClientPrintAll( HUD_PRINTCENTER, "A zombie infection has been spotted in your area!" );
	}
	else
	{
		// Unable to find anyone for first infection.
		DevMsg("Unable to find a viable infection host. Will continue to retry until host(s) have been found...\n");
		goto FirstInfectionRetry;
	}
}

//Tony; Re-working restart game so that it cleans up safely, and then respawns everyone.
void CHL2MPRules::RestartGame()
{
	// bounds check
	if ( mp_timelimit.GetInt() < 0 )
	{
		mp_timelimit.SetValue( 0 );
	}
	m_flGameStartTime = gpGlobals->curtime;
	if ( !IsFinite( m_flGameStartTime.Get() ) )
	{
		Warning( "Trying to set a NaN game start time\n" );
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	CleanUpMap();
	
	// now that everything is cleaned up, respawn everyone.
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			// If they're in a vehicle, make sure they get out!
			if ( pPlayer->IsInAVehicle() )
				pPlayer->LeaveVehicle();

			QAngle angles = pPlayer->GetLocalAngles();

			angles.x = 0;
			angles.z = 0;

			pPlayer->SetLocalAngles( angles );
		}

		if ( pPlayer->GetActiveWeapon() )
		{
			pPlayer->GetActiveWeapon()->Holster();
		}

		respawn( pPlayer, false );
	}

	CTeam *pRebels = GetGlobalTeam( TEAM_SURVIVORS );
	CTeam *pCombine = GetGlobalTeam( TEAM_ZOMBIES );

	if ( pRebels )
	{
		pRebels->SetScore( 0 );
	}

	if ( pCombine )
	{
		pCombine->SetScore( 0 );
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;		
	m_bCompleteReset = false;

	IGameEvent * event = gameeventmanager->CreateEvent( "round_start" );
	if ( event )
	{
		event->SetInt( "fraglimit", 0 );
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted

		gameeventmanager->FireEvent( event );
	}
}
#endif

// S:O - Moved around #ifndefs and such (actually did this more than once around this area).
void CHL2MPRules::CleanUpMap()
{
#ifndef CLIENT_DLL
	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		CBaseHL2MPCombatWeapon *pWeapon = dynamic_cast< CBaseHL2MPCombatWeapon* >( pCur );
		// Weapons with owners don't want to be removed..
		if ( pWeapon )
		{
			if ( !pWeapon->GetPlayerOwner() )
			{
				UTIL_Remove( pCur );
			}
		}
		// remove entities that has to be restored on roundrestart (breakables etc)
		else if ( !FindInList( s_PreserveEnts, pCur->GetClassname() ) )
		{
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	// Cancel all queued events, in case a func_bomb_target fired some delayed outputs that
	// could kill respawning CTs
	g_EventQueue.Clear();

	// Now reload the map entities.
	class CHL2MPMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( !FindInList( s_PreserveEnts, pClassname ) )
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
					m_iIterator = g_MapEntityRefs.Next( m_iIterator );

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CHL2MPMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// S:O - Do some additional cleaning, just in case.
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if( pPlayer->m_hRagdoll )
		{
			pPlayer->m_hRagdoll->Remove();
			pPlayer->m_hRagdoll = NULL;
		}
	}

	engine->ServerCommand("r_cleardecals\n");

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
#else

	// S:O - More cleaning!
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ClearRagdoll();

		if( pPlayer->m_hRagdoll )
		{
			pPlayer->m_hRagdoll->Remove();
			pPlayer->m_hRagdoll = NULL;
		}
	}

	engine->ClientCmd("r_cleardecals\n");

#endif
}

#ifndef CLIENT_DLL
void CHL2MPRules::CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg )
{
	if( m_bAwaitingReadyRestart && FStrEq( chatmsg, mp_ready_signal.GetString() ) )
	{
		if( !pPlayer->IsReady() )
		{
			pPlayer->SetReady( true );
		}		
	}
}

void CHL2MPRules::CheckRestartGame( void )
{
	// Restart the game if specified by the server
	int iRestartDelay = mp_restartgame.GetInt();

	if ( iRestartDelay > 0 )
	{
		if ( iRestartDelay > 60 )
			iRestartDelay = 60;


		// let the players know
		char strRestartDelay[64];
		Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );

		// S:O - Don't forget to end our current round!
		SetRoundState(RoundEnding, iRestartDelay);

		m_flRestartGameTime = gpGlobals->curtime + iRestartDelay;
		m_bCompleteReset = true;
		mp_restartgame.SetValue( 0 );
	}

	if( mp_readyrestart.GetBool() )
	{
		m_bAwaitingReadyRestart = true;
		m_bHeardAllPlayersReady = false;
		

		const char *pszReadyString = mp_ready_signal.GetString();


		// Don't let them put anything malicious in there
		if( pszReadyString == NULL || Q_strlen(pszReadyString) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "hl2mp_ready_restart" );
		if ( event )
			gameeventmanager->FireEvent( event );

		mp_readyrestart.SetValue( 0 );

		// cancel any restart round in progress
		m_flRestartGameTime = -1;
	}
}

void CHL2MPRules::CheckAllPlayersReady( void )
{
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;
		if ( !pPlayer->IsReady() )
			return;
	}
	m_bHeardAllPlayersReady = true;
}

// S:O - Round Stuff.
void CHL2MPRules::StartRoundtimer(int iDuration)
{
	m_iDuration=iDuration;
	m_fStart=gpGlobals->curtime;
}

// S:O - Reinforcements Stuff.
void CHL2MPRules::StartReinforcementsTimer(int iDuration)
{
	m_iReinforcementsDuration=iDuration;
	m_fReinforcementsStart=gpGlobals->curtime;
}

// S:O - Round Stuff.
int CHL2MPRules::GetRoundtimerRemain2()
{
	return m_fStart<0 ? -1 : m_iDuration-int(gpGlobals->curtime-m_fStart);
}

int CHL2MPRules::GetReinforcementsTimerRemain2()
{
	return m_fReinforcementsStart<0 ? -1 : m_iReinforcementsDuration-int(gpGlobals->curtime-m_fReinforcementsStart);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CHL2MPRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == TRUE )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_Spec";
		}
		else
		{
			const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
			if ( chatLocation && *chatLocation )
			{
				pszFormat = "HL2MP_Chat_Team_Loc";
			}
			else
			{
				pszFormat = "HL2MP_Chat_Team";
			}
		}
	}
	// everyone
	else
	{
		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_All";	
		}
		else
		{
			pszFormat = "HL2MP_Chat_AllSpec";
		}
	}

	return pszFormat;
}

//==================
//AI Patch Addition
//==================

void CHL2MPRules::InitDefaultAIRelationships( void )
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i=0;i<NUM_AI_CLASSES;i++)
	{
		for (j=0;j<NUM_AI_CLASSES;j++)
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

	// S:O - Alter according the mod's new class definitions.
	// ------------------------------------------------------------
	//	> CLASS_ANTLION
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PROTOSNIPER,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ANTLION,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BARNACLE
	//
	//  In this case, the relationship D_HT indicates which characters
	//  the barnacle will try to eat.
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BARNACLE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_EARTH_FAUNA,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ANTLION,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSQUID
	// ------------------------------------------------------------
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HEADCRAB,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HOUNDEYE,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	*/
	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_PASSIVE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_HUNTER,	D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HEADCRAB,			D_FR, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HOUNDEYE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_VORTIGAUNT,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ZOMBIE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_REBEL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_GUNSHIP
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_HUNTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_CONSCRIPT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HACKED_ROLLERMINE,D_NU, 0);
	
	// ------------------------------------------------------------
	//	> CLASS_FLARE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ANTLION,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HEADCRAB
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HACKED_ROLLERMINE,D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_HOUNDEYE
	// ------------------------------------------------------------
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HEADCRAB,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	*/

	// ------------------------------------------------------------
	//	> CLASS_MANHACK
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HEADCRAB,			D_HT,-1);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,		CLASS_HOUNDEYE,			D_HT,-1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_METROPOLICE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MISSILE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ANTLION,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSEYE,			D_NU, 0);	
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSEYE,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_REBEL,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PROTOSNIPER,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HEADCRAB,			D_FR, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ZOMBIE,			D_FR, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PROTOSNIPER,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BARNACLE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_HUNTER,	D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PROTOSNIPER,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_SCANNER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,		CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_HUNTER,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MANHACK,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_METROPOLICE,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MILITARY,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_SCANNER,			D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_STALKER,			D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PROTOSNIPER,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_STALKER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_VORTIGAUNT
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_REBEL,	D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_SCANNER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_VORTIGAUNT,		D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HACKED_ROLLERMINE,D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,		CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HEADCRAB,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MILITARY,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ZOMBIE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_PROTOSNIPER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,		CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_METROPOLICE,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MILITARY,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MISSILE,			D_NU, 5);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_STALKER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_VORTIGAUNT,		D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	//
	// Hates pretty much everything equally except other earth fauna.
	// This will make the critter choose the nearest thing as its enemy.
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_NONE,				D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_FLARE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MANHACK,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MISSILE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_SCANNER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_STALKER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_VORTIGAUNT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PROTOSNIPER,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HACKED_ROLLERMINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CONSCRIPT,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HEADCRAB,			D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_SCANNER,			D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_STALKER,			D_HT, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_VORTIGAUNT,		D_LI, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ZOMBIE,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_EARTH_FAUNA,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HACKED_ROLLERMINE,D_LI, 0);
}
//===== End Of AI Patch ======

// S:O -
// Everything from here on out is all us and fairly self-explanatory.
// Have fun! >=D
void CHL2MPRules::ZombieSound()
{	
	// This doesn't work yet.

	/*CUtlLinkedList< CHL2MP_Player* > possiblePlayers;
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( pPlayer && pPlayer->GetTeamNumber() == 2 && pPlayer->IsAlive() )
		{
				possiblePlayers.AddToTail( pPlayer );
		}

		CHL2MP_Player *pRandomPlayer = NULL;

		if ( possiblePlayers.Count() > 0 )
		{
			int iRandomPicked = random->RandomInt(1,possiblePlayers.Count()-1);
			pRandomPlayer = possiblePlayers[iRandomPicked];

			if (!pRandomPlayer)
				return;

			// Make sure we're still dealing with a valid zombie. If so, SCREAM BITCH!
			if ( pRandomPlayer->GetTeamNumber() == 2 && pRandomPlayer->IsAlive() )
			{
				pRandomPlayer->EmitSound("Zombify");
			}
		}
	}*/
}
// cjd @add - Override the base class's function so we can determine when a player is allowed to respawn
bool CHL2MPRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return false;
}

void CHL2MPRules::HandleRoundState( void )
{
	switch( m_iCurrentState )
	{
		case RoundStarted:
			//DevMsg( "A new round has begun!\n");

			RoundNowEnding = false;
			has_round_ended.SetValue( 0 );

			if ( runoncepl0x )
			{
				zombie_spawned = false;
				did_zombie_spawn.SetValue( 0 );

				if ( FStrEq( so_gamemode.GetString(), "Objective" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Complete your objectives before time runs out!\n" );
				}
				else if ( FStrEq( so_gamemode.GetString(), "Escape" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Escape before time runs out!\n" );
				}
				else if ( FStrEq( so_gamemode.GetString(), "Survival" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Survive the zombie horde.\n" );
				}
				else if ( FStrEq( so_gamemode.GetString(), "Holdout" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Defend the holdout zone!\n" );
				}
				else if ( FStrEq( so_gamemode.GetString(), "Infection" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Survive the infection!\n" );
				}
				else if ( FStrEq( so_gamemode.GetString(), "Overlord" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Survive the wrath of the zombie overlord!\n" );
				}
				else if ( FStrEq( so_gamemode.GetString(), "Extermination" ) )
				{
					hudtextparms_s tTextParam;
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
					tTextParam.channel		= 1;

					UTIL_HudMessageAll( tTextParam, "Wipe out the opposing force!\n" );
				}

				// S:O - Play a nice round start sound to indicate to players that a new round has begun
				for (int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

					if ( pPlayer )
						engine->ClientCommand( pPlayer->edict(), "play events/so-roundbegin.mp3" );
				}

				RoundEndOverlayFired = false;

				// S:O - New "round_end" event implementation
				IGameEvent * event = gameeventmanager->CreateEvent( "round_end" );
				if ( event )
				{
					if (FStrEq(so_gamemode.GetString(), "Infection"))
					{
						event->SetString("objective","Infection");
					}
					else if (FStrEq(so_gamemode.GetString(), "Survival"))
					{
						event->SetString("objective","Survival");
					}
					else if (FStrEq(so_gamemode.GetString(), "Holdout"))
					{
						event->SetString("objective","Holdout");
					}
					else if (FStrEq(so_gamemode.GetString(), "Escape"))
					{
						event->SetString("objective","Escape");
					}
					else if (FStrEq(so_gamemode.GetString(), "VIP"))
					{
						event->SetString("objective","VIP");
					}
					else if (FStrEq(so_gamemode.GetString(), "Hybrid"))
					{
						event->SetString("objective","Hybrid");
					}
					else if ( FStrEq( so_gamemode.GetString(), "Objective" ) )
					{
						event->SetString("objective","Objective");
					}
					else if ( FStrEq( so_gamemode.GetString(), "Overlord" ) )
					{
						event->SetString("objective","Overlord");
					}
					else if ( FStrEq( so_gamemode.GetString(), "Extermination" ) )
					{
						event->SetString("objective","Extermination");
					}
					else
					{
						event->SetString("objective","UNKNOWN");
					}
					
					gameeventmanager->FireEvent( event );
				}

				runoncepl0x = false;
			}
		break;

		case RoundEnding:

			DevMsg( "Round is now ending...\n");
			RoundNowEnding = true;
			has_round_ended.SetValue( 1 );

			runoncepl0x = true;

			// Kill this before we break something.
			zombie_spawned = false;
			did_zombie_spawn.SetValue( 0 );

			// Just in case our round ends didn't catch it, let's set this to true again.
			//RoundEndOverlayFired = true;

			if( m_StateTimer.IsElapsed() )
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

					if ( pPlayer )
					{
						if ( !FStrEq(so_gamemode.GetString(), "Extermination") )
						{
							if ( pPlayer->GetTeamNumber() == 2 )
							{	
								pPlayer->ChangeTeam( 1 );
							}

							if ( pPlayer->GetTeamNumber() == 1 )
							{
								pPlayer->ChangeTeam( 3 );
							}
						}
					}
				}

				//RoundEndOverlayFired = false;
				//RoundNowEnding = false;

				has_round_ended.SetValue( 0 );
				RoundRestart();

				DevMsg( "Restarting round...\n");
			}

			// Just in case our round ends didn't catch it, let's set this to true again.
			RoundEndOverlayFired = true;

		break;
	}
}

int CHL2MPRules::GetAlivePlayers(int iIndex)
{
	// Loop through the players
	CBasePlayer* pClient = NULL;
	int iPlayers = 0;

	for ( int i = 0; i <= gpGlobals->maxClients; i++ )
	{
		pClient = UTIL_PlayerByIndex( i );

		if ( !pClient || !pClient->edict() )
			continue;

		if ( !(pClient->IsNetClient()) )
			continue;

		if( pClient->m_lifeState != LIFE_ALIVE )
			continue;

		if(pClient->GetTeamNumber() == iIndex)
			iPlayers++;
	}

	return iPlayers;
}

int CHL2MPRules::GetAllPlayers(int iIndex)
{
	// Loop through the players
	CBasePlayer* pClient = NULL;
	int iPlayers = 0;

	for ( int i = 0; i <= gpGlobals->maxClients; i++ )
	{
		pClient = UTIL_PlayerByIndex( i );

		if ( !pClient || !pClient->edict() )
			continue;

		if ( !(pClient->IsNetClient()) )
			continue;

		if(!pClient->GetTeamNumber() == 1)
			continue;

		if(pClient->GetTeamNumber() == iIndex)
			iPlayers++;
	}

	return iPlayers;
}

/*// S:O - Slow Motion Effect
void CHL2MPRules::ActivateSlowMotion( void )
{
	// Only do our little slow motion effect if we're not in slow motion already.
	// Also, to avoid slow motion spam, check our timer.
	if ( !SlowMotion && m_SlowMotionAntiSpamTimer.IsElapsed() )
	{
		SlowMotion = true;
		m_fSlowMotionTimer = gpGlobals->curtime;
	}
}

// S:O - Slow Motion Effect
void CHL2MPRules::DeactivateSlowMotion( void )
{
	SlowMotion = false;
	m_fSlowMotionTimer = gpGlobals->curtime;
}*/

// S:O - Recalculate Dynamic Difficulty
void CHL2MPRules::RecalcDifficulty( void )
{
	DevMsg("Recalculating dynamic difficulty for approximately %i zombies...\n", gEntList.m_ZombieList.Count());

	// Reset our kills value before calculating our newest one.
	iKills = 0;

	// Get the Survivor team's score (total kills).
	iKills = GetGlobalTeam( 3 )->GetScore();

	DevMsg("...the Survivors have killed approximately %i zombie NPC(s), adjusting zombie NPC health according to this value...\n", iKills);

	// Make the appropriate difficulty changes depending on the value of our calculated kill count.
	if ( iKills >= 250 )
	{
		szReaperHealthFloat = ( so_reaper_health.GetFloat() * 3.5f );
		szReaperNoJumpHealthFloat = ( so_reaper_nojump_health.GetFloat() * 3.5f );
		szSeekerHealthFloat = ( so_seeker_health.GetFloat() * 3.5f );
		szCreeperHealthFloat = ( so_creeper_health.GetFloat() * 3.5f );
		szSploderHealthFloat = ( so_sploder_health.GetFloat() * 3.5f );

		Q_snprintf( szReaperHealth, sizeof(szReaperHealth), "%f", szReaperHealthFloat );
		Q_snprintf( szReaperNoJumpHealth, sizeof(szReaperNoJumpHealth), "%f", szReaperNoJumpHealthFloat );
		Q_snprintf( szSeekerHealth, sizeof(szSeekerHealth), "%f", szSeekerHealthFloat );
		Q_snprintf( szCreeperHealth, sizeof(szCreeperHealth), "%f", szCreeperHealthFloat );
		Q_snprintf( szSploderHealth, sizeof(szSploderHealth), "%f", szSploderHealthFloat );

		so_reaper_health_dd.SetValue( szReaperHealth );
		so_reaper_nojump_health_dd.SetValue( szReaperNoJumpHealth );
		so_seeker_health_dd.SetValue( szSeekerHealth );
		so_creeper_health_dd.SetValue( szCreeperHealth );
		so_sploder_health_dd.SetValue( szSploderHealth );

		// S:O - The Survivors have made a lot of kills, so let's make things interesting and spawn a boss! >=D
		if ( FStrEq(so_gamemode.GetString(), "Survival") || FStrEq(so_gamemode.GetString(), "Holdout") || FStrEq(so_gamemode.GetString(), "Objective") )
		{
			if ( (so_boss_system.GetInt() != 0) && (GetRoundtimerRemain2() <= bossspawn_timer) && (!m_bBossSpawned) )
			{
				SpawnBoss();

				// S:O - After we've spawned one this round, don't spawn another until the next round.
				m_bBossSpawned = true;
			}
		}

		so_dyndiff_level.SetValue( "5" );
	}
	else if ( iKills >= 200 )
	{
		szReaperHealthFloat = ( so_reaper_health.GetFloat() * 3.0f );
		szReaperNoJumpHealthFloat = ( so_reaper_nojump_health.GetFloat() * 3.0f );
		szSeekerHealthFloat = ( so_seeker_health.GetFloat() * 3.0f );
		szCreeperHealthFloat = ( so_creeper_health.GetFloat() * 3.0f );
		szSploderHealthFloat = ( so_sploder_health.GetFloat() * 3.0f );

		Q_snprintf( szReaperHealth, sizeof(szReaperHealth), "%f", szReaperHealthFloat );
		Q_snprintf( szReaperNoJumpHealth, sizeof(szReaperNoJumpHealth), "%f", szReaperNoJumpHealthFloat );
		Q_snprintf( szSeekerHealth, sizeof(szSeekerHealth), "%f", szSeekerHealthFloat );
		Q_snprintf( szCreeperHealth, sizeof(szCreeperHealth), "%f", szCreeperHealthFloat );
		Q_snprintf( szSploderHealth, sizeof(szSploderHealth), "%f", szSploderHealthFloat );

		so_reaper_health_dd.SetValue( szReaperHealth );
		so_reaper_nojump_health_dd.SetValue( szReaperNoJumpHealth );
		so_seeker_health_dd.SetValue( szSeekerHealth );
		so_creeper_health_dd.SetValue( szCreeperHealth );
		so_sploder_health_dd.SetValue( szSploderHealth );

		so_dyndiff_level.SetValue( "4" );
	}
	else if ( iKills >= 150 )
	{
		szReaperHealthFloat = ( so_reaper_health.GetFloat() * 2.5f );
		szReaperNoJumpHealthFloat = ( so_reaper_nojump_health.GetFloat() * 2.5f );
		szSeekerHealthFloat = ( so_seeker_health.GetFloat() * 2.5f );
		szCreeperHealthFloat = ( so_creeper_health.GetFloat() * 2.5f );
		szSploderHealthFloat = ( so_sploder_health.GetFloat() * 2.5f );

		Q_snprintf( szReaperHealth, sizeof(szReaperHealth), "%f", szReaperHealthFloat );
		Q_snprintf( szReaperNoJumpHealth, sizeof(szReaperNoJumpHealth), "%f", szReaperNoJumpHealthFloat );
		Q_snprintf( szSeekerHealth, sizeof(szSeekerHealth), "%f", szSeekerHealthFloat );
		Q_snprintf( szCreeperHealth, sizeof(szCreeperHealth), "%f", szCreeperHealthFloat );
		Q_snprintf( szSploderHealth, sizeof(szSploderHealth), "%f", szSploderHealthFloat );

		so_reaper_health_dd.SetValue( szReaperHealth );
		so_reaper_nojump_health_dd.SetValue( szReaperNoJumpHealth );
		so_seeker_health_dd.SetValue( szSeekerHealth );
		so_creeper_health_dd.SetValue( szCreeperHealth );
		so_sploder_health_dd.SetValue( szSploderHealth );

		so_dyndiff_level.SetValue( "3" );
	}
	else if ( iKills >= 100 )
	{
		szReaperHealthFloat = ( so_reaper_health.GetFloat() * 2.0f );
		szReaperNoJumpHealthFloat = ( so_reaper_nojump_health.GetFloat() * 2.0f );
		szSeekerHealthFloat = ( so_seeker_health.GetFloat() * 2.0f );
		szCreeperHealthFloat = ( so_creeper_health.GetFloat() * 2.0f );
		szSploderHealthFloat = ( so_sploder_health.GetFloat() * 2.0f );

		Q_snprintf( szReaperHealth, sizeof(szReaperHealth), "%f", szReaperHealthFloat );
		Q_snprintf( szReaperNoJumpHealth, sizeof(szReaperNoJumpHealth), "%f", szReaperNoJumpHealthFloat );
		Q_snprintf( szSeekerHealth, sizeof(szSeekerHealth), "%f", szSeekerHealthFloat );
		Q_snprintf( szCreeperHealth, sizeof(szCreeperHealth), "%f", szCreeperHealthFloat );
		Q_snprintf( szSploderHealth, sizeof(szSploderHealth), "%f", szSploderHealthFloat );

		so_reaper_health_dd.SetValue( szReaperHealth );
		so_reaper_nojump_health_dd.SetValue( szReaperNoJumpHealth );
		so_seeker_health_dd.SetValue( szSeekerHealth );
		so_creeper_health_dd.SetValue( szCreeperHealth );
		so_sploder_health_dd.SetValue( szSploderHealth );

		so_dyndiff_level.SetValue( "2" );
	}
	else if ( iKills >= 50 )
	{
		szReaperHealthFloat = ( so_reaper_health.GetFloat() * 1.5f );
		szReaperNoJumpHealthFloat = ( so_reaper_nojump_health.GetFloat() * 1.5f );
		szSeekerHealthFloat = ( so_seeker_health.GetFloat() * 1.5f );
		szCreeperHealthFloat = ( so_creeper_health.GetFloat() * 1.5f );
		szSploderHealthFloat = ( so_sploder_health.GetFloat() * 1.5f );

		Q_snprintf( szReaperHealth, sizeof(szReaperHealth), "%f", szReaperHealthFloat );
		Q_snprintf( szReaperNoJumpHealth, sizeof(szReaperNoJumpHealth), "%f", szReaperNoJumpHealthFloat );
		Q_snprintf( szSeekerHealth, sizeof(szSeekerHealth), "%f", szSeekerHealthFloat );
		Q_snprintf( szCreeperHealth, sizeof(szCreeperHealth), "%f", szCreeperHealthFloat );
		Q_snprintf( szSploderHealth, sizeof(szSploderHealth), "%f", szSploderHealthFloat );

		so_reaper_health_dd.SetValue( szReaperHealth );
		so_reaper_nojump_health_dd.SetValue( szReaperNoJumpHealth );
		so_seeker_health_dd.SetValue( szSeekerHealth );
		so_creeper_health_dd.SetValue( szCreeperHealth );
		so_sploder_health_dd.SetValue( szSploderHealth );

		so_dyndiff_level.SetValue( "1" );
	}
	else if ( iKills < 50 )
	{
		szReaperHealthFloat = ( so_reaper_health.GetFloat() );
		szReaperNoJumpHealthFloat = ( so_reaper_nojump_health.GetFloat() );
		szSeekerHealthFloat = ( so_seeker_health.GetFloat() );
		szCreeperHealthFloat = ( so_creeper_health.GetFloat() );
		szSploderHealthFloat = ( so_sploder_health.GetFloat() );

		Q_snprintf( szReaperHealth, sizeof(szReaperHealth), "%f", szReaperHealthFloat );
		Q_snprintf( szReaperNoJumpHealth, sizeof(szReaperNoJumpHealth), "%f", szReaperNoJumpHealthFloat );
		Q_snprintf( szSeekerHealth, sizeof(szSeekerHealth), "%f", szSeekerHealthFloat );
		Q_snprintf( szCreeperHealth, sizeof(szCreeperHealth), "%f", szCreeperHealthFloat );
		Q_snprintf( szSploderHealth, sizeof(szSploderHealth), "%f", szSploderHealthFloat );

		so_reaper_health_dd.SetValue( szReaperHealth );
		so_reaper_nojump_health_dd.SetValue( szReaperNoJumpHealth );
		so_seeker_health_dd.SetValue( szSeekerHealth );
		so_creeper_health_dd.SetValue( szCreeperHealth );
		so_sploder_health_dd.SetValue( szSploderHealth );

		so_dyndiff_level.SetValue( "0" );
	}

	DevMsg("...dynamic difficulty successfully recalculated at level %i!\n", so_dyndiff_level.GetInt());
}
#endif

#ifndef CLIENT_DLL
int CHL2MPRules::CalculateSurvivorCount( void )
{
	return (g_Teams[TEAM_SURVIVORS]->GetNumPlayers() + gEntList.m_SurvivorList.Count());
}

int CHL2MPRules::CalculateZombieCount( void )
{
	return (g_Teams[TEAM_ZOMBIES]->GetNumPlayers() + gEntList.m_ZombieList.Count());
}

int CHL2MPRules::CalculateMilitaryCount( void )
{
	return (g_Teams[TEAM_MILITARY]->GetNumPlayers() + gEntList.m_MilitaryList.Count());
}
#endif

#ifdef CLIENT_DLL
int CHL2MPRules::GetRoundtimerRemain() {
  float timer = m_fStart<0 ? -1 : m_iDuration-int(gpGlobals->curtime-m_fStart);

  if( timer < 0 )
	  return 0.0f;

  return timer;
}

int CHL2MPRules::GetReinforcementsTimerRemain() {
  float retimer = m_fReinforcementsStart<0 ? -1 : m_iReinforcementsDuration-int(gpGlobals->curtime-m_fReinforcementsStart);

  if( retimer < 0 )
	  return 0.0f;

  return retimer;
}
#endif