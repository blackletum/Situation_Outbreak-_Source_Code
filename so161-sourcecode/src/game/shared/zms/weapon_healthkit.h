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

#ifndef HL2MP_WEAPON_HEALTHKIT_H
#define HL2MP_WEAPON_HEALTHKIT_H
#pragma once


#include "hl2mp/weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp/weapon_hl2mpbasebasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponHealthKit C_WeaponHealthKit
#endif

//-----------------------------------------------------------------------------
// CWeaponHealthKit
//-----------------------------------------------------------------------------

class CWeaponHealthKit : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponHealthKit, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponHealthKit();

	void		Precache( void );
	void		ItemPostFrame( void );
	void		PrimaryAttack( void );
	void		SecondaryAttack( void );

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	//void		SecondaryAttack( void )	{	return;	}

	float		GetRange( void )		{	return	75.0f;	}
	float		GetFireRate( void )		{	return	1.0f;	}

	void		Drop( const Vector &vecVelocity );

	bool		ReloadOrSwitchWeapons( void );

	CWeaponHealthKit( const CWeaponHealthKit & );

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

private:
	bool m_bSuccessfulHeal;
};


#endif // HL2MP_WEAPON_HEALTHKIT_H