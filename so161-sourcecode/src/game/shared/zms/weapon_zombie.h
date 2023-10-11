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

#ifndef HL2MP_weapon_zombie_H
#define HL2MP_weapon_zombie_H
#pragma once


#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponZombie C_WeaponZombie
#endif

//-----------------------------------------------------------------------------
// CWeaponZombie
//-----------------------------------------------------------------------------

class CWeaponZombie : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponZombie, CBaseHL2MPBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif

	CWeaponZombie();

	virtual float		GetRange( void );
	virtual float		GetRangeZombieCommand( void );
	virtual float		GetFireRate( void );
	virtual float		GetDamage( void );
	virtual void		ItemPostFrame( void );
	virtual void		NPCMakerSpawn( void );
	virtual void		AddViewKick( void );
	virtual float		GetDamageForActivity( Activity hitActivity );
	//void		SecondaryAttack( void )	{	return;	}
	virtual void		PrimaryAttack( void );
	//void		DelayedAttackCheck( void );
	virtual void		Swing( void);
	virtual void		Hit( trace_t &traceHit );
	virtual void		ZombieCommand( trace_t &traceHitZombieCommand, Vector &endPos );

	virtual void		SecondaryAttack( void );
	virtual void		SwingSecondary( void);
	virtual void		HitSecondary();

	virtual void		Drop( const Vector &vecVelocity );

	CNetworkVar( float, m_flNextNPCCreateTime );

	CWeaponZombie( const CWeaponZombie & );

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

private:
	//bool  m_bDelayedAttack;
	//float m_flDelayedAttackTime;
	int m_spriteTexture;
};


#endif // HL2MP_weapon_zombie_H




