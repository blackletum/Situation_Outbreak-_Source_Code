//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
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

#ifndef HL2MP_WEAPON_AXE_H
#define HL2MP_WEAPON_AXE_H
#pragma once


#include "hl2mp/weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp/weapon_hl2mpbasebasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponAxe C_WeaponAxe
#endif

//-----------------------------------------------------------------------------
// CWeaponAxe
//-----------------------------------------------------------------------------

class CWeaponAxe : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponAxe, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponAxe();

	void		ItemPostFrame( void );

	void		SecondaryAttack( void );
	void		Swing( void );
	void		Hit( trace_t &traceHit );

	void		AddViewKick( void );
	void		AddViewKickSecondary( void );
	float		GetDamageForActivity( Activity hitActivity );
	//void		SecondaryAttack( void )	{ return; }

	float		GetRange( void )		{	return	70.0f;	}
	float		GetFireRate( void )		{	return	SequenceDuration();	}	// more realistic with new animations

	void		Drop( const Vector &vecVelocity );

	CWeaponAxe( const CWeaponAxe & );

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif
};


#endif // HL2MP_WEAPON_AXE_H