//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MONSTERMAKER_H
#define MONSTERMAKER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"


//-----------------------------------------------------------------------------
// Spawnflags
//-----------------------------------------------------------------------------
#define	SF_NPCMAKER_START_ON		1	// start active ( if has targetname )
#define SF_NPCMAKER_NPCCLIP			8	// Children are blocked by NPCclip
#define SF_NPCMAKER_FADE			16	// Children's corpses fade
#define SF_NPCMAKER_INF_CHILD		32	// Infinite number of children
#define	SF_NPCMAKER_NO_DROP			64	// Do not adjust for the ground's position when checking for spawn
#define SF_NPCMAKER_HIDEFROMPLAYER	128 // Don't spawn if the player's looking at me
#define SF_NPCMAKER_ALWAYSUSERADIUS	256	// Use radius spawn whenever spawning
#define SF_NPCMAKER_NOPRELOADMODELS 512	// Suppress preloading into the cache of all referenced .mdl files

//=========================================================
//=========================================================
class CNPCSpawnDestination : public CPointEntity 
{
	DECLARE_CLASS( CNPCSpawnDestination, CPointEntity );

public:
	CNPCSpawnDestination();
	bool IsAvailable();						// Is this spawn destination available for selection?
	void OnSpawnedNPC( CAI_BaseNPC *pNPC );	// Notify this spawn destination that an NPC has spawned here.

	float		m_ReuseDelay;		// How long to be unavailable after being selected
	string_t	m_RenameNPC;		// If not NULL, rename the NPC that spawns here to this.
	float		m_TimeNextAvailable;// The time at which this destination will be available again.

	COutputEvent	m_OnSpawnNPC;

	DECLARE_DATADESC();
};

// S:O - A rally point target entity for NPC spawners
// Makes all NPCs of a cetain NPC spawner gather at a specific location when they spawn
// Useful for assaulting players without having to deal with Valve's crazy assault system of doom
class CNPCRallyPoint : public CBaseAnimating
{
public:
	DECLARE_CLASS( CNPCRallyPoint, CBaseAnimating );

	CNPCRallyPoint();

	void Spawn( void );
	int GetSpawnParent();
	void SetSpawnParent( const int entindex );

	void SetCoordinates( Vector vecNewRallyCoordinates );
	Vector GetCoordinates();

	void ActivateRallyPoint();
	void DeactivateRallyPoint();

	bool IsActive() {return m_bActive;}

private:
	Vector m_vecCoordinates;

	int m_iOwner;
	bool m_bActive; //qck: If the player sets a spawn point, this is activated.
};

abstract_class CBaseNPCMaker : public CBaseEntity
{
public:
	DECLARE_CLASS( CBaseNPCMaker, CBaseEntity );

	void Spawn( void );
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void MakerThink( void );
	bool HumanHullFits( const Vector &vecLocation );
	bool CanMakeNPC( bool bIgnoreSolidEntities = false );

	virtual void DeathNotice( CBaseEntity *pChild );// NPC maker children use this to tell the NPC maker that they have died.
	virtual void MakeNPC( void ) = 0;

	virtual	void ChildPreSpawn( CAI_BaseNPC *pChild ) {};
	virtual	void ChildPostSpawn( CAI_BaseNPC *pChild );

	CBaseNPCMaker(void) {}

	// Input handlers
	void InputSpawnNPC( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputSetMaxChildren( inputdata_t &inputdata );
	void InputAddMaxChildren( inputdata_t &inputdata );
	void InputSetMaxLiveChildren( inputdata_t &inputdata );
	void InputSetSpawnFrequency( inputdata_t &inputdata );

	// State changers
	void Toggle( void );
	virtual void Enable( void );
	virtual void Disable( void );

	virtual bool IsDepleted( void );

	DECLARE_DATADESC();
	
	int			m_nMaxNumNPCs;			// max number of NPCs this ent can create
	float		m_flSpawnFrequency;		// delay (in secs) between spawns 

	COutputEHANDLE m_OnSpawnNPC;
	COutputEvent m_OnAllSpawned;
	COutputEvent m_OnAllSpawnedDead;
	COutputEvent m_OnAllLiveChildrenDead;
	
	int		m_nLiveChildren;	// how many NPCs made by this NPC maker that are currently alive
	int		m_nMaxLiveChildren;	// max number of NPCs that this maker may have out at one time.

	bool	m_bDisabled;

	EHANDLE m_hIgnoreEntity;
	string_t m_iszIngoreEnt;

	// S:O - Rallypoint
	string_t rallypointName;
	CNPCRallyPoint*	pRallyEnt;
};

class CNPCMaker : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CNPCMaker, CBaseNPCMaker );

	CNPCMaker( void );

	void Precache( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_iszNPCClassname;			// classname of the NPC(s) that will be created.
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_spawnEquipment;
	string_t m_RelationshipString;		// Used to load up relationship keyvalues
	string_t m_ChildTargetName;
};

//-----------------------------------------------------------------------------
// S:O - so_zombiespawner
// Spawns the specified classname of zombie NPC every x seconds.
//-----------------------------------------------------------------------------

class CZombieSpawner : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CZombieSpawner, CBaseNPCMaker );

	CZombieSpawner( void );

	void Spawn( void );
	void Precache( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_iszNPCClassname;
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_ChildTargetName;
	string_t m_DynDiffDetails;

	bool m_bDynDiff;

	int iKills;
};

//-----------------------------------------------------------------------------
// S:O - so_zombiespawner_random
// Spawns a random class of zombie NPC every x seconds.
//-----------------------------------------------------------------------------

class CZombieSpawnerRandom : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CZombieSpawnerRandom, CBaseNPCMaker );

	CZombieSpawnerRandom( void );

	void Spawn( void );
	void Precache( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_ChildTargetName;
	string_t m_DynDiffDetails;

	bool m_bIncludeSploderZombie;
	bool m_bDynDiff;

	int iKills;
};

//-----------------------------------------------------------------------------
// S:O - so_survivorspawner
// Spawns a survivor NPC every x seconds.
//-----------------------------------------------------------------------------

class CSurvivorSpawner : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CSurvivorSpawner, CBaseNPCMaker );

	CSurvivorSpawner( void );

	void Spawn( void );
	void Precache( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_spawnEquipment;
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_ChildTargetName;
};

//-----------------------------------------------------------------------------
// S:O - so_militaryspawner
// Spawns a military NPC every x seconds.
//-----------------------------------------------------------------------------

class CMilitarySpawner : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CMilitarySpawner, CBaseNPCMaker );

	CMilitarySpawner( void );

	void Spawn( void );
	void Precache( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_spawnEquipment;
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_ChildTargetName;
};

//-----------------------------------------------------------------------------
// S:O - so_weaponspawner_random
// Spawns a random weapon at the start of each round.
//-----------------------------------------------------------------------------

class CZMSRandWeapon : public CPointEntity
{
public:
	DECLARE_CLASS( CZMSRandWeapon, CPointEntity );

	CZMSRandWeapon( void );

	void Precache( void );
	void Spawn( void );

	DECLARE_DATADESC();

	bool m_bMeleeOnly;
};

class CTemplateNPCMaker : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CTemplateNPCMaker, CBaseNPCMaker );

	CTemplateNPCMaker( void ) 
	{
		m_iMinSpawnDistance = 0;
	}

	virtual void Precache();

	virtual CNPCSpawnDestination *FindSpawnDestination();
	virtual void MakeNPC( void );
	void MakeNPCInRadius( void );
	void MakeNPCInLine( void );
	virtual void MakeMultipleNPCS( int nNPCs );

protected:
	virtual void PrecacheTemplateEntity( CBaseEntity *pEntity );

	bool PlaceNPCInRadius( CAI_BaseNPC *pNPC );
	bool PlaceNPCInLine( CAI_BaseNPC *pNPC );

	// Inputs
	void InputSpawnInRadius( inputdata_t &inputdata ) { MakeNPCInRadius(); }
	void InputSpawnInLine( inputdata_t &inputdata ) { MakeNPCInLine(); }
	void InputSpawnMultiple( inputdata_t &inputdata );
	void InputChangeDestinationGroup( inputdata_t &inputdata );
	void InputSetMinimumSpawnDistance( inputdata_t &inputdata );
	
	float	m_flRadius;

	DECLARE_DATADESC();

	string_t m_iszTemplateName;		// The name of the NPC that will be used as the template.
	string_t m_iszTemplateData;		// The keyvalue data blob from the template NPC that will be used to spawn new ones.
	string_t m_iszDestinationGroup;	

	int		m_iMinSpawnDistance;

	enum ThreeStateYesNo_t
	{
		TS_YN_YES = 0,
		TS_YN_NO,
		TS_YN_DONT_CARE,
	};

	enum ThreeStateDist_t
	{
		TS_DIST_NEAREST = 0,
		TS_DIST_FARTHEST,
		TS_DIST_DONT_CARE,
	};

	ThreeStateYesNo_t	m_CriterionVisibility;
	ThreeStateDist_t	m_CriterionDistance;
};

#endif // MONSTERMAKER_H
