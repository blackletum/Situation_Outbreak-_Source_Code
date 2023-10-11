//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2MP_PLAYER_H
#define HL2MP_PLAYER_H
#pragma once

class CHL2MP_Player;

#include "basemultiplayerplayer.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "hl2mp_playeranimstate.h"
#include "hl2mp_player_shared.h"
#include "hl2mp_gamerules.h"
#include "utldict.h"

//=============================================================================
// >> HL2MP_Player
//=============================================================================
class CHL2MPPlayerStateInfo
{
public:
	HL2MPPlayerState m_iPlayerState;
	const char *m_pStateName;

	void (CHL2MP_Player::*pfnEnterState)();	// Init and deinit the state.
	void (CHL2MP_Player::*pfnLeaveState)();

	void (CHL2MP_Player::*pfnPreThink)();	// Do a PreThink() in this state.
};

class CHL2MP_Player : public CHL2_Player
{
public:
	DECLARE_CLASS( CHL2MP_Player, CHL2_Player );

	CHL2MP_Player();
	~CHL2MP_Player( void );
	
	static CHL2MP_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CHL2MP_Player::s_PlayerEdict = ed;
		return (CHL2MP_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// This passes the event to the client's and server's CHL2MPPlayerAnimState.
	void			DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	void			SetupBones( matrix3x4_t *pBoneToWorld, int boneMask );

	virtual void Precache( void );
	virtual void Spawn( void );

	// S:O - SetSkin for new Military team
	virtual void SetSkin( int skinNum ) { m_nSkin = skinNum; }

	virtual void PostThink( void );
	virtual void PreThink( void );
	virtual void PlayerDeathThink( void );
	virtual bool HandleCommand_JoinTeam( int team );
	virtual bool ClientCommand( const CCommand &args );
	virtual void CreateViewModel( int viewmodelindex = 0 );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual bool WantsLagCompensationOnEntity( const CBaseEntity *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const; // AI Patch Addition.
	virtual void FireBullets ( const FireBulletsInfo_t &info );
	
	virtual int GetAmmoCount( int iAmmoIndex ) const;
	virtual void	RemoveAmmo( int iCount, int iAmmoIndex );

	// S:O - Determines the player's zombie class (if valid)
	const char *GetZombieClassName( void );

	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual void ChangeTeam( int iTeam );
	virtual void PickupObject ( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void UpdateOnRemove( void );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual CBaseEntity* EntSelectSpawnPoint( void );
		
	int FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );
	void	PrecacheFootStepSounds( void );
	bool	ValidatePlayerModel( const char *pModel );

	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	void CheatImpulseCommands( int iImpulse );
	void CreateRagdollEntity( void );
	void GiveAllItems( void );
	void GiveDefaultItems( void );

	void NoteWeaponFired( void );

	void SetAnimation( PLAYER_ANIM playerAnim );

	void SetPlayerModel( void );
	void SetPlayerTeamModel( void );
	Activity TranslateTeamActivity( Activity ActToTranslate );
	
	float GetNextModelChangeTime( void ) { return m_flNextModelChangeTime; }
	float GetNextTeamChangeTime( void ) { return m_flNextTeamChangeTime; }
	void  PickDefaultSpawnTeam( void );
	void  SetupPlayerSoundsByModel( const char *pModelName );
	const char *GetPlayerModelSoundPrefix( void );
	int	  GetPlayerModelType( void ) { return m_iPlayerSoundType;	}
	
	void  DetonateTripmines( void );

	void Reset();

	bool IsReady();
	void SetReady( bool bReady );

	void CheckChatText( char *p, int bufsize );

	void State_Transition( HL2MPPlayerState newState );
	void State_Enter( HL2MPPlayerState newState );
	void State_Leave();
	void State_PreThink();
	CHL2MPPlayerStateInfo *State_LookupInfo( HL2MPPlayerState state );

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();


	virtual bool StartObserverMode( int mode );
	virtual void StopObserverMode( void );


	Vector m_vecTotalBulletForce;	//Accumulator for bullet force in a single frame

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 

	virtual bool	CanHearAndReadChatFrom( CBasePlayer *pPlayer );

	// Player avoidance
	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;
	void HL2MPPushawayThink(void);

	// S:O - Extention on whether or not we can use a weapon based upon the team we're on.
	virtual bool Weapon_CanUse( CBaseCombatWeapon *pWeapon );

	// S:O - Classify each player based on their team for proper NPC relationship handling.
	Class_T Classify ( void );

	// ZMS -
	// This is where we define most all of our team-specific shit, including player models.
	void TeamConfig( void );

	// S:O - Leveling System
	CNetworkVar(int, m_iExp);
	CNetworkVar(int, m_iLevel);

	// S:O - Leveling System
	int GetXP() { return m_iExp; }
	void AddXP(int add=1) { m_iExp += add; CheckLevel(); }

	// S:O - Leveling System
	int GetLevel() { return m_iLevel; }
	void CheckLevel();

	// S:O - Leveling System
	void LevelUp();
	void ReapplyLevelPowerups();
	void ResetXP() { m_iExp = 0; m_iLevel = 1; LevelUp(); }

	// S:O - Leveling System
	// Different functions that can control what happens when a player levels up, etc...
	void SetHealthMax(int h) { m_iMaxHealth = h; }

	// S:O - Infection system
	void MakeInfected( bool tempinf ) { m_bIsInfected = tempinf; }
	bool IsInfected() { return m_bIsInfected; }

	// S:O - Infection system
	void StartZombifyTimer( float tempval ) { m_ZombifyTimer.Start( tempval ); }

private:

	CHL2MPPlayerAnimState *m_PlayerAnimState;

	CNetworkQAngle( m_angEyeAngles );

	int m_iLastWeaponFireUsercmd;
	int m_iModelType;
	CNetworkVar( bool, m_bSpawnInterpCounter );
	CNetworkVar( int, m_iPlayerSoundType );

	float m_flNextModelChangeTime;
	float m_flNextTeamChangeTime;

	float m_flSlamProtectTime;	

	HL2MPPlayerState m_iPlayerState;
	CHL2MPPlayerStateInfo *m_pCurStateInfo;

	bool ShouldRunRateLimitedCommand( const CCommand &args );

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float,int>	m_RateLimitLastCommandTimes;

    bool m_bEnterObserver;
	bool m_bReady;

	// S:O - Misc.
	float m_flCheckLevelUp;
	float m_flHealthRegenTime;
	float m_flGiveCommanderCash;
	float m_fBleedingDelay;
	float m_fSpeedChangeDelay;
	CountdownTimer m_ReinforcementsTimer;
	bool NeedsToBeSwitchedToReinforcements;

	// S:O - Extermination team balancing
	int m_iNumSurvivors;
	int m_iNumMilitary;

	// S:O - Infection system
	CountdownTimer m_ZombifyTimer;
	CNetworkVar(bool, m_bIsInfected);
};

inline CHL2MP_Player *ToHL2MPPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CHL2MP_Player*>( pEntity );
}

#endif //HL2MP_PLAYER_H
