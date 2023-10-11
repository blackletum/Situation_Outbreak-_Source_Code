//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2MP_GAMERULES_H
#define HL2MP_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "gamevars_shared.h"

#ifndef CLIENT_DLL
#include "hl2mp_player.h"
#endif

#define VEC_CROUCH_TRACE_MIN	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMax

enum
{
	TEAM_ZOMBIES = 2,
	TEAM_SURVIVORS,
	TEAM_MILITARY,
	TEAM_VIP,
};

enum
{
	RoundStarted = 0,
	RoundEnding,
	RoundEnded,
};

#ifdef CLIENT_DLL
	#define CHL2MPRules C_HL2MPRules
	#define CHL2MPGameRulesProxy C_HL2MPGameRulesProxy
#endif

class CHL2MPGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHL2MPGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class HL2MPViewVectors : public CViewVectors
{
public:
	HL2MPViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax ) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;	
};

class CHL2MPRules : public CTeamplayRules
{
public:
	DECLARE_CLASS( CHL2MPRules, CTeamplayRules );

#ifdef CLIENT_DLL

	virtual int GetRoundtimerRemain( void );
	virtual int GetReinforcementsTimerRemain( void );
	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif
	
	CHL2MPRules();
	virtual ~CHL2MPRules();

	virtual void Precache( void );
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon );
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
	virtual void Think( void );
	virtual void CreateStandardEntities( void );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual void GoToIntermission( void );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual const char *GetGameDescription( void );
	// derive this function if you mod uses encrypted weapon info files
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"x9Ke0BY7"; }
	virtual const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;

	float GetMapRemainingTime();
	void CleanUpMap();
	
#ifndef CLIENT_DLL

	// S:O - Misc.
	bool runoncepl0x;
	bool zombie_spawned;
	bool PlayersAreConnected;
	bool RoundNowEnding;
	bool RoundEndOverlayFired;
	bool SlowMotion;
	void CheckRestartGame();
	void RestartGame();
	void RoundRestart();

	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );
	virtual float	FlItemRespawnTime( CItem *pItem );
	virtual bool	CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	void	AddLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	ManageObjectRelocation( void );
	void    CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg );
	const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void InitDefaultAIRelationships( void ); //AI Patch Addition.

	// S:O - Misc.
	int		GetRoundtimerRemain2( void );
	int		GetReinforcementsTimerRemain2( void );
	int		GetAlivePlayers(int iIndex);
	int		GetAllPlayers(int iIndex);
	int		GetRoundState( void ) { return m_iCurrentState; }
	void	ZombifyPlayer( CBasePlayer *pPlayer);
	void	ZombifyFirstPlayer( void );
	void	PickVIP( void );
	void	SetRoundState( int iState, float flTimeInState )
	{
		m_StateTimer.Start( flTimeInState );
		m_iCurrentState = iState;

		DevMsg( "Set state: %d - time left in state %4.1f\n", iState, flTimeInState );
	}
	virtual void ZombieSound( void );
	void	HandleRoundState( void );// cjd @add
	void	RecalcDifficulty( void );			// cjd @add
	virtual void StartRoundtimer( int iDuration );
	virtual void StartReinforcementsTimer( int iDuration );
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ); // cjd @add - so we can determine when players are allowed to respawn

	int CalculateSurvivorCount( void );
	int CalculateZombieCount( void );
	int CalculateMilitaryCount( void );

	// S:O - Slow Motion Effect
	/*void ActivateSlowMotion( void );
	void DeactivateSlowMotion( void );*/

#endif
	virtual void ClientDisconnected( edict_t *pClient );

	bool CheckGameOver( void );
	bool IsIntermission( void );

	void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	
	bool	IsTeamplay( void ) { return m_bTeamPlayEnabled;	}
	void	CheckAllPlayersReady( void );

	// S:O - Misc.
	CNetworkVar( float, m_fStart);
	CNetworkVar( int, m_iDuration);
	CNetworkVar( float, m_fReinforcementsStart);
	CNetworkVar( int, m_iReinforcementsDuration);

private:

#ifndef CLIENT_DLL
	void SpawnBoss( void );

	int m_iCurrentState;// cjd @add - holds state information about the game
	CountdownTimer m_StateTimer;// cjd @add - Countdown to when the players respawn after everybody is dead

	/*// S:O - Slow Motion Effect
	CountdownTimer m_SlowMotionAntiSpamTimer;*/

	// S:O - New Stuffs
	float m_fReforceTimer;
	float m_fDifficultyCalcTimer;
	float m_fDangerCalcTimer;
	float m_fSlowMotionTimer;
	float m_flRestartGameTime;
	float m_tmNextPeriodicThink;
	float m_tmNextStateThink;
	int zombie_timer;
	int zombie_min;
	int zombie_max;
	int zombie_result;
	int bossspawn_timer;
	int bossspawn_min;
	int bossspawn_max;
	int bossspawn_result;
	int iKills;
	int iTeam;
    int iDeaths;
	bool m_bCompleteReset;
	bool m_bAwaitingReadyRestart;
	bool m_bHeardAllPlayersReady;
	bool m_bBossSpawned;

	// S:O - New boolean declarations
	bool bNPCKill;
	bool bZombieNPCKill;
	bool bSurvivorNPCKill;
	bool bMilitaryNPCKill;

	// S:O - Recalculate Dynamic Difficulty
	char szReaperHealth[512];
	char szReaperNoJumpHealth[512];
	char szSeekerHealth[512];
	char szCreeperHealth[512];
	char szSploderHealth[512];

	float szReaperHealthFloat;
	float szReaperNoJumpHealthFloat;
	float szSeekerHealthFloat;
	float szCreeperHealthFloat;
	float szSploderHealthFloat;

	//int m_iNumSurvivors;
	//int m_iNumMilitary;

#endif

	CNetworkVar( bool, m_bTeamPlayEnabled );
	CNetworkVar( float, m_flGameStartTime );
	CUtlVector<EHANDLE> m_hRespawnableItemsAndWeapons;
};

inline CHL2MPRules* HL2MPRules()
{
	return static_cast<CHL2MPRules*>(g_pGameRules);
}

#endif //HL2MP_GAMERULES_H