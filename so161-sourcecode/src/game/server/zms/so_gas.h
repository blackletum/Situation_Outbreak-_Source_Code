//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_hint.h"
#include "env_Headcrabcanister_shared.h"
#include "explode.h"
#include "beam_shared.h"
#include "SpriteTrail.h"
#include "ar2_explosion.h"
#include "SkyCamera.h"
#include "smoke_trail.h"
#include "ai_basenpc.h"
#include "ai_motor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// GasCanister Class
//-----------------------------------------------------------------------------
class CGasCanister : public CBaseAnimating
{
	DECLARE_CLASS( CGasCanister, CBaseAnimating );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:

	// Initialization
	CGasCanister();

	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual void		UpdateOnRemove();

	virtual void		SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
		int m_nGasCount;
	void				InputFireCanister( inputdata_t &inputdata );
	void				InputOpenCanister( inputdata_t &inputdata );
	void				InputSpawnGass( inputdata_t &inputdata );
	void				InputStopSmoke( inputdata_t &inputdata );
	void				TeleportToRandomPlayer( void );
	// Think(s)
	void				GasCanisterSkyboxThink( void );
	void				GasCanisterWorldThink( void );
	void				GasCanisterSpawnGasThink();
	void				GasCanisterSkyboxOnlyThink( void );
	void				GasCanisterSkyboxRestartThink( void );
	void				WaitForOpenSequenceThink();

	// Place the canister in the world
	CSkyCamera*			PlaceCanisterInWorld();

	// Check for impacts
	void				TestForCollisionsAgainstEntities( const Vector &vecEndPosition );
	void				TestForCollisionsAgainstWorld( const Vector &vecEndPosition );

	// Figure out where we enter the world
	void				ComputeWorldEntryPoint( Vector *pStartPosition, QAngle *pStartAngles, Vector *pStartDirection );

	// Blows up!
	void				Detonate( void );

	// Landed!
	void				SetLanded( void );
	void				Landed( void );

	// Open!
	void				OpenCanister( void );
	void				CanisterFinishedOpening();

	// Set up the world model
	void				SetupWorldModel();

	// Start spawning Gass
	void				StartSpawningGass( float flDelay );
	CNetworkVar( bool, m_bLanded );

	CNetworkVarEmbedded( CEnvHeadcrabCanisterShared, m_Shared );
	CHandle<CSpriteTrail> m_hTrail;
	CHandle<SmokeTrail>	m_hSmokeTrail;
	int m_nGasType;
	Vector m_vecImpactPosition;
	float m_flDamageRadius;
	float m_flDamage;
	bool m_bIncomingSoundStarted;
	bool m_bHasDetonated;
	bool m_bLaunched;
	bool m_bOpened;
	float m_flSmokeLifetime;
	string_t m_iszLaunchPositionName;

	COutputEHANDLE m_OnLaunched;
	COutputEvent m_OnImpacted;
	COutputEvent m_OnOpened;

	// Only for skybox only cannisters.
	float m_flMinRefireTime;
	float m_flMaxRefireTime;
	int m_nSkyboxCannisterCount;
};
