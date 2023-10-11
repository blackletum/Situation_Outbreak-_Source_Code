//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		zombie - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "zms/weapon_charple.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponCharple
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCharple, DT_WeaponCharple )

BEGIN_NETWORK_TABLE( CWeaponCharple, DT_WeaponCharple )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCharple )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_charple, CWeaponCharple );
PRECACHE_WEAPON_REGISTER( weapon_charple );

//#ifndef CLIENT_DLL

acttable_t	CWeaponCharple::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_MELEE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_MELEE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_MELEE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_MELEE,					false },
	
	{ ACT_MELEE_ATTACK1,				ACT_MELEE_ATTACK_SWING,					true }, //AI Patch Addition.
	{ ACT_IDLE,							ACT_IDLE_ANGRY_MELEE,					false }, //AI Patch Addition.
	{ ACT_IDLE_ANGRY,					ACT_IDLE_ANGRY_MELEE,					false }, //AI Patch Addition.
};

IMPLEMENT_ACTTABLE(CWeaponCharple);

//#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponCharple::CWeaponCharple( void )
{
	//m_bDelayedAttack = false;
	//m_flDelayedAttackTime = 0.0f;
	m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );
}

float CWeaponCharple::GetDamage( void )
{
	// CHARPLE WEAKNESS: Doesn't do a whole lot of damage per hit
	return 25.0f;
}