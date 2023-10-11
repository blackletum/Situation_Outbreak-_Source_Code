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

#ifndef HL2MP_weapon_stalker_H
#define HL2MP_weapon_stalker_H
#pragma once


/*#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"*/

#include "weapon_zombie.h"


#ifdef CLIENT_DLL
#define CWeaponStalker C_WeaponStalker
#endif

//-----------------------------------------------------------------------------
// CWeaponStalker
//-----------------------------------------------------------------------------

class CWeaponStalker : public CWeaponZombie
{
public:
	DECLARE_CLASS( CWeaponStalker, CWeaponZombie );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponStalker();

	float		GetDamage( void );

	CWeaponStalker( const CWeaponStalker & );

private:
	//bool  m_bDelayedAttack;
	//float m_flDelayedAttackTime;
	int m_spriteTexture;
};


#endif // HL2MP_weapon_stalker_H




