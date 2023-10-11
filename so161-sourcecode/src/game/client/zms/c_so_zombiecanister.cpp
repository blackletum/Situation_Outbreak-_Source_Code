//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "fx_explosion.h"
#include "tempentity.h"
#include "c_tracer.h"
#include "env_headcrabcanister_shared.h"
#include "baseparticleentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Headcrab canister Class (Client-side only!)
//-----------------------------------------------------------------------------
class C_ZombieCanister :  public C_BaseAnimating
{
	DECLARE_CLASS( C_ZombieCanister, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

public:
	//-------------------------------------------------------------------------
	// Initialization/Destruction
	//-------------------------------------------------------------------------
	C_ZombieCanister();
	~C_ZombieCanister();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink();

private:
	C_ZombieCanister( const C_ZombieCanister & );

	CEnvHeadcrabCanisterShared	m_Shared;
	CNetworkVar( bool, m_bLanded );
};


EXTERN_RECV_TABLE(DT_EnvHeadcrabCanisterShared);

IMPLEMENT_CLIENTCLASS_DT( C_ZombieCanister, DT_EnvZombieCanister, CZombieCanister )
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE(DT_EnvHeadcrabCanisterShared) ),
	RecvPropBool( RECVINFO( m_bLanded ) ),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_ZombieCanister::C_ZombieCanister()
{
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
C_ZombieCanister::~C_ZombieCanister()
{
}


//-----------------------------------------------------------------------------
// On data update
//-----------------------------------------------------------------------------
void C_ZombieCanister::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	// Stop client-side simulation on landing
	if ( m_bLanded )
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}


//-----------------------------------------------------------------------------
// Compute position
//-----------------------------------------------------------------------------
void C_ZombieCanister::ClientThink()
{
	Vector vecEndPosition;
	QAngle vecEndAngles;
	m_Shared.GetPositionAtTime( gpGlobals->curtime, vecEndPosition, vecEndAngles );
	SetAbsOrigin( vecEndPosition );
	SetAbsAngles( vecEndAngles );
}

