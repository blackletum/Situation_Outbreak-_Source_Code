/////////////////////
// S:O - Chainsaw //
/////////////////////

// This is epic.
// This is Sparta!

#include "cbase.h"
#include "weapon_chainsaw.h"
#include "hl2mp/weapon_hl2mpbasehlmpcombatweapon.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
	#include "ai_basenpc.h"
	#include "ilagcompensationmanager.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponChainsaw
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponChainsaw, DT_WeaponChainsaw )

BEGIN_NETWORK_TABLE( CWeaponChainsaw, DT_WeaponChainsaw )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponChainsaw )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_chainsaw, CWeaponChainsaw );
PRECACHE_WEAPON_REGISTER( weapon_chainsaw );

//#ifndef CLIENT_DLL

acttable_t	CWeaponChainsaw::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_CROSSBOW,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_CROSSBOW,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_CROSSBOW,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_CROSSBOW,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_CROSSBOW,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_CROSSBOW,			false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_CROSSBOW,					false },
	
	{ ACT_MELEE_ATTACK1,				ACT_MELEE_ATTACK_SWING,					true }, //AI Patch Addition.
	{ ACT_IDLE,							ACT_IDLE_ANGRY_MELEE,					false }, //AI Patch Addition.
	{ ACT_IDLE_ANGRY,					ACT_IDLE_ANGRY_MELEE,					false }, //AI Patch Addition.
};

IMPLEMENT_ACTTABLE(CWeaponChainsaw);

//#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponChainsaw::CWeaponChainsaw( void )
{
}

// S:O - Override default ItemPostFrame method
void CWeaponChainsaw::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

	// S:O - One of the biggest hacks ever.
	// I'm really, really sorry about this.
#ifdef CLIENT_DLL
	if ( m_bFakeAnimate )
	{
		SetSkin( 1 );
		m_bFakeAnimate = false;
	}
	else
	{
		SetSkin( 2 );
		m_bFakeAnimate = true;
	}
#endif

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else if ( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		SecondaryAttack();
	}
	else 
	{
		WeaponIdle();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponChainsaw::GetDamageForActivity( Activity hitActivity )
{
	return 250.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponChainsaw::AddViewKickSecondary( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = SharedRandomFloat( "crowbarpax", 2.0f, 4.0f );
	punchAng.y = SharedRandomFloat( "crowbarpay", -4.0f, -2.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng );
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponChainsaw::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = SharedRandomFloat( "crowbarpax", 1.0f, 2.0f );
	punchAng.y = SharedRandomFloat( "crowbarpay", -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng );

	/*CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

#ifndef CLIENT_DLL
	pPlayer->SnapEyeAngles( angles );
#endif

	pPlayer->ViewPunch( QAngle( -0.5, random->RandomFloat( -1, 1 ), 0 ) );*/
}


#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
ConVar sk_chainsaw_lead_time( "sk_chainsaw_lead_time", "0.9" );

int CWeaponChainsaw::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = sk_chainsaw_lead_time.GetFloat();
	dt += random->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );
	if ((flDist > 64) && (flExtrapolatedDist > 64))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponChainsaw::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if ( pEnemy )
	{
		Vector vecDelta;
		VectorSubtract( pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta );
		VectorNormalize( vecDelta );
		
		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize( vecDelta2D );
		if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
		{
			vecDirection = vecDelta;
		}
	}


	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
		Vector(-16,-16,-16), Vector(36,36,36), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.75 );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponChainsaw::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::Drop( const Vector &vecVelocity )
{
	// S:O - Stop our chainsaw idle sound if we own the weapon but don't have it equipped.
	WeaponSound( TAUNT );

#ifndef CLIENT_DLL
	BaseClass::Drop(vecVelocity);
#endif
}

// S:O - Melee Weapon - Secondary Attack
// Does 5 damage and pushes the victim backwards.
// The swing for this attack is very fast as well.

void CWeaponChainsaw::Swing()
{
	trace_t traceHit;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;

	WeaponSound( SINGLE );

	Hit( traceHit );

	SendWeaponAnim( nHitActivity );

	pOwner->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.4f;
}

void CWeaponChainsaw::Hit(trace_t &traceHit)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		pOwner->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		WeaponSound( MELEE_HIT );
	
#ifndef CLIENT_DLL
		CTakeDamageInfo info( GetOwner(), GetOwner(), 5, DMG_CLUB );
		
		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
		ApplyMultiDamage();

		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );

		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pHitEntity );

		// Don't allow us to push the target back if it is a fellow survivor.
		if ( pPlayer && (pPlayer->GetTeamNumber() == pOwner->GetTeamNumber()) ) // yes, we're using numbers here to prevent us from using another include
			return;

		// Don't allow us to push the target back if it is a NPC that's not a zombie.
		if ( pHitEntity->IsNPC() && pHitEntity->Classify() != CLASS_ZOMBIE )
			return;

		if( pHitEntity->IsPlayer() || pHitEntity->IsNPC() )
		{
			float yawKick = random->RandomFloat( -10, 10 );
			float yawDirection = random->RandomFloat( -10, 10 );

			// Kick the player angles!
			pOwner->ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

			// Kick the target's angles, too!
			pHitEntity->ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

			Vector dir = pHitEntity->GetAbsOrigin() - pOwner->GetAbsOrigin();
			VectorNormalize(dir);

			QAngle angles;
			VectorAngles( dir, angles );
			Vector forward, right;
			AngleVectors( angles, &forward, &right, NULL );

			// Finally, push the target back...
			pHitEntity->ApplyAbsVelocityImpulse( forward * 1000.0f );
			pHitEntity->EmitSound( "NPC_Metropolice.Shove" );
		}
#endif
	}
}

void CWeaponChainsaw::SecondaryAttack()
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	Swing();
	AddViewKickSecondary();

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

void CWeaponChainsaw::SetSkin( int skinNum )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	pViewModel->m_nSkin = skinNum;
}