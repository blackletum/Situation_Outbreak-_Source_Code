//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		zombie - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "zms/weapon_corpse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponCorpse
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCorpse, DT_WeaponCorpse )

BEGIN_NETWORK_TABLE( CWeaponCorpse, DT_WeaponCorpse )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCorpse )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_corpse, CWeaponCorpse );
PRECACHE_WEAPON_REGISTER( weapon_corpse );

//#ifndef CLIENT_DLL

acttable_t	CWeaponCorpse::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponCorpse);

//#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponCorpse::CWeaponCorpse( void )
{
	//m_bDelayedAttack = false;
	//m_flDelayedAttackTime = 0.0f;
	m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );
}

float CWeaponCorpse::GetDamage( void )
{
	// CORPSE ADVANTAGE: Does a lot of damage per hit
	return 75.0f;
}