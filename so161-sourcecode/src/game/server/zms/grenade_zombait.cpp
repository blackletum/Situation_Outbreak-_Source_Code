//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Flaming bottle thrown from the hand
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "ammodef.h"
#include "gamerules.h"
#include "grenade_zombait.h"
#include "weapon_brickbat.h"
#include "soundent.h"
#include "decals.h"
#include "fire.h"
#include "shake.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// S:O - Additional Includes
#include "npc_BaseZombie.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexFireball;

#define MOLOTOV_EXPLOSION_VOLUME	1024

#define ZOMBAIT_MODEL "models/weapons/w_igrenade.mdl"

BEGIN_DATADESC( CGrenade_Zombait )

	DEFINE_FIELD( m_pFireTrail, FIELD_CLASSPTR ),

	// Function Pointers
	DEFINE_ENTITYFUNC( ZombaitTouch ),
	DEFINE_THINKFUNC( ZombaitThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_zombait, CGrenade_Zombait );

void CGrenade_Zombait::Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX ); 
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	RemoveEffects( EF_NOINTERP );

	SetModel( ZOMBAIT_MODEL );

	UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));

	SetTouch( &CGrenade_Zombait::ZombaitTouch );
	SetThink( &CGrenade_Zombait::ZombaitThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= 25.0f;
	m_DmgRadius		= 500.0f;

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( 1.0 );
	SetFriction( 0.8 );  // Give a little bounce so can flatten
	SetSequence( 1 );

	m_pFireTrail = SmokeTrail::CreateSmokeTrail();

	if( m_pFireTrail )
	{
		m_pFireTrail->m_SpawnRate			= 48;
		m_pFireTrail->m_ParticleLifetime	= 1.0f;
		
		m_pFireTrail->m_StartColor.Init( 0.2f, 0.6f, 0.2f );
		m_pFireTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		
		m_pFireTrail->m_StartSize	= 8;
		m_pFireTrail->m_EndSize		= 32;
		m_pFireTrail->m_SpawnRadius	= 8;
		m_pFireTrail->m_MinSpeed	= 8;
		m_pFireTrail->m_MaxSpeed	= 16;
		m_pFireTrail->m_Opacity		= 0.65f;

		m_pFireTrail->SetLifetime( 15.0f );
		m_pFireTrail->FollowEntity( this, "0" );
	}

	m_bDetonated = false;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenade_Zombait::ZombaitTouch( CBaseEntity *pOther )
{
	if ( !m_bDetonated )
		Detonate();
	else
		SetNextThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CGrenade_Zombait::Detonate( void ) 
{
	SetModelName( NULL_STRING );		//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );	// intangible

	m_takedamage = DAMAGE_NO;

	trace_t trace;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + Vector ( 0, 0, -128 ),  MASK_SOLID_BRUSHONLY, 
		this, COLLISION_GROUP_NONE, &trace);

	// Make sure we're not landing inside of something (pull back out a bit if necessary)
	if ( trace.fraction != 1.0 )
		SetLocalOrigin( trace.endpos + (trace.plane.normal * (m_flDamage - 24) * 0.6) );

	// Emit our detonation sound
	EmitSound( "Grenade_Molotov.Detonate");

	// Get the owner of the bait
	CBaseEntity *pOwner;
	pOwner = GetOwnerEntity();
	SetOwnerEntity( NULL ); // can't traceline attack owner if this is set

	// Make a nice little decal underneath where the bait was thrown
	UTIL_DecalTrace( &trace, "Scorch" );
	UTIL_BloodDecalTrace( &trace, 1 );
	UTIL_BloodDecalTrace( &trace, 2 );
	UTIL_BloodDecalTrace( &trace, 3 );
	UTIL_BloodDecalTrace( &trace, 1 );
	UTIL_BloodDecalTrace( &trace, 2 );
	UTIL_BloodDecalTrace( &trace, 3 );
	UTIL_BloodDecalTrace( &trace, 1 );
	UTIL_BloodDecalTrace( &trace, 2 );
	UTIL_BloodDecalTrace( &trace, 3 );

	// Shake nearby players' screen
	//UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );

	// Play a nice little imaginary jingle for enemy NPCs to hear
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	// Do some radius damage to those nearby
	//RadiusDamage( CTakeDamageInfo( this, pOwner, m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_DmgRadius, pOwner->Classify(), NULL );

	// Finally, bait our zombies
	BaitDemZombies( GetAbsOrigin() );

	// Other stuff required before deletion
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime + 0.2 );

	// Remove our fire trail before we delete ourselves
	//if ( m_pFireTrail )
	//	UTIL_Remove( m_pFireTrail );

	// We're done, so delete us
	//UTIL_Remove( this );

	// Indicate that this bait was detonated so it doesn't need to think anymore
	// This is save resources and prevents any nasty bugs from occuring
	m_bDetonated = true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenade_Zombait::ZombaitThink( void )
{
	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can throw the brickbat
	if (GetOwnerEntity())
	{
		trace_t tr;
		Vector	vUpABit = GetAbsOrigin();
		vUpABit.z += 5.0;

		CBaseEntity* saveOwner	= GetOwnerEntity();
		SetOwnerEntity( NULL );
		UTIL_TraceEntity( this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr );
		if ( tr.startsolid || tr.fraction != 1.0 )
		{
			SetOwnerEntity( saveOwner );
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CGrenade_Zombait::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( ZOMBAIT_MODEL );
	UTIL_PrecacheOther( "_firesmoke" );
	PrecacheScriptSound( "Grenade_Molotov.Detonate" );
}

//------------------------------------------------------------------------------------------------------------
// S:O - Baits all zombies by telling them to go to the 
//------------------------------------------------------------------------------------------------------------
void CGrenade_Zombait::BaitDemZombies( const Vector &targetPos )
{
	CNPC_BaseZombie *pSelector = NULL;

	Vector forward;
	//VectorNormalize( targetPos );

	for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
	{
		pSelector = dynamic_cast<CNPC_BaseZombie*>( gEntList.m_ZombieList[i] );

		if ( pSelector )
		{
			pSelector->SetEnemy( NULL );
			pSelector->SetSchedule( SCHED_ZOMBIE_AMBUSH_MODE );
			CAI_BaseNPC::ConqCommanded( targetPos, forward, true, pSelector );
		}
	}
}