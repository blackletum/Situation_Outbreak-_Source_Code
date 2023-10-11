//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h" // Modification: Moved to top due to compiler errors.
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "hl2mp_player_shared.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "hl2mp_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"
#include "GameStats.h"
#include "tier0/vprof.h"
#include "bone_setup.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "obstacle_pushaway.h"

#include "ilagcompensationmanager.h"

// S:O - Additional Includes
#include "zms/weapon_zombie.h"
#include "lua/zms_luagameplay.h"
#include "lua/zms_lua.h"
#include "lua/ge_luamanager.h"
#include "so_viewmodel.h"
#include "gib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_iLastCitizenModel = 0;
int g_iLastCombineModel = 0;
int g_iLastMilitaryModel = 0;

// S:O - See spawn point function in this .cpp for details...
CBaseEntity	 *g_pLastPlayerSpawn = NULL;
//CBaseEntity	 *g_pLastCombineSpawn = NULL;
//CBaseEntity	 *g_pLastRebelSpawn = NULL;

extern CBaseEntity				*g_pLastSpawn;

#define HL2MP_COMMAND_MAX_RATE 0.3

// S:O - ConVar Declarations
ConVar so_spawnmoney( "so_spawnmoney", "5000", FCVAR_GAMEDLL | FCVAR_NOTIFY, "The minimum amount of points a player starts with when they spawn");
//ConVar so_zombie_knockback( "so_zombie_knockback", "0.5", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Multiplier for the amount of zombie knockback");
ConVar so_zombie_healthregen( "so_zombie_healthregen", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Enables/disables health regeneration for zombies");
ConVar so_human_healthregen( "so_human_healthregen", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Enables/disables health regeneration for humans");
ConVar so_healthregen_speed( "so_healthregen_speed", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Multiplier for the speed of health regen" );
ConVar so_zombie_speed( "so_zombie_speed", "200", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines how fast zombies move" );

extern ConVar so_zombie_health;
extern ConVar so_gamemode;
extern ConVar cl_playermodel;
extern ConVar so_respawn;
extern ConVar did_zombie_spawn;
extern ConVar so_zombie_speed;
extern ConVar so_dyndiff_level;
extern ConVar so_infection_system;
extern ConVar so_infection_chance;

void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade );

LINK_ENTITY_TO_CLASS( player, CHL2MP_Player );

LINK_ENTITY_TO_CLASS( info_player_combine, CPointEntity );
LINK_ENTITY_TO_CLASS( info_player_rebel, CPointEntity );

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CHL2MP_Player, DT_HL2MPLocalPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
//	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CHL2MP_Player, DT_HL2MPNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CHL2MP_Player, DT_HL2MP_Player)
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "hl2mplocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2MPLocalPlayerExclusive), SendProxy_SendLocalDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "hl2mpnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2MPNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	// S:O - Leveling System
	SendPropInt( SENDINFO( m_iExp ) ),
	SendPropInt( SENDINFO( m_iLevel ) ),

	// S:O - Infection system
	SendPropInt( SENDINFO( m_bIsInfected ) ),

	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropBool( SENDINFO( m_bSpawnInterpCounter) ),
	SendPropInt( SENDINFO( m_iPlayerSoundType), 3 ),
		
END_SEND_TABLE()

BEGIN_DATADESC( CHL2MP_Player )
END_DATADESC()

// S:O - Modified Player Model List
const char *g_ppszRandomCitizenModels[] = 
{
	// SURVIVOR MODELS
	"models/player/Humans/survivor1.mdl",
	"models/player/Humans/survivor2.mdl",
	"models/player/Humans/survivor3.mdl",
	"models/player/Humans/survivor4.mdl",
	"models/player/Humans/survivor5.mdl",
	"models/player/Humans/survivor6.mdl",
	"models/player/Humans/survivor7.mdl",
	"models/player/Humans/survivor8.mdl",
	"models/player/Humans/survivor9.mdl",
	"models/player/Humans/survivor10.mdl",
	"models/player/Humans/survivor11.mdl",
	"models/player/Humans/survivor12.mdl",
	"models/player/Humans/survivor13.mdl",
	"models/player/Humans/survivor14.mdl",
	"models/player/Humans/survivor15.mdl",
	"models/player/Humans/survivor16.mdl",
	"models/player/Humans/survivor17.mdl",
	"models/player/Humans/survivor18.mdl",
	"models/player/Humans/survivor19.mdl",
	"models/player/Humans/survivor20.mdl",
	"models/player/Humans/survivor21.mdl",
	"models/player/Humans/survivor22.mdl",
	"models/player/Humans/survivor23.mdl",
	"models/player/Humans/survivor24.mdl",
	"models/player/Humans/survivor25.mdl",
	"models/player/Humans/survivor26.mdl",
	"models/player/Humans/survivor27.mdl",
};

const char *g_ppszRandomCombineModels[] =
{
	// ZOMBIE OVERLORD MODELS
	"models/player/Zombies/zombie1.mdl",
	"models/player/Zombies/zombie2.mdl",
	"models/player/Zombies/zombie3.mdl",
	// ZOMBIE CLASS MODELS
	"models/player/Zombies/charple.mdl",
	"models/player/Zombies/corpse.mdl",
	"models/player/Zombies/stalker.mdl",
};

const char *g_ppszRandomMilitaryModels[] =
{
	// MILITARY SOLDIER MODELS
	"models/player/Military/soldier.mdl",
};

#define MAX_COMBINE_MODELS 6
#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CHL2MP_Player::CHL2MP_Player()
{
	//Tony; create our player animation state.
	m_PlayerAnimState = CreateHL2MPPlayerAnimState( this );
	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	m_bSpawnInterpCounter = false;

    m_bEnterObserver = false;
	m_bReady = false;

	BaseClass::ChangeTeam( 0 );

	// S:O - Misc.
	m_flCheckLevelUp = 0.0f;
	m_flHealthRegenTime = 0.0f;
	m_flGiveCommanderCash = 0.0f;
	m_fBleedingDelay = 0.0f;
	m_fSpeedChangeDelay = 0.0f;

	NeedsToBeSwitchedToReinforcements = false;

	// S:O - Leveling System
	m_iExp = 0;
	m_iLevel = 1;
	LevelUp();

	// S:O - Infection system
	m_bIsInfected = false;

	// S:O - Extermination team balancing
	m_iNumSurvivors = 0;
	m_iNumMilitary = 0;
}

CHL2MP_Player::~CHL2MP_Player( void )
{
	m_PlayerAnimState->Release();
}

void CHL2MP_Player::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
}

void CHL2MP_Player::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel ( "sprites/glow01.vmt" );
	PrecacheParticleSystem( "rainsplash" );

	//Precache Citizen models
	int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCitizenModels[i] );

	//Precache Combine Models
	nHeads = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCombineModels[i] );

	//Precache Military Models
	nHeads = ARRAYSIZE( g_ppszRandomMilitaryModels );

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomMilitaryModels[i] );

	PrecacheFootStepSounds();

	// ZMS - Precache Sounds and NPCs
	PrecacheScriptSound( "Male.Die" );
	PrecacheScriptSound( "Male.Pain" );
	PrecacheScriptSound( "Female.Die" );
	PrecacheScriptSound( "Female.Pain" );
	PrecacheScriptSound( "Zombie.Pain" );
	PrecacheScriptSound( "Zombie.Death" );
	PrecacheScriptSound( "Military.Pain" );
	PrecacheScriptSound( "Military.Death" );
	PrecacheScriptSound( "Reload.Male" );
	PrecacheScriptSound( "Reload.Female" );
	PrecacheScriptSound( "Infected" );
	PrecacheScriptSound( "Zombify" );
	PrecacheScriptSound( "Nightvision.On" );
	PrecacheScriptSound( "Nightvision.Off" );

	// FIXME: Find another place for this!
	// We need to precache these to stop console spam caused by entities that emit sparks and their sounds.
	// This is just a hack until we figure out where exactly the problem is in the code.
	PrecacheScriptSound( "DoSpark" );
	PrecacheScriptSound( "LoudSpark" );
	PrecacheScriptSound( "ReallyLoudSpark" );
	PrecacheScriptSound( "ambient.electrical_zap_1" );
	PrecacheScriptSound( "ambient.electrical_zap_2" );
	PrecacheScriptSound( "ambient.electrical_zap_3" );
	PrecacheScriptSound( "ambient.electrical_zap_4" );
	PrecacheScriptSound( "ambient.electrical_zap_5" );
	PrecacheScriptSound( "ambient.electrical_zap_6" );
	PrecacheScriptSound( "ambient.electrical_zap_7" );
	PrecacheScriptSound( "ambient.electrical_zap_8" );
	PrecacheScriptSound( "ambient.electrical_zap_9" );
	PrecacheScriptSound( "ambient.electrical_random_zap_1" );
	PrecacheScriptSound( "ambient.electrical_random_zap_2" );

	// FIXME: Find another place for this!
	// (Used in weapon_crowbar.cpp for our secondary shove attack.)
	PrecacheScriptSound( "NPC_Metropolice.Shove" );

	// FIXME: Find another place for this!
	// (Precache our NPCs!)
	UTIL_PrecacheOther( "npc_zombie" );
	UTIL_PrecacheOther( "npc_fastzombie" );
	UTIL_PrecacheOther( "npc_reaper_nojump" );
	UTIL_PrecacheOther( "npc_poisonzombie" );
	UTIL_PrecacheOther( "npc_sploderzombie" );
	UTIL_PrecacheOther( "npc_headcrab" );
	UTIL_PrecacheOther( "npc_headcrab_fast" );
	UTIL_PrecacheOther( "npc_headcrab_poison" );
	UTIL_PrecacheOther( "npc_turret_floor" );
	UTIL_PrecacheOther( "npc_citizen" );
	UTIL_PrecacheOther( "npc_combine_s" );
	UTIL_PrecacheOther( "npc_cloud" );
	UTIL_PrecacheOther( "npc_ghost" );

	// This is fine where it is, I think.
	// Used in hud_health.cpp as a warning that the player's health is low.
	// NOTE: This isn't used yet, so there's no point in precaching it at this time.
	// PrecacheScriptSound( "LowHealth.Warning" );

	// S:O - Experimental simple gib system
	/*PrecacheModel( "models/gibs/hgibs_rib.mdl" );
	PrecacheModel( "models/gibs/hgibs_scapula.mdl" );
	PrecacheModel( "models/gibs/hgibs_spine.mdl" );*/
}

void CHL2MP_Player::GiveAllItems( void )
{
	// S:O - Modified Weapon and Ammo List
	EquipSuit();

	CBasePlayer::GiveAmmo( 99,	"9mm");
	CBasePlayer::GiveAmmo( 10,	"AR2" );
	CBasePlayer::GiveAmmo( 10,	"AK47" );
	CBasePlayer::GiveAmmo( 10,	"MP5K");
	CBasePlayer::GiveAmmo( 10,	"MAC10");
	CBasePlayer::GiveAmmo( 3,	"smg1_grenade");
	CBasePlayer::GiveAmmo( 3,	"AR2_Grenade");
	CBasePlayer::GiveAmmo( 50,	"Buckshot");
	CBasePlayer::GiveAmmo( 50,	"Buckshot2");
	CBasePlayer::GiveAmmo( 50,	"Buckshot3");
	CBasePlayer::GiveAmmo( 10,	"Sniper");
	CBasePlayer::GiveAmmo( 10,	"Deagle" );
	CBasePlayer::GiveAmmo( 3,	"rpg_round");
	CBasePlayer::GiveAmmo( 3,	"grenade" );
	CBasePlayer::GiveAmmo( 3,	"IncendiaryGrenade" );
	CBasePlayer::GiveAmmo( 3,	"Zombait" );

	GiveNamedItem( "weapon_9mm" );
	GiveNamedItem( "weapon_deagle" );
	GiveNamedItem( "weapon_mp5k" );
	GiveNamedItem( "weapon_mac10" );
	GiveNamedItem( "weapon_m4" );
	GiveNamedItem( "weapon_doublebarrel" );
	GiveNamedItem( "weapon_frag" );
	GiveNamedItem( "weapon_incendiary" );
	GiveNamedItem( "weapon_sniper" );
	GiveNamedItem( "weapon_rpg" );
	GiveNamedItem( "weapon_ak47" );
}

// S:O - Player Classification
// This helps NPCs determine whether the player is friendly or not.
Class_T CHL2MP_Player::Classify ( void )
{
	if ( GetTeamNumber() == 1 )
	{
		return CLASS_NONE;
	}
	else if ( GetTeamNumber() == 2 )
	{
		return CLASS_ZOMBIE;
	}
	else if ( GetTeamNumber() == 3 )
	{
		return CLASS_PLAYER;
	}
	else if ( GetTeamNumber() == 4 )
	{
		return CLASS_COMBINE;
	}
	else
	{
		return CLASS_NONE;
	}
}

// S:O - Leveling System
#define XP_FOR_LEVEL_2 50
#define XP_FOR_LEVEL_3 250
#define XP_FOR_LEVEL_4 500
#define XP_FOR_LEVEL_5 1000

// S:O - Leveling System
void CHL2MP_Player::CheckLevel()
{
	bool bShouldLevel = false;
	if ( GetLevel() == 1 )
	{
		if ( GetXP() >= XP_FOR_LEVEL_2 )
			bShouldLevel = true;
	}
	else if ( GetLevel() == 2 )
	{
		if ( GetXP() >= XP_FOR_LEVEL_3 )
			bShouldLevel = true;
	}
	else if ( GetLevel() == 3 )
	{
		if ( GetXP() >= XP_FOR_LEVEL_4 )
			bShouldLevel = true;
	}
	else if ( GetLevel() == 4 )
	{
		if ( GetXP() >= XP_FOR_LEVEL_5 )
			bShouldLevel = true;
	}

	if ( bShouldLevel )
	{
		m_iLevel ++;
		ClientPrint( this, HUD_PRINTTALK, UTIL_VarArgs( "Level up! You are now at level %i.\n", GetLevel() ) );
		LevelUp();
	}
}

// S:O - Leveling System
void CHL2MP_Player::LevelUp()
{
	switch ( GetLevel() )
	{
		case 1:
			// Defined in Spawn(), etc.
			// All players start at level 1 by default, so there's really no point in telling them they have no power-ups!
		break;

		case 2:
			m_iMaxHealth = m_iMaxHealth + 25;
			m_iHealth = m_iMaxHealth;
			ClientPrint( this, HUD_PRINTTALK, UTIL_VarArgs( " +25 Health [NEW HP: %i] / Health Restored\n", GetMaxHealth() ) );
			ClientPrint( this, HUD_PRINTTALK, " +1000 Starting Points\n" );
		break;

		case 3:
			m_iMaxHealth = m_iMaxHealth + 25;
			m_iHealth = m_iMaxHealth;
			ClientPrint( this, HUD_PRINTTALK, UTIL_VarArgs( " +25 Health [NEW HP: %i] / Health Restored\n", GetMaxHealth() ) );
			ClientPrint( this, HUD_PRINTTALK, " +1000 Starting Points\n" );
		break;

		case 4:
			m_iMaxHealth = m_iMaxHealth + 25;
			m_iHealth = m_iMaxHealth;
			ClientPrint( this, HUD_PRINTTALK, UTIL_VarArgs( " +25 Health [NEW HP: %i] / Health Restored\n", GetMaxHealth() ) );
			ClientPrint( this, HUD_PRINTTALK, " +1000 Starting Points\n" );
		break;

		case 5:
			m_iMaxHealth = m_iMaxHealth + 25;
			m_iHealth = m_iMaxHealth;
			ClientPrint( this, HUD_PRINTTALK, UTIL_VarArgs( " +25 Health [NEW HP: %i] / Health Restored\n", GetMaxHealth() ) );
			ClientPrint( this, HUD_PRINTTALK, " +1000 Starting Points\n" );
		break;
	}
}

// S:O - Leveling System
// This is called when a player spawns to reapply any power-ups the player might have gotten from leveling up in previous rounds.
// Note that all of these changes are silent, since the player expects them to still be in effect anyway.
void CHL2MP_Player::ReapplyLevelPowerups()
{
	switch ( GetLevel() )
	{
		case 1:
			// Defined in Spawn(), etc.
		break;

		case 2:
			m_iMaxHealth = m_iMaxHealth + 25;
			m_iHealth = m_iMaxHealth;
		break;

		case 3:
			m_iMaxHealth = m_iMaxHealth + 50;
			m_iHealth = m_iMaxHealth;
		break;

		case 4:
			m_iMaxHealth = m_iMaxHealth + 75;
			m_iHealth = m_iMaxHealth;
		break;

		case 5:
			m_iMaxHealth = m_iMaxHealth + 100;
			m_iHealth = m_iMaxHealth;
		break;
	}
}

// S:O - Modified Default Weapons and Ammo
void CHL2MP_Player::GiveDefaultItems( void )
{
	if ( GetTeamNumber() == 1 )
	{
		RemoveAllWeapons();
	}
	else if ( GetTeamNumber() == 2 )
	{
		if ( !IsInfected() )
		{
			EquipSuit();
			RemoveAllWeapons();
		}

		// S:O - Class-specific claws are given out in TeamConfig()
	}
	else if ( GetTeamNumber() == 3 )
	{
		EquipSuit();

		CBasePlayer::GiveAmmo( 99,	"9mm");
		CBasePlayer::GiveAmmo( 10,	"AR2" );
		CBasePlayer::GiveAmmo( 10,	"AK47" );
		CBasePlayer::GiveAmmo( 10,	"MP5K");
		CBasePlayer::GiveAmmo( 10,	"MAC10");
		CBasePlayer::GiveAmmo( 3,	"smg1_grenade");
		CBasePlayer::GiveAmmo( 3,	"AR2_Grenade");
		CBasePlayer::GiveAmmo( 50,	"Buckshot");
		CBasePlayer::GiveAmmo( 50,	"Buckshot2");
		CBasePlayer::GiveAmmo( 50,	"Buckshot3");
		CBasePlayer::GiveAmmo( 10,	"Deagle" );
		CBasePlayer::GiveAmmo( 3,	"rpg_round");
		CBasePlayer::GiveAmmo( 3,	"grenade" );
		CBasePlayer::GiveAmmo( 3,	"IncendiaryGrenade" );
		CBasePlayer::GiveAmmo( 3,	"Zombait" );
		CBasePlayer::GiveAmmo( 10,	"Sniper");

		// If we don't have any brass knuckles on us, give us a pair.
		if ( !Weapon_OwnsThisType("weapon_brassknuckles") )
			GiveNamedItem( "weapon_brassknuckles" );

		// If we don't have another type of pistol (EX: a deagle), then we get an 9mm as well!
		if ( !Weapon_OwnsThisType("weapon_deagle") )
			GiveNamedItem( "weapon_9mm" );
	}
	else if ( GetTeamNumber() == 4 )
	{
		EquipSuit();

		CBasePlayer::GiveAmmo( 99,	"9mm");
		CBasePlayer::GiveAmmo( 10,	"AR2" );
		CBasePlayer::GiveAmmo( 10,	"AK47" );
		CBasePlayer::GiveAmmo( 10,	"MP5K");
		CBasePlayer::GiveAmmo( 10,	"MAC10");
		CBasePlayer::GiveAmmo( 3,	"smg1_grenade");
		CBasePlayer::GiveAmmo( 3,	"AR2_Grenade");
		CBasePlayer::GiveAmmo( 50,	"Buckshot");
		CBasePlayer::GiveAmmo( 50,	"Buckshot2");
		CBasePlayer::GiveAmmo( 50,	"Buckshot3");
		CBasePlayer::GiveAmmo( 10,	"Deagle" );
		CBasePlayer::GiveAmmo( 3,	"rpg_round");
		CBasePlayer::GiveAmmo( 3,	"grenade" );
		CBasePlayer::GiveAmmo( 3,	"IncendiaryGrenade" );
		CBasePlayer::GiveAmmo( 3,	"Zombait" );
		CBasePlayer::GiveAmmo( 10,	"Sniper");

		// TODO: Give military guys some cool shit that makes them look cooler than the survivors when they spawn

		// If we don't have any brass knuckles on us, give us a pair.
		if ( !Weapon_OwnsThisType("weapon_brassknuckles") )
			GiveNamedItem( "weapon_brassknuckles" );

		// If we don't have another type of pistol (EX: a deagle), then we get an 9mm as well!
		if ( !Weapon_OwnsThisType("weapon_deagle") )
			GiveNamedItem( "weapon_9mm" );
	}
}

// S:O - Modified Default Spawn Teams
void CHL2MP_Player::PickDefaultSpawnTeam( void )
{
	if ( (m_iLateJoiner == 2) && (HL2MPRules()->PlayersAreConnected) && (!g_fGameOver) && (!HL2MPRules()->RoundNowEnding) && (!HL2MPRules()->RoundEndOverlayFired) && (HL2MPRules()->GetAlivePlayers(2) + HL2MPRules()->GetAlivePlayers(3) + HL2MPRules()->GetAlivePlayers(4) != 0) )
	{
		// If we're not a late joiner, wait to be respawned
		// This will either happen during a round or at the end of a round depending on the value of so_respawn
		ChangeTeam( TEAM_SPECTATOR );
	}
	// S:O - Initialize the Extermination gamemode's team balancing system
	else if ( FStrEq(so_gamemode.GetString(), "Extermination") )
	{
		// Force us to unassigned while we find a team to join
		// this solves a whole lot of issues, trust me
		ChangeTeam( TEAM_UNASSIGNED );

		// Update our variables for the number of players on each team
		m_iNumSurvivors = g_Teams[TEAM_SURVIVORS]->GetNumPlayers();
		m_iNumMilitary = g_Teams[TEAM_MILITARY]->GetNumPlayers();

		// Join the team with the least amount of players on it
		if ( m_iNumMilitary > m_iNumSurvivors )
		{
			ChangeTeam( TEAM_SURVIVORS );
		}
		else if ( m_iNumMilitary < m_iNumSurvivors )
		{
			ChangeTeam( TEAM_MILITARY );
		}
		else
		{
			// Teams are even!
			// this is a bit of a hack, but it seems to work
			// the downside is that the survivors will always have more people if there's an odd number of players
			// not the end of the world seeing as the military is a bit more powerful anyway, in my opinion
			ChangeTeam( TEAM_SURVIVORS );
		}
	}
	else if ( FStrEq(so_gamemode.GetString(), "Infection") || FStrEq(so_gamemode.GetString(), "Escape") )
	{
		if ( did_zombie_spawn.GetInt() == 1 )
		{
			ChangeTeam( TEAM_ZOMBIES );
		}
		else
		{
			// If a zombie hasn't spawned yet, then we're clear to spawn as a survivor.
			ChangeTeam( TEAM_SURVIVORS );
		}
	}
	else if ( FStrEq(so_gamemode.GetString(), "Survival") || FStrEq(so_gamemode.GetString(), "Holdout") || FStrEq( so_gamemode.GetString(), "Objective" ) )
	{
		ChangeTeam( TEAM_SURVIVORS );
	}
	else if ( FStrEq(so_gamemode.GetString(), "Overlord") )
	{
		// Code currently changes the chosen zombie overlord's team and doesn't tell them to spawn
		// That's why we're able to respawn everyone as a Survivor here
		// If this ever changes though, this will probably have to be adjusted
		ChangeTeam( TEAM_SURVIVORS );

		/*if ( did_zombie_spawn.GetInt() == 1 )
		{
			ChangeTeam( TEAM_SURVIVORS );
		}
		else
		{
			// If a zombie hasn't spawned yet, then we're clear to spawn as a survivor.
			ChangeTeam( TEAM_SURVIVORS );
		}*/
	}
	else
	{
		// Not sure how we got here, but whatever.
		// Spawn as a Survivor just so we don't fuck anything up.
		ChangeTeam( TEAM_SURVIVORS );
	}

	//m_iConnectSpawn = false;
}

#define HL2MP_PUSHAWAY_THINK_CONTEXT	"HL2MPPushawayThink"
void CHL2MP_Player::HL2MPPushawayThink(void)
{
	// Push physics props out of our way.
	PerformObstaclePushaway( this );
	SetNextThink( gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, HL2MP_PUSHAWAY_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2MP_Player::Spawn(void)
{
	// S:O - LUA
	ZMSLuaGamePlay *g = GetGP();

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	PickDefaultSpawnTeam();

	BaseClass::Spawn();
	
	if ( !IsObserver() )
	{
		pl.deadflag = false;
		RemoveSolidFlags( FSOLID_NOT_SOLID );

		RemoveEffects( EF_NODRAW );
	}

	RemoveEffects( EF_NOINTERP );

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

	m_impactEnergyScale = HL2MPPLAYER_PHYSDAMAGE_SCALE;

	if ( HL2MPRules()->IsIntermission() )
	{
		AddFlag( FL_FROZEN );
	}
	else
	{
		RemoveFlag( FL_FROZEN );
	}

	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater(false);

	m_bReady = false;

	//Tony; do the spawn animevent
	DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

	SetContextThink( &CHL2MP_Player::HL2MPPushawayThink, gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, HL2MP_PUSHAWAY_THINK_CONTEXT );

	// S:O - Configure ourselves based on the team we're on.
	TeamConfig();

	// S:O - If we're on fire, extinguish the fire.
	if( IsOnFire() ) 
	{ 
		RemoveFlag( FL_ONFIRE ); 
		CBaseEntity *pEnt = GetEffectEntity();
		if( pEnt )
		{
			pEnt->Remove();
		}
	}
	
	// S:O - Redraw our viewmodel in case it was undrawn for one reason or another.
	CBaseViewModel *pViewModel = GetViewModel();
	pViewModel->RemoveEffects( EF_NODRAW );

	// S:O - Set our collision group for so_noblock.
	SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );

	if ( GetTeamNumber() == 2 )
		ClientPrint( this, HUD_PRINTTALK, "You have spawned as a %s1. Eat some brains!\n", GetZombieClassName() );

	m_iLateJoiner = 0;

	// S:O - LUA
	if(g)
	{
		g->PlayerSpawn(this);
	}
}

const char *CHL2MP_Player::GetZombieClassName( void )
{	
	const char *szClassName = NULL;

	if ( IsCharple() )
		szClassName = "Charple";
	else if ( IsCorpse() )
		szClassName = "Corpse";
	else if ( IsStalker() )
		szClassName = "Stalker";
	else if ( IsGhost() )
		szClassName = "Ghost";
	else
		szClassName = "zombie";

	return szClassName;
}

// S:O - Give survivors the ability to pickup objects of reasonable sizes and weights.
void CHL2MP_Player::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	if ( GetTeamNumber() == TEAM_SURVIVORS || GetTeamNumber() == TEAM_MILITARY )
	{
		if ( GetGroundEntity() == pObject )
			return;

		if ( bLimitMassAndSize == true )
		{
			if ( CBasePlayer::CanPickupObject( pObject, 250, 150 ) == false )
				return;
		}

		// Don't allow me to pick this up if there's an NPC on top of me!
		if ( pObject->HasNPCsOnIt() )
			return;

		PlayerPickupObject( this, pObject );
	}
	else
	{
		return;
	}
}

bool CHL2MP_Player::ValidatePlayerModel( const char *pModel )
{
	int iModels = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < iModels; ++i )
	{
		if ( !Q_stricmp( g_ppszRandomCitizenModels[i], pModel ) )
		{
			return true;
		}
	}

	iModels = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < iModels; ++i )
	{
	   	if ( !Q_stricmp( g_ppszRandomCombineModels[i], pModel ) )
		{
			return true;
		}
	}

	iModels = ARRAYSIZE( g_ppszRandomMilitaryModels );

	for ( i = 0; i < iModels; ++i )
	{
	   	if ( !Q_stricmp( g_ppszRandomMilitaryModels[i], pModel ) )
		{
			return true;
		}
	}

	return false;
}

// S:O - Misc. Modifications
// Modified only to prevent crashes, and nothing more
void CHL2MP_Player::SetPlayerTeamModel( void )
{
	const char *szModelName = szModelName = modelinfo->GetModelName( GetModel() );
	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( (ValidatePlayerModel( szModelName ) == false) || (modelIndex == -1) )
	{
		szModelName = "models/player/Zombies/zombie1.mdl";
		m_iModelType = TEAM_ZOMBIES;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}

	/*if ( GetTeamNumber() == TEAM_ZOMBIES )
	{
		if ( Q_stristr( szModelName, "models/player/Humans") )
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
			g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
			szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];
		}

		m_iModelType = TEAM_ZOMBIES;
	}
	else if ( GetTeamNumber() == TEAM_SURVIVORS )
	{
		if ( !Q_stristr( szModelName, "models/player/Humans") )
		{
			int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

			g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
			szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];
		}

		m_iModelType = TEAM_SURVIVORS;
	}
	else if ( GetTeamNumber() == TEAM_MILITARY )
	{
		if ( !Q_stristr( szModelName, "models/player/Military") )
		{
			int nHeads = ARRAYSIZE( g_ppszRandomMilitaryModels );

			g_iLastMilitaryModel = ( g_iLastMilitaryModel + 1 ) % nHeads;
			szModelName = g_ppszRandomMilitaryModels[g_iLastMilitaryModel];
		}

		m_iModelType = TEAM_MILITARY;
	}*/
	
	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

// S:O - Misc. Modifications
// We're always using teamplay, so this really isn't needed anymore
void CHL2MP_Player::SetPlayerModel( void )
{
	/*const char *szModelName = NULL;
	const char *pszCurrentModelName = modelinfo->GetModelName( GetModel());

	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	if ( ValidatePlayerModel( szModelName ) == false )
	{
		char szReturnString[512];

		if ( ValidatePlayerModel( pszCurrentModelName ) == false )
		{
			pszCurrentModelName = "models/player/Zombies/zombie1.mdl";
		}

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
		engine->ClientCommand ( edict(), szReturnString );

		szModelName = pszCurrentModelName;
	}

	if ( GetTeamNumber() == TEAM_ZOMBIES )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
		g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];

		m_iModelType = TEAM_ZOMBIES;
	}
	else if ( GetTeamNumber() == TEAM_SURVIVORS )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

		g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];

		m_iModelType = TEAM_SURVIVORS;
	}
	else if ( GetTeamNumber() == TEAM_MILITARY )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomMilitaryModels );

		g_iLastMilitaryModel = ( g_iLastMilitaryModel + 1 ) % nHeads;
		szModelName = g_ppszRandomMilitaryModels[g_iLastMilitaryModel];

		m_iModelType = TEAM_MILITARY;
	}
	else
	{
		if ( Q_strlen( szModelName ) == 0 ) 
		{
			szModelName = g_ppszRandomCitizenModels[0];
		}

		if ( Q_stristr( szModelName, "models/player/Humans") )
		{
			m_iModelType = TEAM_SURVIVORS;
		}
		else if ( Q_stristr( szModelName, "models/player/Zombies") )
		{
			m_iModelType = TEAM_ZOMBIES;
		}
		else if ( Q_stristr( szModelName, "models/player/Military") )
		{
			m_iModelType = TEAM_MILITARY;
		}
	}

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 )
	{
		szModelName = "models/player/Zombies/zombie1.mdl";
		m_iModelType = TEAM_ZOMBIES;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}

	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;*/
}

// S:O - Misc. Modifications
void CHL2MP_Player::SetupPlayerSoundsByModel( const char *pModelName )
{
	if ( Q_stristr( pModelName, "models/player/Humans") || Q_stristr( pModelName, "models/player/Military") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr(pModelName, "models/player/Zombies" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_COMBINESOLDIER;
	}
}

// S:O - Misc. edits
bool CHL2MP_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
	return bRet;
}

void CHL2MP_Player::PreThink( void )
{
	BaseClass::PreThink();
	State_PreThink();

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;

	// S:O -
	// If we're not using the sniper at any given time, we shouldn't be using its scope either.
	CBaseCombatWeapon *pSniper = Weapon_OwnsThisType("weapon_sniper");
	if ( (!pSniper) || ((pSniper) && (GetActiveWeapon() != pSniper)) )
	{
		CSingleUserRecipientFilter filter( this );
		UserMessageBegin(filter, "ShowScope");
			WRITE_BYTE(0);
		MessageEnd();
	}

	// S:O -
	// Regenerate players' health every two seconds.
	if( gpGlobals->curtime >= m_flHealthRegenTime )
	{
		int iHealth = GetHealth();
		int iMaxHealth = GetMaxHealth();

		if ( GetTeamNumber() == TEAM_ZOMBIES && so_zombie_healthregen.GetInt() != 0 && IsAlive() )
		{
			if ( iHealth < iMaxHealth )
			{
				if ( IsCorpse() )
					SetHealth( iHealth + 10.0 );
				else
					SetHealth( iHealth + 5.0 );
			}
			else if ( iHealth > iMaxHealth )
				SetHealth( iMaxHealth );
		}
		else if ( (GetTeamNumber() == TEAM_SURVIVORS || GetTeamNumber() == TEAM_MILITARY) && so_human_healthregen.GetInt() != 0 && IsAlive() )
		{
			if ( iHealth < iMaxHealth )
				SetHealth( iHealth + 1.0 );
			else if ( iHealth > iMaxHealth )
				SetHealth( iMaxHealth );
		}

		m_flHealthRegenTime = gpGlobals->curtime + ( 2.0f / so_healthregen_speed.GetFloat() );
	}

	// S:O -
	// Give cash to the zombie overlord every two seconds.
	if ( gpGlobals->curtime >= m_flGiveCommanderCash && FStrEq( so_gamemode.GetString(), "Overlord" ) )
	{
		if ( GetTeamNumber() == TEAM_ZOMBIES && IsAlive() )
		{
			int iMoney = m_iMoney;
			m_iMoney = iMoney + 25;
		}

		m_flGiveCommanderCash = gpGlobals->curtime + 2.0f;
	}

	// S:O -
	// Check if we've gained a level or not every five seconds.
	if ( gpGlobals->curtime >= m_flCheckLevelUp )
	{
		if ( IsAlive() )
			CheckLevel();

		m_flCheckLevelUp = gpGlobals->curtime + 5.0f;
	}

	// S:O -
	// Don't switch to spectator right away following death.
	// Instead, have a bit of a delay.
	if ( m_ReinforcementsTimer.IsElapsed() && !IsAlive() && (GetTeamNumber() != TEAM_SPECTATOR) && NeedsToBeSwitchedToReinforcements )
	{
		ChangeTeam( TEAM_SPECTATOR );

		// S:O - Display our custom death messages.
		if ( so_respawn.GetInt() == 1 )
		{
			char szSpawnMsg[128];
			Q_snprintf( szSpawnMsg, sizeof( szSpawnMsg ), "%i", HL2MPRules()->GetReinforcementsTimerRemain2() );
			ClientPrint( this, HUD_PRINTTALK, "You will respawn in approximately %s1 seconds.\n", szSpawnMsg );
		}
		else if ( so_respawn.GetInt() == 0 )
		{
			ClientPrint( this, HUD_PRINTTALK, "You will respawn next round." );
		}

		NeedsToBeSwitchedToReinforcements = false;
	}

	// S:O -
	// If we're low on health, BLEED!
	if ( gpGlobals->curtime >= m_fBleedingDelay )
	{
		if ( (GetTeamNumber() > 1) && (GetHealth() <= 20) && (GetHealth() != 0) && IsAlive() )
		{
			Vector vecSpot;
			Vector vecDir;

			vecSpot = GetAbsOrigin();
			vecSpot.x += random->RandomFloat( -12, 12 ); 
			vecSpot.y += random->RandomFloat( -12, 12 ); 
			vecSpot.z += random->RandomFloat( 50, 60 ); 

			// Make us bleed a bit to alert the player that they are in some serious trouble!
			UTIL_BloodDrips( vecSpot, vec3_origin, BLOOD_COLOR_RED, 1 );
				
			//CLocalPlayerFilter filter;
			//C_BaseEntity::EmitSound( filter, -1, "LowHealth.Warning" );
		}

		m_fBleedingDelay = gpGlobals->curtime + 1.5f;
	}

	// S:O - Infection system
	if ( IsInfected() && m_ZombifyTimer.IsElapsed() )
	{
		MakeInfected( false );
		ChangeTeam( 2 );

		ClientPrint( this, HUD_PRINTTALK, "You have become a %s1. Eat some brains!\n", GetZombieClassName() );
	}

	// S:O - Limit the amount of points a player can have at any given time to 20000
	int iMoney = m_iMoney;
	if ( iMoney > 20000 )
	{
		m_iMoney = 20000;
	}

	// S:O - Fix the spectator fake weapon glitch with this single piece of code.
	CBaseViewModel *pViewModel = GetViewModel();
	if ( pViewModel && (!IsAlive() || GetTeamNumber() == 1) )
		pViewModel->AddEffects( EF_NODRAW );
}

void CHL2MP_Player::PostThink( void )
{
	BaseClass::PostThink();
	
	if ( GetFlags() & FL_DUCKING )
	{
		SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
	}

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();
    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	// S:O - Alter a player's speed based upon their weapon and health
	if ( gpGlobals->curtime >= m_fSpeedChangeDelay )
	{
		if ( GetTeamNumber() == TEAM_ZOMBIES )
		{
			if ( IsPlayerWalking() )
				SetMaxSpeed( so_zombie_speed.GetFloat() * 0.65f );
			else if ( IsCharple() )
				// CHARPLE BENEFIT: Moves very quickly
				SetMaxSpeed( so_zombie_speed.GetFloat() * 1.50f );
			else if ( IsCorpse() )
				// CORPSE WEAKNESS: Doesn't move very quickly
				SetMaxSpeed( so_zombie_speed.GetFloat() * 0.80f );
			else if ( IsInfected() )
				SetMaxSpeed( so_zombie_speed.GetFloat() * 0.50f );
			else
				SetMaxSpeed( so_zombie_speed.GetFloat() );
		}
		else if ( GetTeamNumber() == TEAM_SURVIVORS || GetTeamNumber() == TEAM_MILITARY )
		{
			CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>( GetActiveWeapon() );

			if ( pWeapon )
			{
				if ( GetTeamNumber() == TEAM_SURVIVORS )
					SetMaxSpeed( 225 );
				else if ( GetTeamNumber() == TEAM_MILITARY )
					SetMaxSpeed( 200 );

				float maxspeed = (float)MaxSpeed();
				float healthratio = ((float)GetHealth() / (float)GetMaxHealth()) + 0.50;
				float weaponweight = (float)pWeapon->GetWeight();

				// Don't start deducting speed just because we've sustained a 'lil bit of damage
				if ( healthratio > 1.0 )
					healthratio = 1.0;

				float calcmaxspeed = (float)( ((maxspeed) * (healthratio)) - (weaponweight) );

				// Don't let us move TOO slowly so as not to piss people off
				// 100 is damn slow enough!
				if ( calcmaxspeed < 100.0 )
					calcmaxspeed = 100.0;

				if ( IsPlayerWalking() )
					SetMaxSpeed( calcmaxspeed * 0.65f );
				else
					SetMaxSpeed( calcmaxspeed );
			}
		}

		m_fSpeedChangeDelay = gpGlobals->curtime + 0.1f;
	}
}

void CHL2MP_Player::PlayerDeathThink()
{
	if( !IsObserver() )
	{
		BaseClass::PlayerDeathThink();
	}
}

void CHL2MP_Player::FireBullets ( const FireBulletsInfo_t &info )
{
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );

	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>( GetActiveWeapon() );

	if ( pWeapon )
	{
		modinfo.m_iPlayerDamage = modinfo.m_iDamage = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
	}

	NoteWeaponFired();

	BaseClass::FireBullets( modinfo );

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( this );
}

void CHL2MP_Player::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

bool CHL2MP_Player::WantsLagCompensationOnEntity( const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const // AI Patch Addition.
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	return BaseClass::WantsLagCompensationOnEntity( pEntity, pCmd, pEntityTransmitBits ); // AI Patch Addition.
}

Activity CHL2MP_Player::TranslateTeamActivity( Activity ActToTranslate )
{
	if ( m_iModelType == TEAM_ZOMBIES )
		 return ActToTranslate;
	
	if ( ActToTranslate == ACT_RUN )
		 return ACT_RUN_AIM_AGITATED;

	if ( ActToTranslate == ACT_IDLE )
		 return ACT_IDLE_AIM_AGITATED;

	if ( ActToTranslate == ACT_WALK )
		 return ACT_WALK_AIM_AGITATED;

	return ActToTranslate;
}

extern ConVar hl2_normspeed;

extern int	gEvilImpulse101;
//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CHL2MP_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
	{
		return false;
	}

	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());

	if ( bOwnsWeaponAlready == true ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		 if ( Weapon_EquipAmmoOnly( pWeapon ) )
		 {
			 pWeapon->CheckRespawn();

			 UTIL_Remove( pWeapon );
			 return true;
		 }
		 else
		 {
			 return false;
		 }
	}

	// S:O - Only allow a survivor to carry one weapon per bucket and slot.
	for ( int i = 0; i < WeaponCount(); i++ )
    {
    	CBaseCombatWeapon *pSearch = GetWeapon( i );

    	if ( (pSearch) && (pSearch->GetSlot() == pWeapon->GetSlot()) )
    	{
    		return false;
    	}
    }

	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );

	// S:O - Switch to the weapon we just picked up, regardless of its power or worth.
	Weapon_Switch( pWeapon );

	return true;
}

// S:O - Determine whether or not we're allowed to use a weapon based upon the team we're on.
bool CHL2MP_Player::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(this);

	if ( !pPlayer )
		return 0;

	if ( GetTeamNumber() == 2 )
	{
		if ( !pWeapon->ClassMatches( "weapon_zombie" ) && !pWeapon->ClassMatches( "weapon_corpse" ) && !pWeapon->ClassMatches( "weapon_charple" ) && !pWeapon->ClassMatches( "weapon_stalker" ) )
		{
			return false;
		}
	}
	else if (GetTeamNumber() == 3 || GetTeamNumber() == 4)
	{
		if ( pWeapon->ClassMatches( "weapon_zombie" ) || pWeapon->ClassMatches( "weapon_corpse" ) || pWeapon->ClassMatches( "weapon_charple" ) || pWeapon->ClassMatches( "weapon_stalker" ) )
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// if we made it this far, our rules see no reason why they shouldn't use it,
	// but the original rules might, so let them decide
	return BaseClass::Weapon_CanUse(pWeapon);
}

// S:O - Team-specific Configuration
void CHL2MP_Player::TeamConfig( void )
{
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(this);
	if ( !pPlayer )
		return;

	// S:O - Reinforcements
	if ( GetTeamNumber() == 1 )
	{
		m_iHealth = 1;
		m_iMaxHealth = 1;
		m_iFOV = 90;
		GiveDefaultItems();
		FlashlightTurnOff();

		// Attempt to remove this player from our list of Survivors.
		// If we don't find the player in our list, they're probably spawning for the first time this round.
		gEntList.m_PlayerList.FindAndRemove(this);
		gEntList.m_EscapedList.FindAndRemove(this);

		MakeInfected( false );
	}

	// S:O - Zombies
	else if ( GetTeamNumber() == 2 )
	{
		// A bit of a hack to see if we're a ghost or not (different from IsGhost() boolean function)
		bool m_bIsGhost = false;

		// See next comment below...
		m_iFOV = 99;
		GiveDefaultItems();
		FlashlightTurnOff();
		StopObserverMode();

		// This block with the models and the like needs to be the first thing we do to ensure zombie class configuration is applied properly
		if ( FStrEq(so_gamemode.GetString(), "Infection") )
		{
			if ( IsInfected() )
			{
				int randzombie = RandomInt(1,3);
				switch ( randzombie )
				{
					case 1:
						SetModel("models/player/Zombies/zombie1.mdl");
					break;

					case 2:
						SetModel("models/player/Zombies/zombie2.mdl");
					break;

					case 3:
						SetModel("models/player/Zombies/zombie3.mdl");
					break;
				}
			}
			else
			{
				int randzombie = RandomInt(1,4);
				switch ( randzombie )
				{
					case 1:
						SetModel("models/player/Zombies/stalker.mdl");
						GiveNamedItem( "weapon_stalker" );
						Weapon_Switch( Weapon_OwnsThisType( "weapon_stalker" ) );
					break;

					case 2:
						SetModel("models/player/Zombies/corpse.mdl");
						GiveNamedItem( "weapon_corpse" );
						Weapon_Switch( Weapon_OwnsThisType( "weapon_corpse" ) );
					break;

					case 3:
						SetModel("models/player/Zombies/charple.mdl");
						GiveNamedItem( "weapon_charple" );
						Weapon_Switch( Weapon_OwnsThisType( "weapon_charple" ) );
					break;

					case 4:
						m_bIsGhost = true;
						SetModel("models/player/Zombies/zombie1.mdl");
						GiveNamedItem( "weapon_zombie" );
						Weapon_Switch( Weapon_OwnsThisType( "weapon_zombie" ) );
					break;
				}
			}
		}
		else
		{
			if ( IsInfected() )
			{
				int randzombie = RandomInt(1,3);
				switch ( randzombie )
				{
					case 1:
						SetModel("models/player/Zombies/zombie1.mdl");
					break;

					case 2:
						SetModel("models/player/Zombies/zombie2.mdl");
					break;

					case 3:
						SetModel("models/player/Zombies/zombie3.mdl");
					break;
				}
			}
			else
			{
				int randzombie = RandomInt(1,3);
				switch ( randzombie )
				{
					case 1:
						SetModel("models/player/Zombies/zombie1.mdl");
					break;

					case 2:
						SetModel("models/player/Zombies/zombie1.mdl");
					break;

					case 3:
						SetModel("models/player/Zombies/zombie1.mdl");
					break;
				}

				GiveNamedItem( "weapon_zombie" );
				Weapon_Switch( Weapon_OwnsThisType( "weapon_zombie" ) );
			}
		}

		if ( FStrEq(so_gamemode.GetString(), "Overlord") )
		{
			int m_iOverlordExtraHealth = (so_zombie_health.GetInt() + so_zombie_health.GetInt());
			m_iHealth = m_iOverlordExtraHealth;
			m_iMaxHealth = m_iOverlordExtraHealth;
		}
		else
		{
			int m_iZombieHealth = 1;
			if ( IsGhost() )
			{
				// GHOST WEAKNESS: Really, really weak
				m_iZombieHealth = so_zombie_health.GetInt() * 0.25;
			}
			if ( IsStalker() )
			{
				// STALKER WEAKNESS: Doesn't have a whole lot of health
				m_iZombieHealth = so_zombie_health.GetInt() * 0.5;
			}
			else if ( IsCorpse() )
			{
				// CORPSE BENEFIT: Has a whole lot of health
				m_iZombieHealth = so_zombie_health.GetInt() * 1.5;
			}
			else if ( !IsInfected() )
			{
				m_iZombieHealth = so_zombie_health.GetInt();
			}

			if ( !IsInfected() )
			{
				m_iHealth = m_iZombieHealth;
				m_iMaxHealth = m_iZombieHealth;
			}
		}

		if ( IsInfected() )
		{
			EmitSound("Infected");
			SetMaxSpeed( so_zombie_speed.GetFloat() * 0.50f );
		}
		else
		{
			EmitSound("Zombify");
			SetMaxSpeed( so_zombie_speed.GetFloat() );
		}

		// Attempt to remove this player from our list of Survivors.
		// If we don't find the player in our list, they're probably spawning for the first time this round.
		gEntList.m_PlayerList.FindAndRemove(this);
		gEntList.m_EscapedList.FindAndRemove(this);

		// S:O - Only make ghosts (zombie class) invisible and whatnot
		CBaseViewModel *pViewModel = GetViewModel();
		if ( m_bIsGhost )
		{
			SetRenderMode( kRenderTransColor );
			SetRenderColorA(50);

			if ( pViewModel )
			{
				pViewModel->SetRenderMode( kRenderTransColor );
				pViewModel->SetRenderColorA(50);
			}
		}
		else
		{
			SetRenderMode( kRenderNormal );
			SetRenderColorA(255);

			if ( pViewModel )
			{
				pViewModel->SetRenderMode( kRenderNormal );
				pViewModel->SetRenderColorA(255);
			}
		}

		// Make this the absolute LAST call for each player, just in case.
		ReapplyLevelPowerups();
	}

	// S:O - Survivors
	else if ( GetTeamNumber() == 3 )
	{
		m_iHealth = 100;
		m_iMaxHealth = 100;
		SetMaxSpeed( 225 );
		m_iFOV = 90;

		if ( Weapon_OwnsThisType("weapon_zombie") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_zombie") );
		else if ( Weapon_OwnsThisType("weapon_charple") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_charple") );
		else if ( Weapon_OwnsThisType("weapon_stalker") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_stalker") );
		else if ( Weapon_OwnsThisType("weapon_corpse") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_corpse") );

		// S:O - Add this player to our list of Survivors if we haven't done so already
		if ( gEntList.m_PlayerList.Find( pPlayer ) == -1 )
		{
			gEntList.m_PlayerList.AddToTail(this);
		}

		GiveDefaultItems();
		FlashlightTurnOff();
		StopObserverMode();

		int iAdditionalMoney = 0;
		switch ( GetLevel() )
		{
			case 1:
				iAdditionalMoney = 0;
			break;

			case 2:
				iAdditionalMoney = 1000;
			break;

			case 3:
				iAdditionalMoney = 2000;
			break;

			case 4:
				iAdditionalMoney = 3000;
			break;

			case 5:
				iAdditionalMoney = 4000;
			break;
		}

		int iMoney = m_iMoney;
		if ( iMoney < (so_spawnmoney.GetInt() + iAdditionalMoney) )
			m_iMoney = so_spawnmoney.GetInt() + iAdditionalMoney;

		int randsurvivor = RandomInt(1,27);
		switch ( randsurvivor )
		{
			case 1:
				SetModel("models/player/Humans/survivor1.mdl"); 
				break;

			case 2:
				SetModel("models/player/Humans/survivor2.mdl"); 
				break;

			case 3:
				SetModel("models/player/Humans/survivor3.mdl"); 
				break;

			case 4:
				SetModel("models/player/Humans/survivor4.mdl"); 
				break;

			case 5:
				SetModel("models/player/Humans/survivor5.mdl"); 
				break;

			case 6:
				SetModel("models/player/Humans/survivor6.mdl"); 
				break;

			case 7:
				SetModel("models/player/Humans/survivor7.mdl"); 
				break;

			case 8:
				SetModel("models/player/Humans/survivor8.mdl"); 
				break;

			case 9:
				SetModel("models/player/Humans/survivor9.mdl"); 
				break;

			case 10:
				SetModel("models/player/Humans/survivor10.mdl"); 
				break;

			case 11:
				SetModel("models/player/Humans/survivor11.mdl"); 
				break;

			case 12:
				SetModel("models/player/Humans/survivor12.mdl"); 
				break;

			case 13:
				SetModel("models/player/Humans/survivor13.mdl"); 
				break;

			case 14:
				SetModel("models/player/Humans/survivor14.mdl"); 
				break;

			case 15:
				SetModel("models/player/Humans/survivor15.mdl"); 
				break;

			case 16:
				SetModel("models/player/Humans/survivor16.mdl"); 
				break;

			case 17:
				SetModel("models/player/Humans/survivor17.mdl"); 
				break;

			case 18:
				SetModel("models/player/Humans/survivor18.mdl"); 
				break;

			case 19:
				SetModel("models/player/Humans/survivor19.mdl"); 
				break;

			case 20:
				SetModel("models/player/Humans/survivor20.mdl"); 
				break;

			case 21:
				SetModel("models/player/Humans/survivor21.mdl"); 
				break;

			case 22:
				SetModel("models/player/Humans/survivor22.mdl"); 
				break;

			case 23:
				SetModel("models/player/Humans/survivor23.mdl"); 
				break;

			case 24:
				SetModel("models/player/Humans/survivor24.mdl"); 
				break;

			case 25:
				SetModel("models/player/Humans/survivor25.mdl"); 
				break;

			case 26:
				SetModel("models/player/Humans/survivor26.mdl"); 
				break;

			case 27:
				SetModel("models/player/Humans/survivor27.mdl"); 
				break;
		}


		// S:O - Only make ghosts (zombie class) invisible and whatnot
		SetRenderMode( kRenderNormal );
		SetRenderColorA(255);

		CBaseViewModel *pViewModel = GetViewModel();
		if ( pViewModel )
		{
			pViewModel->SetRenderMode( kRenderNormal );
			pViewModel->SetRenderColorA(255);
		}

		MakeInfected( false );

		// Make this the absolute LAST call for each player, just in case.
		ReapplyLevelPowerups();
	}

	// S:O - Military
	else if ( GetTeamNumber() == 4 )
	{
		m_iHealth = 125;
		m_iMaxHealth = 125;
		SetMaxSpeed( 200 );
		m_iFOV = 90;

		if ( Weapon_OwnsThisType("weapon_zombie") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_zombie") );
		else if ( Weapon_OwnsThisType("weapon_charple") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_charple") );
		else if ( Weapon_OwnsThisType("weapon_stalker") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_stalker") );
		else if ( Weapon_OwnsThisType("weapon_corpse") )
			RemovePlayerItem( Weapon_OwnsThisType("weapon_corpse") );

		GiveDefaultItems();
		FlashlightTurnOff();
		StopObserverMode();

		int iAdditionalMoney = 0;
		switch ( GetLevel() )
		{
			case 1:
				iAdditionalMoney = 0;
			break;

			case 2:
				iAdditionalMoney = 1000;
			break;

			case 3:
				iAdditionalMoney = 2000;
			break;

			case 4:
				iAdditionalMoney = 3000;
			break;

			case 5:
				iAdditionalMoney = 4000;
			break;
		}

		int iMoney = m_iMoney;
		if ( iMoney < (so_spawnmoney.GetInt() + iAdditionalMoney) )
			m_iMoney = so_spawnmoney.GetInt() + iAdditionalMoney;

		SetModel("models/player/Military/soldier.mdl");

		int randmilitaryskin = RandomInt(1,3);
		switch ( randmilitaryskin )
		{
			case 1:
				SetSkin(0);
			break;

			case 2:
				SetSkin(1);
			break;

			case 3:
				SetSkin(2);
			break;
		}

		// S:O - Only make ghosts (zombie class) invisible and whatnot
		SetRenderMode( kRenderNormal );
		SetRenderColorA(255);

		CBaseViewModel *pViewModel = GetViewModel();
		if ( pViewModel )
		{
			pViewModel->SetRenderMode( kRenderNormal );
			pViewModel->SetRenderColorA(255);
		}

		MakeInfected( false );

		// Make this the absolute LAST call for each player, just in case.
		ReapplyLevelPowerups();
	}
	else
	{
		// Attempt to remove this player from our list of Survivors.
		// If we don't find the player in our list, they're probably spawning for the first time this round.
		gEntList.m_PlayerList.FindAndRemove(this);
		gEntList.m_EscapedList.FindAndRemove(this);

		MakeInfected( false );

		// S:O - Spawned on a NULL team, or on a team that has not been fully implemented yet.
		// In other words, fail.
		return;
	}
}

void CHL2MP_Player::ChangeTeam( int iTeam )
{
/*	if ( GetNextTeamChangeTime() >= gpGlobals->curtime )
	{
		char szReturnString[128];
		Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch teams again.\n", (int)(GetNextTeamChangeTime() - gpGlobals->curtime) );

		ClientPrint( this, HUD_PRINTTALK, szReturnString );
		return;
	}*/

	bool bKill = false;

	if ( HL2MPRules()->IsTeamplay() != true && iTeam != TEAM_SPECTATOR )
	{
		//don't let them try to join combine or rebels during deathmatch.
		iTeam = TEAM_UNASSIGNED;
	}

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED )
		{
			bKill = true;
		}
	}

	BaseClass::ChangeTeam( iTeam );

	m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		SetPlayerTeamModel();
	}
	else
	{
		SetPlayerModel();
	}

	// S:O - Load team-specific configuration.
	TeamConfig();

	if ( iTeam == TEAM_SPECTATOR )
	{
		RemoveAllItems( true );

		State_Transition( STATE_OBSERVER_MODE );
	}

	if ( bKill == true )
	{
		// S:O - NEVER kill players attempting to change teams, because they're likely being told to do so by our code.
		//CommitSuicide();
	}
}

bool CHL2MP_Player::HandleCommand_JoinTeam( int team )
{
	if ( !GetGlobalTeam( team ) || team == 0 )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

	if ( team == TEAM_SPECTATOR )
	{
		// S:O - We're always allowed to be a spectator in this mod.
		// Prevent this is the cvar is set
		/*if ( !mp_allowspectators.GetInt() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return false;
		}*/

		if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() )
		{
			m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work

			CommitSuicide();

			// add 1 to frags to balance out the 1 subtracted for killing yourself
			IncrementFragCount( 1 );
		}

		ChangeTeam( TEAM_SPECTATOR );

		return true;
	}
	else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}

	// Switch their actual team...
	ChangeTeam( team );

	return true;
}

bool CHL2MP_Player::ClientCommand( const CCommand &args )
{
	if ( FStrEq( args[0], "spectate" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// instantly join spectators
			HandleCommand_JoinTeam( TEAM_SPECTATOR );
		}

		return true;
	}

	// S:O - Add a check for legacy command zms_gamemode to prevent breaking older ZM:S-based maps.
	if ( FStrEq( args[0], "zms_gamemode" ) )
	{
		so_gamemode.SetValue( args[1] );
		return true;
	}

	if ( FStrEq( args[0], "jointeam" ) ) 
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad jointeam syntax\n" );
		}

		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iTeam = atoi( args[1] );

			// S:O - We cannot willingly join a specific team in this mod.
			// This is controlled for us later on by the code.
			// THIS IS NOT A DEMOCRACY, DAMNIT!
			if(iTeam == 2)
			{
				return false;
			}
			if(GetTeamNumber() == 2)
			{
				return false;
			}
			if(GetTeamNumber() == 3)
			{
				return false;
			}
			if(GetTeamNumber() == 4)
			{
				return false;
			}
			else if(GetTeamNumber() == 0)
			{
				HandleCommand_JoinTeam( iTeam );
				return true;
			}
			else if(GetTeamNumber() == 1)
			{
				HandleCommand_JoinTeam( iTeam );
				return true;
			}
		}
		return true;
	}
	else if ( FStrEq( args[0], "joingame" ) )
	{
		return true;
	}

	return BaseClass::ClientCommand( args );
}

void CHL2MP_Player::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 101:
			{
				if( sv_cheats->GetBool() )
				{
					GiveAllItems();
				}
			}
			break;

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CHL2MP_Player::ShouldRunRateLimitedCommand( const CCommand &args )
{
	int i = m_RateLimitLastCommandTimes.Find( args[0] );
	if ( i == m_RateLimitLastCommandTimes.InvalidIndex() )
	{
		m_RateLimitLastCommandTimes.Insert( args[0], gpGlobals->curtime );
		return true;
	}
	else if ( (gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HL2MP_COMMAND_MAX_RATE )
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

void CHL2MP_Player::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CSOViewModel *vm = ( CSOViewModel * )CreateEntityByName( "so_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CHL2MP_Player::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}

// -------------------------------------------------------------------------------- //
// Ragdoll entities.
// -------------------------------------------------------------------------------- //

class CHL2MPRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CHL2MPRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( hl2mp_ragdoll, CHL2MPRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CHL2MPRagdoll, DT_HL2MPRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


void CHL2MP_Player::CreateRagdollEntity( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// If we already have a ragdoll, don't make another one.
	CHL2MPRagdoll *pRagdoll = dynamic_cast< CHL2MPRagdoll* >( m_hRagdoll.Get() );
	
	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CHL2MPRagdoll* >( CreateEntityByName( "hl2mp_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2MP_Player::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOn( void )
{
	// S:O - Only humans can use their flashlights at this time.
	// UPDATE: Zombies now have "zombievision" anyway, so this really shouldn't be a big deal.
	if( flashlight.GetInt() > 0 && IsAlive() && (GetTeamNumber() == 3 || GetTeamNumber() == 4) )
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "HL2Player.FlashlightOn" );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
	{
		EmitSound( "HL2Player.FlashlightOff" );
	}
}

void CHL2MP_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	// S:O - Misc. [TEST]
	// TODO: See if this works the way I think it does or not.

	//Drop a grenade if it's primed.
	/*if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType("weapon_frag");

		if ( GetActiveWeapon() == pGrenade )
		{
			if ( ( m_nButtons & IN_ATTACK ) || (m_nButtons & IN_ATTACK2) )
			{
				DropPrimedFragGrenade( this, pGrenade );
				return;
			}
		}
	}*/

	// S:O - Check to see if this weapon is valid to prevent crashes.
	if ( pWeapon )
	{
		// S:O - Make a dropped weapon blink.
		pWeapon->AddEffects( EF_ITEM_BLINK );

		BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
	}
}

void CHL2MP_Player::DetonateTripmines( void )
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_satchel" )) != NULL)
	{
		CSatchelCharge *pSatchel = dynamic_cast<CSatchelCharge *>(pEntity);
		if (pSatchel->m_bIsLive && pSatchel->GetThrower() == this )
		{
			g_EventQueue.AddEvent( pSatchel, "Explode", 0.20, this, this );
		}
	}

	// Play sound for pressing the detonator
	EmitSound( "Weapon_SLAM.SatchelDetonate" );
}

void CHL2MP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	if ( GetTeamNumber() == TEAM_SURVIVORS )
	{
		// S:O - The player died, so let's remove him or her from our list of Survivors.
		gEntList.m_PlayerList.FindAndRemove(this);
		gEntList.m_EscapedList.FindAndRemove(this);

		// S:O - If we're dealing with a zombie overlord, award the overlord some extra points to help fund their little operation!
		if ( FStrEq( so_gamemode.GetString(), "Overlord" ) )
		{
			for ( int i = 0; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pCommander = dynamic_cast<CHL2MP_Player*>( UTIL_PlayerByIndex( i ) );

				if ( (pCommander) && (pCommander != this) && (pCommander->GetTeamNumber() == TEAM_ZOMBIES) )
				{
					int iMoney = pCommander->m_iMoney;
					pCommander->m_iMoney = iMoney + 1000;

					// Some experience, too!
					pCommander->AddXP(10);
				}
			}
		}
	}

	// S:O - Only create a ragdoll if we were NOT a spectator.
	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	if ( GetTeamNumber() != TEAM_SPECTATOR )
		CreateRagdollEntity();

	// S:O - Doesn't do much of anything
	//DetonateTripmines();

	BaseClass::Event_Killed( subinfo );

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
		{
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
		}
	}
	// S:O - Blast or burn damage? Make our ragdoll burn, just to rub it in!
	else if ( info.GetDamageType() & ( DMG_BLAST | DMG_BURN ) )
	{
		if( m_hRagdoll )
		{
			CBaseAnimating *pRagdoll = (CBaseAnimating *)CBaseEntity::Instance(m_hRagdoll);
			pRagdoll->Ignite(45, false, 10 );
		}
	}

	CBaseEntity *pAttacker = info.GetAttacker();

	if ( pAttacker )
	{
		int iScoreToAdd = 1;

		if ( pAttacker == this )
		{
			iScoreToAdd = -1;
		}

		CHL2MP_Player *pAttackerPlayer = dynamic_cast<CHL2MP_Player*>( pAttacker );
		if ( pAttackerPlayer && (pAttackerPlayer != this) )
		{
			// S:O - Award players some experience for their kill.
			pAttackerPlayer->AddXP(10);

			// S:O - Award players some points for their kill.
			int iMoney = pAttackerPlayer->m_iMoney;
			pAttackerPlayer->m_iMoney = iMoney + 500;

			GetGlobalTeam( pAttackerPlayer->GetTeamNumber() )->AddScore( iScoreToAdd );

			// S:O - There's a chance the player who killed us could become a zombie if they were too close to us when we died...
			if( (so_infection_system.GetInt() != 0) && (so_infection_chance.GetInt() != 0) && !FStrEq(so_gamemode.GetString(), "Overlord") && (pAttackerPlayer->GetTeamNumber() == 3 || pAttackerPlayer->GetTeamNumber() == 4) && !pAttackerPlayer->IsInfected() )
			{
				CBaseEntity *pEntList[100];
				int numEnts = UTIL_EntitiesInSphere( pEntList, 100, GetAbsOrigin(), 75.0f, 0 );
				for ( int i = 0; i < numEnts; i++ )
				{
					if ( pEntList[i] && (pEntList[i] == pAttackerPlayer) )
					{
						int InfectionRand = (int)(random->RandomFloat( 1, so_infection_chance.GetInt() ));
						if ( InfectionRand >= (so_infection_chance.GetInt() - 1) )
						{
							CHL2MPRules *pRules = HL2MPRules();
							if ( !pRules )
								continue;

							pRules->ZombifyPlayer( pAttackerPlayer );
						}
					}
				}
			}
		}
	}

	// S:O - Punish the player for dying by removing all of their points
	// Don't worry, when they respawn they will get some back
	m_iMoney = 0;

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body
	StopZooming();

	// S:O - Balance our frag count.
	// TODO: See if this is really necessary anymore.
	IncrementFragCount( 1 );

	// S:O - Clear any screenfades.
	color32 nothing = {0,0,0,255};
	UTIL_ScreenFade( this, nothing, 0, 0, FFADE_IN | FFADE_PURGE );

	// S:O - Display our custom death messages.
	if ( so_respawn.GetInt() == 1 )
		ClientPrint( this, HUD_PRINTTALK, "Oh dear, you are dead!" );
	else if ( so_respawn.GetInt() == 0 )
		ClientPrint( this, HUD_PRINTTALK, "Oh dear, you are dead!" );

	// S:O - Tell the code we've died and are ready to be switched to spectator until we respawn.
	m_ReinforcementsTimer.Start( 5.0f );
	NeedsToBeSwitchedToReinforcements = true;

	// S:O - If we were infected, we certainly aren't anymore!
	if ( IsInfected() )
		MakeInfected( false );

	// S:O - Fix a little glitch that makes our infected sound play even after we're dead. Rather annoying!
	StopSound("Infected");

	// S:O - Experimental simple gib system
	/*CGib::SpawnSpecificGibs( this, 3, 50, 1, "models/gibs/hgibs_rib.mdl", 5 );
	CGib::SpawnSpecificGibs( this, 2, 50, 1, "models/gibs/hgibs_scapula.mdl", 5 );
	CGib::SpawnSpecificGibs( this, 1, 50, 1, "models/gibs/hgibs_spine.mdl", 5 );
	UTIL_BloodDrips( GetAbsOrigin(), vec3_origin, BLOOD_COLOR_RED, 500 );*/
}

int CHL2MP_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CHL2MP_Player *pAttacker = dynamic_cast<CHL2MP_Player*>( inputInfo.GetAttacker() );

	//return here if the player is in the respawn grace period vs. slams.
	if ( gpGlobals->curtime < m_flSlamProtectTime &&  (inputInfo.GetDamageType() == DMG_BLAST ) )
		return 0;

	m_vecTotalBulletForce += inputInfo.GetDamageForce();
	
	gamestats->Event_PlayerDamage( this, inputInfo );

	if ( pAttacker )
	{
		// S:O - Award players some money for their damage
		int addmoneyratio;

		// S:O - Give players more money depending on the difficulty
		if ( so_dyndiff_level.GetInt() == 0 )
			addmoneyratio = inputInfo.GetDamage() / 1.0;
		else if ( so_dyndiff_level.GetInt() == 1 )
			addmoneyratio = inputInfo.GetDamage() / 0.9;
		else if ( so_dyndiff_level.GetInt() == 2 )
			addmoneyratio = inputInfo.GetDamage() / 0.8;
		else if ( so_dyndiff_level.GetInt() == 3 )
			addmoneyratio = inputInfo.GetDamage() / 0.7;
		else if ( so_dyndiff_level.GetInt() == 4 )
			addmoneyratio = inputInfo.GetDamage() / 0.6;
		else if ( so_dyndiff_level.GetInt() == 5 )
			addmoneyratio = inputInfo.GetDamage() / 0.5;
		else
			addmoneyratio = 0;

		int m_iCurrentMoney = pAttacker->m_iMoney;
		pAttacker->m_iMoney = m_iCurrentMoney + addmoneyratio;
	}

	// S:O - Initialize custom damage effects and sounds.
	if ( GetTeamNumber() == 2 )
	{
		float yawKick = random->RandomFloat( -10, 10 );
		float yawDirection = random->RandomFloat( -10, 10 );

		// Kick the player angles
		ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

		EmitSound("Zombie.Pain");
	}
	else if ( GetTeamNumber() == 3 )
	{
		float yawKick = random->RandomFloat( -10, 10 );
		float yawDirection = random->RandomFloat( -10, 10 );

		// Kick the player angles
		ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

		if ( m_iHealth > 0 )
		{
			const char *pszCurrentModelName = modelinfo->GetModelName( GetModel() );

			if ( FStrEq( pszCurrentModelName, "models/player/Humans/survivor1.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor2.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor3.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor4.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor5.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor6.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor8.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor9.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor10.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor11.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor12.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor13.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor14.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor15.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor16.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor17.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor18.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor19.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor20.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor21.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor22.mdl" ) ||
				 FStrEq( pszCurrentModelName, "models/player/Humans/survivor23.mdl" ) )
			{
				EmitSound( "Male.Pain" );
			}
			else if ( FStrEq( pszCurrentModelName, "models/player/Humans/survivor24.mdl" ) ||
					  FStrEq( pszCurrentModelName, "models/player/Humans/survivor25.mdl" ) ||
					  FStrEq( pszCurrentModelName, "models/player/Humans/survivor26.mdl" ) ||
					  FStrEq( pszCurrentModelName, "models/player/Humans/survivor27.mdl" ) ||
					  FStrEq( pszCurrentModelName, "models/player/Humans/survivor7.mdl" ) )
			{
				EmitSound( "Female.Pain" );
			}
		}
	}
	else if ( GetTeamNumber() == 4 )
	{
		float yawKick = random->RandomFloat( -10, 10 );
		float yawDirection = random->RandomFloat( -10, 10 );

		// Kick the player angles
		ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

		if ( m_iHealth > 0 )
			EmitSound( "Military.Pain" );
	}

	return BaseClass::OnTakeDamage( inputInfo );
}

void CHL2MP_Player::DeathSound( const CTakeDamageInfo &info )
{
	// S:O - If we're a spectator, we shouldn't be making any noises when we die.
	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return;

	if ( m_hRagdoll && m_hRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;

	// S:O - Initialize custom death sounds.
	if ( GetTeamNumber() == 2 )
	{
		EmitSound("Zombie.Death");
	}
	else if ( GetTeamNumber() == 3 )
	{
		const char *pszCurrentModelName = modelinfo->GetModelName( GetModel() );

		if ( FStrEq( pszCurrentModelName, "models/player/Humans/survivor1.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor2.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor3.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor4.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor5.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor6.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor8.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor9.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor10.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor11.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor12.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor13.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor14.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor15.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor16.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor17.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor18.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor19.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor20.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor21.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor22.mdl" ) ||
			 FStrEq( pszCurrentModelName, "models/player/Humans/survivor23.mdl" ) )
		{
			EmitSound( "Male.Die" );
		}
		else if ( FStrEq( pszCurrentModelName, "models/player/Humans/survivor24.mdl" ) ||
				  FStrEq( pszCurrentModelName, "models/player/Humans/survivor25.mdl" ) ||
				  FStrEq( pszCurrentModelName, "models/player/Humans/survivor26.mdl" ) ||
				  FStrEq( pszCurrentModelName, "models/player/Humans/survivor27.mdl" ) ||
				  FStrEq( pszCurrentModelName, "models/player/Humans/survivor7.mdl" ) )
		{
			EmitSound( "Female.Die" );
		}
	}
	else if ( GetTeamNumber() == 4 )
	{
		EmitSound( "Military.Death" );
	}

	// S:O - Misc.
	// TODO: Confirm we don't need any of this anymore.

	/*char szStepSound[128];

	Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.Die", GetPlayerModelSoundPrefix() );

	const char *pModelName = STRING( GetModelName() );

	CSoundParameters params;
	if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
		return;

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );*/
}

// S:O - Modified spawnpoint function
CBaseEntity* CHL2MP_Player::EntSelectSpawnPoint( void )
{
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pLastSpawnPoint = g_pLastSpawn;

	// When in doubt, use info_player_start
	const char *pSpawnpointName = "info_player_start";

	// Do a search for a valid spawnpoint
	if ( FStrEq(so_gamemode.GetString(), "Extermination") )
	{
		if ( GetTeamNumber() == 3 )
		{
			if ( gEntList.FindEntityByClassname( NULL, "so_spawnpoint" ) != NULL )
				pSpawnpointName = "so_spawnpoint";
			if ( gEntList.FindEntityByClassname( NULL, "so_ext_survivor_spawnpoint" ) != NULL )
				pSpawnpointName = "so_ext_survivor_spawnpoint";
		}
		else if ( GetTeamNumber() == 4 )
		{
			if ( gEntList.FindEntityByClassname( NULL, "so_spawnpoint" ) != NULL )
				pSpawnpointName = "so_spawnpoint";
			if ( gEntList.FindEntityByClassname( NULL, "so_ext_military_spawnpoint" ) != NULL )
				pSpawnpointName = "so_ext_military_spawnpoint";
		}
		else
		{
			if ( gEntList.FindEntityByClassname( NULL, "info_player_rebel" ) != NULL )
				pSpawnpointName = "info_player_rebel";
			if ( gEntList.FindEntityByClassname( NULL, "info_player_combine" ) != NULL )
				pSpawnpointName = "info_player_combine";
			if ( gEntList.FindEntityByClassname( NULL, "info_player_deathmatch" ) != NULL )
				pSpawnpointName = "info_player_deathmatch";
			if ( gEntList.FindEntityByClassname( NULL, "so_ext_survivor_spawnpoint" ) != NULL )
				pSpawnpointName = "so_ext_survivor_spawnpoint";
			if ( gEntList.FindEntityByClassname( NULL, "so_ext_military_spawnpoint" ) != NULL )
				pSpawnpointName = "so_ext_military_spawnpoint";
			if ( gEntList.FindEntityByClassname( NULL, "zms_spawnpoint" ) != NULL )
				pSpawnpointName = "zms_spawnpoint";
			if ( gEntList.FindEntityByClassname( NULL, "so_spawnpoint" ) != NULL )
				pSpawnpointName = "so_spawnpoint";
		}
	}
	else
	{
		if ( gEntList.FindEntityByClassname( NULL, "info_player_rebel" ) != NULL )
			pSpawnpointName = "info_player_rebel";
		if ( gEntList.FindEntityByClassname( NULL, "info_player_combine" ) != NULL )
			pSpawnpointName = "info_player_combine";
		if ( gEntList.FindEntityByClassname( NULL, "info_player_deathmatch" ) != NULL )
			pSpawnpointName = "info_player_deathmatch";
		if ( gEntList.FindEntityByClassname( NULL, "so_ext_survivor_spawnpoint" ) != NULL )
			pSpawnpointName = "so_ext_survivor_spawnpoint";
		if ( gEntList.FindEntityByClassname( NULL, "so_ext_military_spawnpoint" ) != NULL )
			pSpawnpointName = "so_ext_military_spawnpoint";
		if ( gEntList.FindEntityByClassname( NULL, "zms_spawnpoint" ) != NULL )
			pSpawnpointName = "zms_spawnpoint";
		if ( gEntList.FindEntityByClassname( NULL, "so_spawnpoint" ) != NULL )
			pSpawnpointName = "so_spawnpoint";
	}

	// Call for the last spawnpoint we used
	pSpot = pLastSpawnPoint;

	// Randomize our spawnpoint
	for ( int i = random->RandomInt(1,5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	// Skip over the last spawnpoint we used
	if ( !pSpot )
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	CBaseEntity *pFirstSpot = pSpot;

	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetLocalOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet, so spawn anywhere
	if ( pSpot )
	{
		goto ReturnSpot;
	}

	if ( !pSpot  )
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_start" );

		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:

	// Record last spawnpoint used
	g_pLastSpawn = pSpot;

	m_flSlamProtectTime = gpGlobals->curtime + 0.5;

	return pSpot;
}

CON_COMMAND( timeleft, "prints the time remaining in the match" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );

	int iTimeRemaining = (int)HL2MPRules()->GetMapRemainingTime();
    
	if ( iTimeRemaining == 0 )
	{
		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "This game has no timelimit." );
		}
		else
		{
			Msg( "* No Time Limit *\n" );
		}
	}
	else
	{
		int iMinutes, iSeconds;
		iMinutes = iTimeRemaining / 60;
		iSeconds = iTimeRemaining % 60;

		char minutes[8];
		char seconds[8];

		Q_snprintf( minutes, sizeof(minutes), "%d", iMinutes );
		Q_snprintf( seconds, sizeof(seconds), "%2.2d", iSeconds );

		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "Time left in map: %s1:%s2", minutes, seconds );
		}

		// S:O - Clients like to spam this to piss people off, so let's prevent that from happening by disabling it.
		/*else
		{
			Msg( "Time Remaining:  %s:%s\n", minutes, seconds );
		}*/
	}	
}


void CHL2MP_Player::Reset()
{	
	ResetDeathCount();
	ResetFragCount();
}

bool CHL2MP_Player::IsReady()
{
	return m_bReady;
}

void CHL2MP_Player::SetReady( bool bReady )
{
	m_bReady = bReady;
}

void CHL2MP_Player::CheckChatText( char *p, int bufsize )
{
	//Look for escape sequences and replace

	char *buf = new char[bufsize];
	int pos = 0;

	// Parse say text for escape sequences
	for ( char *pSrc = p; pSrc != NULL && *pSrc != 0 && pos < bufsize-1; pSrc++ )
	{
		// copy each char across
		buf[pos] = *pSrc;
		pos++;
	}

	buf[pos] = '\0';

	// copy buf back into p
	Q_strncpy( p, buf, bufsize );

	delete[] buf;	

	const char *pReadyCheck = p;

	HL2MPRules()->CheckChatForReadySignal( this, pReadyCheck );
}

void CHL2MP_Player::State_Transition( HL2MPPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}

void CHL2MP_Player::State_Enter( HL2MPPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}

void CHL2MP_Player::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CHL2MP_Player::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CHL2MPPlayerStateInfo *CHL2MP_Player::State_LookupInfo( HL2MPPlayerState state )
{
	// This table MUST match the 
	static CHL2MPPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CHL2MP_Player::State_Enter_ACTIVE, NULL, &CHL2MP_Player::State_PreThink_ACTIVE },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CHL2MP_Player::State_Enter_OBSERVER_MODE,	NULL, &CHL2MP_Player::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

bool CHL2MP_Player::StartObserverMode(int mode)
{
	// we only want to go into observer mode if the player asked to, not on a death timeout
	if ( m_bEnterObserver == true )
	{
		VPhysicsDestroyObject();
		return BaseClass::StartObserverMode( mode );
	}
	return false;
}

void CHL2MP_Player::StopObserverMode()
{
	m_bEnterObserver = false;
	BaseClass::StopObserverMode();
}

void CHL2MP_Player::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;
	if ( IsNetClient() )
	{
		const char *pIdealMode = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_spec_mode" );
		if ( pIdealMode )
		{
			observerMode = atoi( pIdealMode );
			if ( observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING )
			{
				observerMode = m_iObserverLastMode;
			}
		}
	}
	m_bEnterObserver = true;
	StartObserverMode( observerMode );
}

void CHL2MP_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	//Assert( pl.deadflag );
}


void CHL2MP_Player::State_Enter_ACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	
	// md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
	// is false, then they'll be spectating but blocking live players from moving.
	// RemoveSolidFlags( FSOLID_NOT_SOLID );
	
	m_Local.m_iHideHUD = 0;
}


void CHL2MP_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2MP_Player::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return false;

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CHL2MP_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}
// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
					{
					}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 )
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );

	//Tony; use prediction rules.
	filter.UsePredictionRules();
	
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}


void CHL2MP_Player::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: Override setup bones so that is uses the render angles from
//			the HL2MP animation state to setup the hitboxes.
//-----------------------------------------------------------------------------
void CHL2MP_Player::SetupBones( matrix3x4_t *pBoneToWorld, int boneMask )
{
	VPROF_BUDGET( "CHL2MP_Player::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM );

	// Set the mdl cache semaphore.
	MDLCACHE_CRITICAL_SECTION();

	// Get the studio header.
	Assert( GetModelPtr() );
	CStudioHdr *pStudioHdr = GetModelPtr( );

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	// Adjust hit boxes based on IK driven offset.
	Vector adjOrigin = GetAbsOrigin() + Vector( 0, 0, m_flEstIkOffset );

	// FIXME: pass this into Studio_BuildMatrices to skip transforms
	CBoneBitList boneComputed;
	if ( m_pIk )
	{
		m_iIKCounter++;
		m_pIk->Init( pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask );
		GetSkeleton( pStudioHdr, pos, q, boneMask );

		m_pIk->UpdateTargets( pos, q, pBoneToWorld, boneComputed );
		CalculateIKLocks( gpGlobals->curtime );
		m_pIk->SolveDependencies( pos, q, pBoneToWorld, boneComputed );
	}
	else
	{
		GetSkeleton( pStudioHdr, pos, q, boneMask );
	}

	CBaseAnimating *pParent = dynamic_cast< CBaseAnimating* >( GetMoveParent() );
	if ( pParent )
	{
		// We're doing bone merging, so do special stuff here.
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if ( pParentCache )
		{
			BuildMatricesWithBoneMerge( 
				pStudioHdr, 
				m_PlayerAnimState->GetRenderAngles(),
				adjOrigin, 
				pos, 
				q, 
				pBoneToWorld, 
				pParent, 
				pParentCache );

			return;
		}
	}

	Studio_BuildMatrices( 
		pStudioHdr, 
		m_PlayerAnimState->GetRenderAngles(),
		adjOrigin, 
		pos, 
		q, 
		-1,
		pBoneToWorld,
		boneMask );
}