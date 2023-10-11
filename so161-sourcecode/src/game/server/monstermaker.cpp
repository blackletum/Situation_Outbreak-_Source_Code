//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that creates NPCs in the game. There are two types of NPC
//			makers -- one which creates NPCs using a template NPC, and one which
//			creates an NPC via a classname.
//
//=============================================================================//

#include "cbase.h"
#include "datacache/imdlcache.h"
#include "entityapi.h"
#include "entityoutput.h"
#include "ai_basenpc.h"
#include "monstermaker.h"
#include "TemplateEntities.h"
#include "ndebugoverlay.h"
#include "mapentities.h"
#include "IEffects.h"
#include "props.h"

// S:O - Additional Includes
#include "hl2mp/hl2mp_player.h"
#include "team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void DispatchActivate( CBaseEntity *pEntity )
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
	pEntity->Activate();
	mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
}

ConVar ai_inhibit_spawners( "ai_inhibit_spawners", "0", FCVAR_CHEAT );

// S:O - Additional ConVars
ConVar so_max_zombiecount( "so_max_zombiecount", "50", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines approximately how many zombie NPCs are allowed to be alive on a map at any given time." );

LINK_ENTITY_TO_CLASS( info_npc_spawn_destination, CNPCSpawnDestination );

BEGIN_DATADESC( CNPCSpawnDestination )
	DEFINE_KEYFIELD( m_ReuseDelay, FIELD_FLOAT, "ReuseDelay" ),
	DEFINE_KEYFIELD( m_RenameNPC,FIELD_STRING, "RenameNPC" ),
	DEFINE_FIELD( m_TimeNextAvailable, FIELD_TIME ),

	DEFINE_OUTPUT( m_OnSpawnNPC,	"OnSpawnNPC" ),
END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
CNPCSpawnDestination::CNPCSpawnDestination()
{
	// Available right away, the first time.
	m_TimeNextAvailable = gpGlobals->curtime;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPCSpawnDestination::IsAvailable()
{
	if( m_TimeNextAvailable > gpGlobals->curtime )
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPCSpawnDestination::OnSpawnedNPC( CAI_BaseNPC *pNPC )
{
	// Rename the NPC
	if( m_RenameNPC != NULL_STRING )
	{
		pNPC->SetName( m_RenameNPC );
	}

	m_OnSpawnNPC.FireOutput( pNPC, this );
	m_TimeNextAvailable = gpGlobals->curtime + m_ReuseDelay;
}

//-------------------------------------
BEGIN_DATADESC( CBaseNPCMaker )

	DEFINE_KEYFIELD( m_nMaxNumNPCs,			FIELD_INTEGER,	"MaxNPCCount" ),
	DEFINE_KEYFIELD( m_nMaxLiveChildren,		FIELD_INTEGER,	"MaxLiveChildren" ),
	DEFINE_KEYFIELD( m_flSpawnFrequency,		FIELD_FLOAT,	"SpawnFrequency" ),
	DEFINE_KEYFIELD( m_bDisabled,			FIELD_BOOLEAN,	"StartDisabled" ),

	// S:O - Rallypoint
	DEFINE_KEYFIELD( rallypointName,	FIELD_STRING, "rallypointName" ),

	DEFINE_FIELD(	m_nLiveChildren,		FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,	"Spawn",	InputSpawnNPC ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Enable",	InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Disable",	InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Toggle",	InputToggle ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxChildren", InputSetMaxChildren ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddMaxChildren", InputAddMaxChildren ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxLiveChildren", InputSetMaxLiveChildren ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	 "SetSpawnFrequency", InputSetSpawnFrequency ),

	// Outputs
	DEFINE_OUTPUT( m_OnAllSpawned,		"OnAllSpawned" ),
	DEFINE_OUTPUT( m_OnAllSpawnedDead,	"OnAllSpawnedDead" ),
	DEFINE_OUTPUT( m_OnAllLiveChildrenDead,	"OnAllLiveChildrenDead" ),
	DEFINE_OUTPUT( m_OnSpawnNPC,		"OnSpawnNPC" ),

	// Function Pointers
	DEFINE_THINKFUNC( MakerThink ),

	DEFINE_FIELD( m_hIgnoreEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszIngoreEnt, FIELD_STRING, "IgnoreEntity" ), 
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_nLiveChildren	= 0;
	Precache();

	// S:O - By default, always force generated NPCs to fade, regardless of flags.
	m_spawnflags |= SF_NPCMAKER_FADE;

	// S:O - Check to see whether or not this spawner has a rallypoint set
	CBaseEntity *pRallyPoint = gEntList.FindEntityByName( NULL, rallypointName, this );
	pRallyEnt = dynamic_cast<CNPCRallyPoint*>( pRallyPoint );
	if ( pRallyPoint && pRallyEnt )
	{
		pRallyEnt->SetOwnerEntity( this );
		pRallyEnt->SetSpawnParent( entindex() );
		pRallyEnt->ActivateRallyPoint();
	}

	// Do we start enabled?
	if ( m_bDisabled == false )
	{
		SetThink ( &CBaseNPCMaker::MakerThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		SetThink ( &CBaseNPCMaker::SUB_DoNothing );
	}
}

//-----------------------------------------------------------------------------
// A not-very-robust check to see if a human hull could fit at this location.
// used to validate spawn destinations.
//-----------------------------------------------------------------------------
bool CBaseNPCMaker::HumanHullFits( const Vector &vecLocation )
{
	trace_t tr;
	UTIL_TraceHull( vecLocation,
					vecLocation + Vector( 0, 0, 1 ),
					NAI_Hull::Mins(HULL_HUMAN),
					NAI_Hull::Maxs(HULL_HUMAN),
					MASK_NPCSOLID,
					m_hIgnoreEntity,
					COLLISION_GROUP_NONE,
					&tr );

	if( tr.fraction == 1.0 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not it is OK to make an NPC at this instant.
//-----------------------------------------------------------------------------
bool CBaseNPCMaker::CanMakeNPC( bool bIgnoreSolidEntities )
{
	if( ai_inhibit_spawners.GetBool() )
		return false;

	if ( m_nMaxLiveChildren > 0 && m_nLiveChildren >= m_nMaxLiveChildren )
	{// not allowed to make a new one yet. Too many live ones out right now.
		return false;
	}

	if ( m_iszIngoreEnt != NULL_STRING )
	{
		m_hIgnoreEntity = gEntList.FindEntityByName( NULL, m_iszIngoreEnt );
	}

	Vector mins = GetAbsOrigin() - Vector( 34, 34, 0 );
	Vector maxs = GetAbsOrigin() + Vector( 34, 34, 0 );
	maxs.z = GetAbsOrigin().z;
	
	// If we care about not hitting solid entities, look for 'em
	if ( !bIgnoreSolidEntities )
	{
		CBaseEntity *pList[128];

		int count = UTIL_EntitiesInBox( pList, 128, mins, maxs, FL_CLIENT|FL_NPC );
		if ( count )
		{
			//Iterate through the list and check the results
			for ( int i = 0; i < count; i++ )
			{
				//Don't build on top of another entity
				if ( pList[i] == NULL )
					continue;

				//If one of the entities is solid, then we may not be able to spawn now
				if ( ( pList[i]->GetSolidFlags() & FSOLID_NOT_SOLID ) == false )
				{
					// Since the outer method doesn't work well around striders on account of their huge bounding box.
					// Find the ground under me and see if a human hull would fit there.
					trace_t tr;
					UTIL_TraceHull( GetAbsOrigin() + Vector( 0, 0, 2 ),
									GetAbsOrigin() - Vector( 0, 0, 8192 ),
									NAI_Hull::Mins(HULL_HUMAN),
									NAI_Hull::Maxs(HULL_HUMAN),
									MASK_NPCSOLID,
									m_hIgnoreEntity,
									COLLISION_GROUP_NONE,
									&tr );

					if( !HumanHullFits( tr.endpos + Vector( 0, 0, 1 ) ) )
					{
						return false;
					}
				}
			}
		}
	}

	// Do we need to check to see if the player's looking?
	if ( HasSpawnFlags( SF_NPCMAKER_HIDEFROMPLAYER ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
			if ( pPlayer )
			{
				// Only spawn if the player's looking away from me
				if( pPlayer->FInViewCone( GetAbsOrigin() ) && pPlayer->FVisible( GetAbsOrigin() ) )
				{
					if ( !(pPlayer->GetFlags() & FL_NOTARGET) )
						return false;
					DevMsg( 2, "Spawner %s spawning even though seen due to notarget\n", STRING( GetEntityName() ) );
				}
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: If this had a finite number of children, return true if they've all
//			been created.
//-----------------------------------------------------------------------------
bool CBaseNPCMaker::IsDepleted()
{
	if ( (m_spawnflags & SF_NPCMAKER_INF_CHILD) || m_nMaxNumNPCs > 0 )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Toggle the spawner's state
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Toggle( void )
{
	if ( m_bDisabled )
	{
		Enable();
	}
	else
	{
		Disable();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Start the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Enable( void )
{
	// can't be enabled once depleted
	if ( IsDepleted() )
		return;

	m_bDisabled = false;
	SetThink ( &CBaseNPCMaker::MakerThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Stop the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Disable( void )
{
	m_bDisabled = true;
	SetThink( &CBaseNPCMaker::MakerThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that spawns an NPC.
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputSpawnNPC( inputdata_t &inputdata )
{
	if( !IsDepleted() )
	{
		MakeNPC();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that starts the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputEnable( inputdata_t &inputdata )
{
	Enable();
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that stops the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputDisable( inputdata_t &inputdata )
{
	Disable();
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that toggles the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputSetMaxChildren( inputdata_t &inputdata )
{
	m_nMaxNumNPCs = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputAddMaxChildren( inputdata_t &inputdata )
{
	m_nMaxNumNPCs += inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputSetMaxLiveChildren( inputdata_t &inputdata )
{
	m_nMaxLiveChildren = inputdata.value.Int();
}

void CBaseNPCMaker::InputSetSpawnFrequency( inputdata_t &inputdata )
{
	m_flSpawnFrequency = inputdata.value.Float();
}

LINK_ENTITY_TO_CLASS( npc_maker, CNPCMaker );

BEGIN_DATADESC( CNPCMaker )

	DEFINE_KEYFIELD( m_iszNPCClassname,		FIELD_STRING,	"NPCType" ),
	DEFINE_KEYFIELD( m_ChildTargetName,		FIELD_STRING,	"NPCTargetname" ),
	DEFINE_KEYFIELD( m_SquadName,			FIELD_STRING,	"NPCSquadName" ),
	DEFINE_KEYFIELD( m_spawnEquipment,		FIELD_STRING,	"additionalequipment" ),
	DEFINE_KEYFIELD( m_strHintGroup,		FIELD_STRING,	"NPCHintGroup" ),
	DEFINE_KEYFIELD( m_RelationshipString,	FIELD_STRING,	"Relationship" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPCMaker::CNPCMaker( void )
{
	m_spawnEquipment = NULL_STRING;
}


//-----------------------------------------------------------------------------
// Purpose: Precache the target NPC
//-----------------------------------------------------------------------------
void CNPCMaker::Precache( void )
{
	BaseClass::Precache();

	const char *pszNPCName = STRING( m_iszNPCClassname );
	if ( !pszNPCName || !pszNPCName[0] )
	{
		Warning("npc_maker %s has no specified NPC-to-spawn classname.\n", STRING(GetEntityName()) );
	}
	else
	{
		UTIL_PrecacheOther( pszNPCName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates the NPC.
//-----------------------------------------------------------------------------
void CNPCMaker::MakeNPC( void )
{
	if (!CanMakeNPC())
		return;

	CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName( STRING(m_iszNPCClassname) );

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	// ------------------------------------------------
	//  Intialize spawned NPC's relationships
	// ------------------------------------------------
	pent->SetRelationshipString( m_RelationshipString );

	m_OnSpawnNPC.Set( pent, pent, this );

	pent->SetAbsOrigin( GetAbsOrigin() );

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles( angles );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	if ( m_spawnflags & SF_NPCMAKER_FADE )
	{
		pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

	pent->m_spawnEquipment	= m_spawnEquipment;
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// S:O - so_zombiespawner
// Spawns the specified classname of zombie NPC every x seconds.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( zms_zombiespawner, CZombieSpawner );
LINK_ENTITY_TO_CLASS( so_zombiespawner, CZombieSpawner );

BEGIN_DATADESC( CZombieSpawner )

	DEFINE_KEYFIELD( m_iszNPCClassname,		FIELD_STRING,	"NPCType" ),
	DEFINE_KEYFIELD( m_bDynDiff,			FIELD_BOOLEAN,	"UseDynDiff" ),
	DEFINE_KEYFIELD( m_DynDiffDetails,		FIELD_STRING,	"DynDiffDetails" ),
	DEFINE_KEYFIELD( m_ChildTargetName,		FIELD_STRING,	"NPCTargetname" ),
	DEFINE_KEYFIELD( m_SquadName,			FIELD_STRING,	"NPCSquadName" ),
	DEFINE_KEYFIELD( m_strHintGroup,		FIELD_STRING,	"NPCHintGroup" ),

END_DATADESC()

CZombieSpawner::CZombieSpawner( void )
{
}

void CZombieSpawner::Spawn( void )
{
	BaseClass::Spawn();
}

void CZombieSpawner::Precache( void )
{
	BaseClass::Precache();

	const char *pszNPCName = STRING( m_iszNPCClassname );
	if ( !pszNPCName || !pszNPCName[0] )
	{
		Warning( "Failed to precache zombie from so_zombiespawner [%s]!\n", STRING(GetEntityName()) );
	}
	else
	{
		UTIL_PrecacheOther( pszNPCName );
	}
}

void CZombieSpawner::MakeNPC( void )
{
	if ( gEntList.m_ZombieList.Count() >= so_max_zombiecount.GetInt() )
		return;

	if ( !CanMakeNPC() )
		return;

	if ( m_bDynDiff )
	{
		const char *pDynDiffDetails = STRING( m_DynDiffDetails );

		// Get the Survivor team's score (total kills).
		iKills = GetGlobalTeam( 3 )->GetScore();

		// If we didn't detect any kills at all, artificially generate a kill to prevent any problems with our check below.
		if ( iKills == 0 )
		{
			iKills = iKills + 1;
		}

		// Compare number of kills to the zombie spawner's defined restrictions.
		// If the kill count doesn't match or exceed restricton, DO NOT SPAWN A ZOMBIE!
		if ( FStrEq( pDynDiffDetails, "50 Kills" ) )
		{
			if ( iKills < 50 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "100 Kills" ) )
		{
			if ( iKills < 100 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "150 Kills" ) )
		{
			if ( iKills < 150 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "200 Kills" ) )
		{
			if ( iKills < 200 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "250 Kills" ) )
		{
			if ( iKills < 250 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "Random" ) )
		{
			int randdyndiffbasic1 = RandomInt( 1, 2 );
			switch ( randdyndiffbasic1 )
			{
				case 1:
					// Randomly chosen not to spawn this zombie.
					return;
				break;

				case 2:
					// Randomly chosen to spawn our zombie, so do nothing here.
				break;
			}
		}
	}

	CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName( STRING(m_iszNPCClassname) );

	if ( !pent )
	{
		Warning( "so_zombiespawner [%s] attempted to spawn an invalid zombie NPC!\n", STRING(GetEntityName()) );
		return;
	}

	m_OnSpawnNPC.Set( pent, pent, this );

	pent->SetAbsOrigin( GetAbsOrigin() );

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles( angles );

	// Force all zombie NPCs to fall to the ground when they spawn and fade away when they die.
	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );

	// Apply defined squads and hint groups.
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// S:O - so_zombiespawner_random
// Spawns a random class of zombie NPC every x seconds.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( zms_zombiespawner_random, CZombieSpawnerRandom );
LINK_ENTITY_TO_CLASS( so_zombiespawner_random, CZombieSpawnerRandom );

BEGIN_DATADESC( CZombieSpawnerRandom )

	DEFINE_KEYFIELD( m_bIncludeSploderZombie,	FIELD_BOOLEAN,	"IncludeSploderZombie" ),
	DEFINE_KEYFIELD( m_bDynDiff,				FIELD_BOOLEAN,	"UseDynDiff" ),
	DEFINE_KEYFIELD( m_DynDiffDetails,			FIELD_STRING,	"DynDiffDetails" ),
	DEFINE_KEYFIELD( m_ChildTargetName,			FIELD_STRING,	"NPCTargetname" ),
	DEFINE_KEYFIELD( m_SquadName,				FIELD_STRING,	"NPCSquadName" ),
	DEFINE_KEYFIELD( m_strHintGroup,			FIELD_STRING,	"NPCHintGroup" ),

END_DATADESC()

CZombieSpawnerRandom::CZombieSpawnerRandom( void )
{
}

void CZombieSpawnerRandom::Spawn( void )
{
	BaseClass::Spawn();
}

void CZombieSpawnerRandom::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_zombie" );
	UTIL_PrecacheOther( "npc_poisonzombie" );
	UTIL_PrecacheOther( "npc_fastzombie" );
	UTIL_PrecacheOther( "npc_reaper_nojump" );
	UTIL_PrecacheOther( "npc_sploderzombie" );
}

void CZombieSpawnerRandom::MakeNPC( void )
{
	if ( gEntList.m_ZombieList.Count() >= so_max_zombiecount.GetInt() )
		return;

	if ( !CanMakeNPC() )
		return;

	const char * m_iszNPCClassname = "npc_zombie";

	if ( m_bIncludeSploderZombie )
	{
		int randzombienpcspawner = RandomInt(1,5);
		switch ( randzombienpcspawner )
		{
			case 1:
				m_iszNPCClassname = "npc_zombie";
				break;

			case 2:
				m_iszNPCClassname = "npc_fastzombie";
				break;

			case 3:
				m_iszNPCClassname = "npc_poisonzombie";
				break;

			case 4:
				m_iszNPCClassname = "npc_reaper_nojump";
				break;

			// 'Dem sploders are CRAZY.
			// CRAZY I TELLS YA!!!!!!!
			case 5:
				m_iszNPCClassname = "npc_sploderzombie";
				break;
		}
	}
	else
	{
		int randzombienpcspawner = RandomInt(1,4);
		switch ( randzombienpcspawner )
		{
			case 1:
				m_iszNPCClassname = "npc_zombie";
				break;

			case 2:
				m_iszNPCClassname = "npc_fastzombie";
				break;

			case 3:
				m_iszNPCClassname = "npc_poisonzombie";
				break;

			case 4:
				m_iszNPCClassname = "npc_reaper_nojump";
				break;
		}
	}

	if ( m_bDynDiff )
	{
		const char *pDynDiffDetails = STRING( m_DynDiffDetails );

		// Get the Survivor team's score (total kills).
		iKills = GetGlobalTeam( 3 )->GetScore();

		// If we didn't detect any kills at all, artificially generate a kill to prevent any problems with our check below.
		if ( iKills == 0 )
		{
			iKills = iKills + 1;
		}

		// Compare number of kills to the zombie spawner's defined restrictions.
		// If the kill count doesn't match or exceed restricton, DO NOT SPAWN A ZOMBIE!
		// As of now we only compare for up to 250 kills because of the limitations of using int, though we could go on and on with a float or something.
		if ( FStrEq( pDynDiffDetails, "50 Kills" ) )
		{
			if ( iKills < 50 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "100 Kills" ) )
		{
			if ( iKills < 100 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "150 Kills" ) )
		{
			if ( iKills < 150 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "200 Kills" ) )
		{
			if ( iKills < 200 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "250 Kills" ) )
		{
			if ( iKills < 250 )
				return;
		}
		else if ( FStrEq( pDynDiffDetails, "Random" ) )
		{
			int randdyndiffbasic1 = RandomInt( 1, 2 );
			switch ( randdyndiffbasic1 )
			{
				case 1:
					// Randomly chosen not to spawn this zombie.
					return;
				break;

				case 2:
					// Randomly chosen to spawn our zombie, so do nothing here.
				break;
			}
		}
	}

	CAI_BaseNPC *pent = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName( m_iszNPCClassname ) );

	if ( !pent )
	{
		Warning( "so_zombiespawner_random [%s] attempted to spawn an invalid zombie NPC!\n", STRING(GetEntityName()) );
		return;
	}

	m_OnSpawnNPC.Set( pent, pent, this );

	pent->SetAbsOrigin( GetAbsOrigin() );

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles( angles );

	// Force all zombie NPCs to fall to the ground when they spawn and fade away when they die.
	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );

	// Apply defined squads and hint groups.
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// S:O - so_survivorspawner
// Spawns a survivor NPC every x seconds.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( zms_survivorspawner, CSurvivorSpawner );
LINK_ENTITY_TO_CLASS( so_survivorspawner, CSurvivorSpawner );

BEGIN_DATADESC( CSurvivorSpawner )

	DEFINE_KEYFIELD( m_spawnEquipment,		FIELD_STRING,	"additionalequipment" ),
	DEFINE_KEYFIELD( m_ChildTargetName,		FIELD_STRING,	"NPCTargetname" ),
	DEFINE_KEYFIELD( m_SquadName,			FIELD_STRING,	"NPCSquadName" ),
	DEFINE_KEYFIELD( m_strHintGroup,		FIELD_STRING,	"NPCHintGroup" ),

END_DATADESC()

CSurvivorSpawner::CSurvivorSpawner( void )
{
	m_spawnEquipment = NULL_STRING;
}

void CSurvivorSpawner::Spawn( void )
{
	BaseClass::Spawn();
}

void CSurvivorSpawner::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_citizen" );
}

void CSurvivorSpawner::MakeNPC( void )
{
	if ( !CanMakeNPC() )
		return;

	const char * m_iszNPCClassname = "npc_citizen";

	CAI_BaseNPC *pent = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName( m_iszNPCClassname ) );

	if ( !pent )
	{
		Warning( "so_survivorspawner [%s] attempted to spawn an invalid survivor NPC!\n", STRING(GetEntityName()) );
		return;
	}

	m_OnSpawnNPC.Set( pent, pent, this );

	pent->SetAbsOrigin( GetAbsOrigin() );

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles( angles );

	// Force all survivor NPCs to fall to the ground when they spawn and fade away when they die.
	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );

	// Spawn any equipment (weapons) the mapper requested.
	pent->m_spawnEquipment = m_spawnEquipment;

	// Apply defined squads and hint groups.
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// S:O - so_survivorspawner
// Spawns a survivor NPC every x seconds.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( zms_militaryspawner, CMilitarySpawner );
LINK_ENTITY_TO_CLASS( so_militaryspawner, CMilitarySpawner );

BEGIN_DATADESC( CMilitarySpawner )

	DEFINE_KEYFIELD( m_spawnEquipment,		FIELD_STRING,	"additionalequipment" ),
	DEFINE_KEYFIELD( m_ChildTargetName,		FIELD_STRING,	"NPCTargetname" ),
	DEFINE_KEYFIELD( m_SquadName,			FIELD_STRING,	"NPCSquadName" ),
	DEFINE_KEYFIELD( m_strHintGroup,		FIELD_STRING,	"NPCHintGroup" ),

END_DATADESC()

CMilitarySpawner::CMilitarySpawner( void )
{
	m_spawnEquipment = NULL_STRING;
}

void CMilitarySpawner::Spawn( void )
{
	BaseClass::Spawn();
}

void CMilitarySpawner::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_combine_s" );
}

void CMilitarySpawner::MakeNPC( void )
{
	if ( !CanMakeNPC() )
		return;

	const char * m_iszNPCClassname = "npc_combine_s";

	CAI_BaseNPC *pent = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName( m_iszNPCClassname ) );

	if ( !pent )
	{
		Warning( "so_militaryspawner [%s] attempted to spawn an invalid survivor NPC!\n", STRING(GetEntityName()) );
		return;
	}

	m_OnSpawnNPC.Set( pent, pent, this );

	pent->SetAbsOrigin( GetAbsOrigin() );

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles( angles );

	// Force all survivor NPCs to fall to the ground when they spawn and fade away when they die.
	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );

	// Spawn any equipment (weapons) the mapper requested.
	pent->m_spawnEquipment = m_spawnEquipment;

	// Apply defined squads and hint groups.
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// S:O - so_weaponspawner_random
// Spawns a random weapon at the start of each round.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( zms_weaponspawner_random, CZMSRandWeapon );
LINK_ENTITY_TO_CLASS( so_weaponspawner_random, CZMSRandWeapon );

BEGIN_DATADESC( CZMSRandWeapon )
	DEFINE_KEYFIELD( m_bMeleeOnly, FIELD_BOOLEAN, "MeleeOnlyWeaps" ),
END_DATADESC()

CZMSRandWeapon::CZMSRandWeapon( void )
{
}

void CZMSRandWeapon::Precache( void )
{
	BaseClass::Precache();
}

void CZMSRandWeapon::Spawn( void )
{
	BaseClass::Spawn();

	const char * m_iszWeaponClassname = "NULL";

	if ( m_bMeleeOnly == false )
	{
		int randweaponspawner = RandomInt( 1, 11 );
		switch ( randweaponspawner )
		{
			case 1:
				m_iszWeaponClassname = "weapon_9mm";
				break;

			case 2:
				m_iszWeaponClassname = "weapon_deagle";
				break;

			case 3:
				m_iszWeaponClassname = "weapon_mp5k";
				break;

			case 4:
				m_iszWeaponClassname = "weapon_m4";
				break;

			case 5:
				m_iszWeaponClassname = "weapon_ak47";
				break;

			case 6:
				m_iszWeaponClassname = "weapon_mac10";
				break;

			case 7:
				m_iszWeaponClassname = "weapon_rpg";
				break;

			case 8:
				m_iszWeaponClassname = "weapon_sniper";
				break;

			case 9:
				m_iszWeaponClassname = "weapon_frag";
				break;

			case 10:
				m_iszWeaponClassname = "weapon_incendiary";
				break;

			case 11:
				m_iszWeaponClassname = "weapon_doublebarrel";
				break;
		}
	}
	else if ( m_bMeleeOnly == true )
	{
		int randweaponspawner2 = RandomInt( 1, 5 );
		switch ( randweaponspawner2 )
		{
			case 1:
				m_iszWeaponClassname = "weapon_brokenbottle";
				break;

			case 2:
				m_iszWeaponClassname = "weapon_cleaver";
				break;

			case 3:
				m_iszWeaponClassname = "weapon_axe";
				break;

			case 4:
				m_iszWeaponClassname = "weapon_katana";
				break;

			case 5:
				m_iszWeaponClassname = "weapon_chainsaw";
				break;
		}
	}
	else
	{
		// We should never get to this point, but if we do...
		m_iszWeaponClassname = "NULL";
	}

	CBaseCombatWeapon *weap = dynamic_cast< CBaseCombatWeapon * >( CreateEntityByName( m_iszWeaponClassname ) );

	if ( weap )
	{
		weap->SetAbsOrigin( GetAbsOrigin() );
		DispatchSpawn( weap );
		DispatchActivate( weap );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pChild - 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::ChildPostSpawn( CAI_BaseNPC *pChild )
{
	// If I'm stuck inside any props, remove them
	bool bFound = true;
	while ( bFound )
	{
		trace_t tr;
		UTIL_TraceHull( pChild->GetAbsOrigin(), pChild->GetAbsOrigin(), pChild->WorldAlignMins(), pChild->WorldAlignMaxs(), MASK_NPCSOLID, pChild, COLLISION_GROUP_NONE, &tr );
		//NDebugOverlay::Box( pChild->GetAbsOrigin(), pChild->WorldAlignMins(), pChild->WorldAlignMaxs(), 0, 255, 0, 32, 5.0 );
		if ( tr.fraction != 1.0 && tr.m_pEnt )
		{
			if ( FClassnameIs( tr.m_pEnt, "prop_physics" ) )
			{
				// Set to non-solid so this loop doesn't keep finding it
				tr.m_pEnt->AddSolidFlags( FSOLID_NOT_SOLID );
				UTIL_RemoveImmediate( tr.m_pEnt );
				continue;
			}
		}

		bFound = false;
	}
	if ( m_hIgnoreEntity != NULL )
	{
		pChild->SetOwnerEntity( m_hIgnoreEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates a new NPC every so often.
//-----------------------------------------------------------------------------
void CBaseNPCMaker::MakerThink ( void )
{
	if ( m_bDisabled == false )
	{
		SetNextThink( gpGlobals->curtime + m_flSpawnFrequency );
		MakeNPC();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::DeathNotice( CBaseEntity *pVictim )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_nLiveChildren--;

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg( m_nLiveChildren >= 0, "npc_maker receiving child death notice but thinks has no children\n" );

	if ( m_nLiveChildren <= 0 )
	{
		m_OnAllLiveChildrenDead.FireOutput( this, this );

		// See if we've exhausted our supply of NPCs
		if ( ( (m_spawnflags & SF_NPCMAKER_INF_CHILD) == false ) && IsDepleted() )
		{
			// Signal that all our children have been spawned and are now dead
			m_OnAllSpawnedDead.FireOutput( this, this );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates new NPCs from a template NPC. The template NPC must be marked
//			as a template (spawnflag) and does not spawn.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( npc_template_maker, CTemplateNPCMaker );

BEGIN_DATADESC( CTemplateNPCMaker )

	DEFINE_KEYFIELD( m_iszTemplateName, FIELD_STRING, "TemplateName" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_FIELD( m_iszTemplateData, FIELD_STRING ),
	DEFINE_KEYFIELD( m_iszDestinationGroup, FIELD_STRING, "DestinationGroup" ),
	DEFINE_KEYFIELD( m_CriterionVisibility, FIELD_INTEGER, "CriterionVisibility" ),
	DEFINE_KEYFIELD( m_CriterionDistance, FIELD_INTEGER, "CriterionDistance" ),
	DEFINE_KEYFIELD( m_iMinSpawnDistance, FIELD_INTEGER, "MinSpawnDistance" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnNPCInRadius", InputSpawnInRadius ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnNPCInLine", InputSpawnInLine ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SpawnMultiple", InputSpawnMultiple ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeDestinationGroup", InputChangeDestinationGroup ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMinimumSpawnDistance", InputSetMinimumSpawnDistance ),

END_DATADESC()


//-----------------------------------------------------------------------------
// A hook that lets derived NPC makers do special stuff when precaching.
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::PrecacheTemplateEntity( CBaseEntity *pEntity )
{
	pEntity->Precache();
}


void CTemplateNPCMaker::Precache()
{
	BaseClass::Precache();

	if ( !m_iszTemplateData )
	{
		//
		// This must be the first time we're activated, not a load from save game.
		// Look up the template in the template database.
		//
		if (!m_iszTemplateName)
		{
			Warning( "npc_template_maker %s has no template NPC!\n", STRING(GetEntityName()) );
			UTIL_Remove( this );
			return;
		}
		else
		{
			m_iszTemplateData = Templates_FindByTargetName(STRING(m_iszTemplateName));
			if ( m_iszTemplateData == NULL_STRING )
			{
				DevWarning( "npc_template_maker %s: template NPC %s not found!\n", STRING(GetEntityName()), STRING(m_iszTemplateName) );
				UTIL_Remove( this );
				return;
			}
		}
	}

	Assert( m_iszTemplateData != NULL_STRING );

	// If the mapper marked this as "preload", then instance the entity preache stuff and delete the entity
	//if ( !HasSpawnFlags(SF_NPCMAKER_NOPRELOADMODELS) )
	if ( m_iszTemplateData != NULL_STRING )
	{
		CBaseEntity *pEntity = NULL;
		MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
		if ( pEntity != NULL )
		{
			PrecacheTemplateEntity( pEntity );
			UTIL_RemoveImmediate( pEntity );
		}
	}
}

#define MAX_DESTINATION_ENTS	100
CNPCSpawnDestination *CTemplateNPCMaker::FindSpawnDestination()
{
	CNPCSpawnDestination *pDestinations[ MAX_DESTINATION_ENTS ];
	CBaseEntity *pEnt = NULL;
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	int	count = 0;

	if( !pPlayer )
	{
		return NULL;
	}

	// Collect all the qualifiying destination ents
	pEnt = gEntList.FindEntityByName( NULL, m_iszDestinationGroup );

	if( !pEnt )
	{
		DevWarning("Template NPC Spawner (%s) doesn't have any spawn destinations!\n", GetDebugName() );
		return NULL;
	}
	
	while( pEnt )
	{
		CNPCSpawnDestination *pDestination;

		pDestination = dynamic_cast <CNPCSpawnDestination*>(pEnt);

		if( pDestination && pDestination->IsAvailable() )
		{
			bool fValid = true;
			Vector vecTest = pDestination->GetAbsOrigin();
			pPlayer = UTIL_GetNearestPlayer(vecTest);

			if( m_CriterionVisibility != TS_YN_DONT_CARE )
			{
				// Right now View Cone check is omitted intentionally.
				Vector vecTopOfHull = NAI_Hull::Maxs( HULL_HUMAN );
				vecTopOfHull.x = 0;
				vecTopOfHull.y = 0;
				bool fVisible = (pPlayer->FVisible( vecTest ) || pPlayer->FVisible( vecTest + vecTopOfHull ) );

				if( m_CriterionVisibility == TS_YN_YES )
				{
					if( !fVisible )
						fValid = false;
				}
				else
				{
					if( fVisible )
					{
						if ( !(pPlayer->GetFlags() & FL_NOTARGET) )
							fValid = false;
						else
							DevMsg( 2, "Spawner %s spawning even though seen due to notarget\n", STRING( GetEntityName() ) );
					}
				}
			}

			if( fValid )
			{
				pDestinations[ count ] = pDestination;
				count++;
			}
		}

		pEnt = gEntList.FindEntityByName( pEnt, m_iszDestinationGroup );
	}

	if( count < 1 )
		return NULL;

	// Now find the nearest/farthest based on distance criterion
	if( m_CriterionDistance == TS_DIST_DONT_CARE )
	{
		// Pretty lame way to pick randomly. Try a few times to find a random
		// location where a hull can fit. Don't try too many times due to performance
		// concerns.
		for( int i = 0 ; i < 5 ; i++ )
		{
			CNPCSpawnDestination *pRandomDest = pDestinations[ rand() % count ];

			if( HumanHullFits( pRandomDest->GetAbsOrigin() ) )
			{
				return pRandomDest;
			}
		}

		return NULL;
	}
	else
	{
		if( m_CriterionDistance == TS_DIST_NEAREST )
		{
			float flNearest = FLT_MAX;
			CNPCSpawnDestination *pNearest = NULL;

			for( int i = 0 ; i < count ; i++ )
			{
				Vector vecTest = pDestinations[ i ]->GetAbsOrigin();
				pPlayer = UTIL_GetNearestPlayer(vecTest);
				float flDist = ( vecTest - pPlayer->GetAbsOrigin() ).Length();

				if ( m_iMinSpawnDistance != 0 && m_iMinSpawnDistance > flDist )
					continue;

				if( flDist < flNearest && HumanHullFits( vecTest ) )
				{
					flNearest = flDist;
					pNearest = pDestinations[ i ];
				}
			}

			return pNearest;
		}
		else
		{
			float flFarthest = 0;
			CNPCSpawnDestination *pFarthest = NULL;

			for( int i = 0 ; i < count ; i++ )
			{
				Vector vecTest = pDestinations[ i ]->GetAbsOrigin();
				float flDist = ( vecTest - pPlayer->GetAbsOrigin() ).Length();

				if ( m_iMinSpawnDistance != 0 && m_iMinSpawnDistance > flDist )
					continue;

				if( flDist > flFarthest && HumanHullFits( vecTest ) )
				{
					flFarthest = flDist;
					pFarthest = pDestinations[ i ];
				}
			}

			return pFarthest;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeNPC( void )
{
	// If we should be using the radius spawn method instead, do so
	if ( m_flRadius && HasSpawnFlags(SF_NPCMAKER_ALWAYSUSERADIUS) )
	{
		MakeNPCInRadius();
		return;
	}

	if (!CanMakeNPC( ( m_iszDestinationGroup != NULL_STRING ) ))
		return;

	CNPCSpawnDestination *pDestination = NULL;
	if ( m_iszDestinationGroup != NULL_STRING )
	{
		pDestination = FindSpawnDestination();
		if ( !pDestination )
		{
			DevMsg( 2, "%s '%s' failed to find a valid spawnpoint in destination group: '%s'\n", GetClassname(), STRING(GetEntityName()), STRING(m_iszDestinationGroup) );
			return;
		}
	}

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	if ( pDestination )
	{
		pent->SetAbsOrigin( pDestination->GetAbsOrigin() );

		// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
		QAngle angles = pDestination->GetAbsAngles();
		angles.x = 0.0;
		angles.z = 0.0;
		pent->SetAbsAngles( angles );

		pDestination->OnSpawnedNPC( pent );
	}
	else
	{
		pent->SetAbsOrigin( GetAbsOrigin() );

		// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
		QAngle angles = GetAbsAngles();
		angles.x = 0.0;
		angles.z = 0.0;
		pent->SetAbsAngles( angles );
	}

	m_OnSpawnNPC.Set( pEntity, pEntity, this );

	if ( m_spawnflags & SF_NPCMAKER_FADE )
	{
		pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

	pent->RemoveSpawnFlags( SF_NPC_TEMPLATE );

	if ( ( m_spawnflags & SF_NPCMAKER_NO_DROP ) == false )
	{
		pent->RemoveSpawnFlags( SF_NPC_FALL_TO_GROUND ); // don't fall, slam
	}

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeNPCInLine( void )
{
	if (!CanMakeNPC(true))
		return;

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	m_OnSpawnNPC.Set( pEntity, pEntity, this );

	PlaceNPCInLine( pent );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	pent->RemoveSpawnFlags( SF_NPC_TEMPLATE );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
bool CTemplateNPCMaker::PlaceNPCInLine( CAI_BaseNPC *pNPC )
{
	Vector vecPlace;
	Vector vecLine;

	GetVectors( &vecLine, NULL, NULL );

	// invert this, line up NPC's BEHIND the maker.
	vecLine *= -1;

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 8192 ), MASK_SHOT, pNPC, COLLISION_GROUP_NONE, &tr );
	vecPlace = tr.endpos;
	float flStepSize = pNPC->GetHullWidth();

	// Try 10 times to place this npc.
	for( int i = 0 ; i < 10 ; i++ )
	{
		UTIL_TraceHull( vecPlace,
						vecPlace + Vector( 0, 0, 10 ),
						pNPC->GetHullMins(),
						pNPC->GetHullMaxs(),
						MASK_SHOT,
						pNPC,
						COLLISION_GROUP_NONE,
						&tr );

		if( tr.fraction == 1.0 )
		{
			pNPC->SetAbsOrigin( tr.endpos );
			return true;
		}

		vecPlace += vecLine * flStepSize;
	}

	DevMsg("**Failed to place NPC in line!\n");
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Place NPC somewhere on the perimeter of my radius.
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeNPCInRadius( void )
{
	if ( !CanMakeNPC(true))
		return;

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	if ( !PlaceNPCInRadius( pent ) )
	{
		// Failed to place the NPC. Abort
		UTIL_RemoveImmediate( pent );
		return;
	}

	m_OnSpawnNPC.Set( pEntity, pEntity, this );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	pent->RemoveSpawnFlags( SF_NPC_TEMPLATE );

	// Allow the spawned NPC to use the optional rallypoint system.
	pent->SetNPCSpawnID( entindex() );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );

	pent->SetOwnerEntity( this );
	DispatchActivate( pent );

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find a place to spawn an npc within my radius.
//			Right now this function tries to place them on the perimeter of radius.
// Output : false if we couldn't find a spot!
//-----------------------------------------------------------------------------
bool CTemplateNPCMaker::PlaceNPCInRadius( CAI_BaseNPC *pNPC )
{
	Vector vPos;

	if ( CAI_BaseNPC::FindSpotForNPCInRadius( &vPos, GetAbsOrigin(), pNPC, m_flRadius ) )
	{
		pNPC->SetAbsOrigin( vPos );
		return true;
	}

	DevMsg("**Failed to place NPC in radius!\n");
	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeMultipleNPCS( int nNPCs )
{
	bool bInRadius = ( m_iszDestinationGroup == NULL_STRING && m_flRadius > 0.1 );
	while ( nNPCs-- )
	{
		if ( !bInRadius )
		{
			MakeNPC();
		}
		else
		{
			MakeNPCInRadius();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::InputSpawnMultiple( inputdata_t &inputdata )
{
	MakeMultipleNPCS( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::InputChangeDestinationGroup( inputdata_t &inputdata )
{
	m_iszDestinationGroup = inputdata.value.StringID();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::InputSetMinimumSpawnDistance( inputdata_t &inputdata )
{
	m_iMinSpawnDistance = inputdata.value.Int();
}

//--------------------------------------------------------
// CNPCRallyPoint (so_spawner_rallypoint)
//--------------------------------------------------------
// S:O - A rally point target entity for NPC spawners
// Makes all NPCs of a cetain NPC spawner gather at a specific location when they spawn
// Useful for assaulting players without having to deal with Valve's crazy assault system of doom
//

LINK_ENTITY_TO_CLASS( so_spawner_rallypoint, CNPCRallyPoint );

CNPCRallyPoint::CNPCRallyPoint()
{
	m_iOwner = 0;
	m_bActive = false;	// not active by default
}

void CNPCRallyPoint::Spawn()
{
	Precache();
	SetSolid( SOLID_NONE );
	AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_FLY );
	m_vecCoordinates = GetAbsOrigin();
}

Vector CNPCRallyPoint::GetCoordinates()
{
	return m_vecCoordinates;
}

void CNPCRallyPoint::SetCoordinates( Vector vecNewRallyCoordinates )
{
	m_vecCoordinates = vecNewRallyCoordinates;
	SetAbsOrigin( vecNewRallyCoordinates );
}

int CNPCRallyPoint::GetSpawnParent()
{
	return m_iOwner; 
}

void CNPCRallyPoint::SetSpawnParent( int entindex )
{
	m_iOwner = entindex;
}

void CNPCRallyPoint::ActivateRallyPoint()
{
	m_bActive = true;
}

void CNPCRallyPoint::DeactivateRallyPoint()
{
	m_bActive = false;
}