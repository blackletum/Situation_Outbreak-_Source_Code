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

#ifndef HL2MP_weapon_charple_H
#define HL2MP_weapon_charple_H
#pragma once


/*#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"*/

#include "weapon_zombie.h"


#ifdef CLIENT_DLL
#define CWeaponCharple C_WeaponCharple
#endif

//-----------------------------------------------------------------------------
// CWeaponCharple
//-----------------------------------------------------------------------------

class CWeaponCharple : public CWeaponZombie
{
public:
	DECLARE_CLASS( CWeaponCharple, CWeaponZombie );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponCharple();

	float		GetDamage( void );

	CWeaponCharple( const CWeaponCharple & );

private:
	//bool  m_bDelayedAttack;
	//float m_flDelayedAttackTime;
	int m_spriteTexture;
};


#endif // HL2MP_weapon_charple_H




