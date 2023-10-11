//========= Copyright © 2009 Agent Red Productions					 ============//
//
// Purpose: Template for scope based weapons
// Author: Stephen Swires
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sniperbase.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSOSniperBase, DT_WeaponSOSniperBase )

BEGIN_NETWORK_TABLE( CWeaponSOSniperBase, DT_WeaponSOSniperBase )
#ifdef CLIENT_DLL
RecvPropBool( RECVINFO( m_bNeedsCocking ) ),
RecvPropBool( RECVINFO( m_bIsCocking ) ),
RecvPropFloat( RECVINFO( m_flEndCockTime ) ),
#else
SendPropBool( SENDINFO( m_bNeedsCocking ) ),
SendPropBool( SENDINFO( m_bIsCocking ) ),
SendPropFloat( SENDINFO( m_flEndCockTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSOSniperBase )
END_PREDICTION_DATA()
#endif

CWeaponSOSniperBase::CWeaponSOSniperBase()
{
	m_bIsCocking = false;
	m_bNeedsCocking = false;
	m_flEndCockTime = 0.0f;
}

bool CWeaponSOSniperBase::Deploy( void )
{
	m_bIsCocking = false;
	m_flEndCockTime = 0.0f;

	return BaseClass::Deploy();
}


bool CWeaponSOSniperBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bInReload )
	{
		return false;
	}

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponSOSniperBase::PrimaryAttack( void )
{
	if( !IsBoltAction() || !m_bNeedsCocking )
	{
		int preShotAmmo = m_iClip1;

		BaseClass::PrimaryAttack();

		if ( preShotAmmo > 0 && UnscopeAfterShot()) // post shot unironsight, this is done after shooting just in case ironsighting affects accuracy
			ExitIronsights();

		if( m_iClip1 > 0 && IsBoltAction() ) // bolt action behaviour
			m_bNeedsCocking = true;

		WeaponSound( SPECIAL2 );
	}
}

// make snipers extremely accurate when scoped
float CWeaponSOSniperBase::GetAccuracyModifier()
{
	if( IsIronsighted() )
		return 0.01f;

	return BaseClass::GetAccuracyModifier();
}


bool CWeaponSOSniperBase::IsScoped()
{
	return IsIronsighted();
}

bool CWeaponSOSniperBase::ShouldDrawScope()
{
	return ( ( IsIronsighted() && gpGlobals->curtime >= m_flIronsightTime + GetIronsightTime() ) ? true : false );
}

void CWeaponSOSniperBase::ItemPostFrame( void )
{
	// bolt action sniper features
	if( IsBoltAction() )
	{
		// needs cocking, but we aren't cocking, so do it
		if( m_bNeedsCocking && !m_bIsCocking && gpGlobals->curtime >= m_flNextPrimaryAttack )
		{
			CockBolt();
		}
		// cocking has finished
		else if( m_bIsCocking && gpGlobals->curtime >= m_flEndCockTime )
		{
			FinishCocking();
		}
	}

	if( !IsCocking() || !IsBoltAction() )
		BaseClass::ItemPostFrame();
}

// start to cock the weapon
void CWeaponSOSniperBase::CockBolt()
{
	SendWeaponAnim( ACT_VM_PULLBACK );
	m_bIsCocking = true;

	float endTime = gpGlobals->curtime + SequenceDuration();
	
	m_flEndCockTime = endTime;
	m_flNextPrimaryAttack = endTime;
	SetWeaponIdleTime( endTime );
}

void CWeaponSOSniperBase::FinishCocking()
{
	m_bIsCocking = false;
	m_bNeedsCocking = false;
	m_flEndCockTime = 0.0f;
}