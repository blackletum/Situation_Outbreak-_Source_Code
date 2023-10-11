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

#ifndef HL2MP_WEAPON_CLEAVER_H
#define HL2MP_WEAPON_CLEAVER_H
#pragma once


#include "hl2mp/weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp/weapon_hl2mpbasebasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponCleaver C_WeaponCleaver
#endif

//-----------------------------------------------------------------------------
// CWeaponCleaver
//-----------------------------------------------------------------------------

class CWeaponCleaver : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponCleaver, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponCleaver();

	void		ItemPostFrame( void );

	void		SecondaryAttack( void );
	void		Swing( void );
	void		Hit( trace_t &traceHit );

	void		AddViewKick( void );
	void		AddViewKickSecondary( void );
	float		GetDamageForActivity( Activity hitActivity );
	//void		SecondaryAttack( void )	{	return;	}

	float		GetRange( void )		{	return	60.0f;	}
	float		GetFireRate( void )		{	return	0.6f;	}

	void		Drop( const Vector &vecVelocity );

	CWeaponCleaver( const CWeaponCleaver & );

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif
};


#endif // HL2MP_WEAPON_CLEAVER_H