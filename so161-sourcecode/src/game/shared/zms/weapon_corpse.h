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

#ifndef HL2MP_weapon_corpse_H
#define HL2MP_weapon_corpse_H
#pragma once


/*#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"*/

#include "weapon_zombie.h"

#ifdef CLIENT_DLL
#define CWeaponCorpse C_WeaponCorpse
#endif

//-----------------------------------------------------------------------------
// CWeaponCorpse
//-----------------------------------------------------------------------------

class CWeaponCorpse : public CWeaponZombie
{
public:
	DECLARE_CLASS( CWeaponCorpse, CWeaponZombie );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponCorpse();

	float		GetDamage( void );

	CWeaponCorpse( const CWeaponCorpse & );

private:
	//bool  m_bDelayedAttack;
	//float m_flDelayedAttackTime;
	int m_spriteTexture;
};


#endif // HL2MP_weapon_corpse_H




