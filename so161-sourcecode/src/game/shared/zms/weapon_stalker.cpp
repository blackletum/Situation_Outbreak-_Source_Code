//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		zombie - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "zms/weapon_stalker.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponStalker
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponStalker, DT_WeaponStalker )

BEGIN_NETWORK_TABLE( CWeaponStalker, DT_WeaponStalker )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponStalker )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_stalker, CWeaponStalker );
PRECACHE_WEAPON_REGISTER( weapon_stalker );

//#ifndef CLIENT_DLL

acttable_t	CWeaponStalker::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponStalker);

//#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponStalker::CWeaponStalker( void )
{
	//m_bDelayedAttack = false;
	//m_flDelayedAttackTime = 0.0f;
	m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );
}

float CWeaponStalker::GetDamage( void )
{
	return 50.0f;
}