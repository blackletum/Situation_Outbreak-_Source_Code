///////////////////////
// S:O - Health Kit //
///////////////////////

#include "cbase.h"
#include "weapon_healthkit.h"
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
// CWeaponHealthKit
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponHealthKit, DT_WeaponHealthKit )

BEGIN_NETWORK_TABLE( CWeaponHealthKit, DT_WeaponHealthKit )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponHealthKit )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_healthkit, CWeaponHealthKit );
PRECACHE_WEAPON_REGISTER( weapon_healthkit );

//#ifndef CLIENT_DLL

acttable_t	CWeaponHealthKit::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_PHYSGUN,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PHYSGUN,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_PHYSGUN,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PHYSGUN,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_PHYSGUN,					false },

	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,					false },
	
	{ ACT_MELEE_ATTACK1,				ACT_MELEE_ATTACK_SWING,					true }, //AI Patch Addition.
	{ ACT_IDLE,							ACT_IDLE_ANGRY_MELEE,					false }, //AI Patch Addition.
	{ ACT_IDLE_ANGRY,					ACT_IDLE_ANGRY_MELEE,					false }, //AI Patch Addition.
};

IMPLEMENT_ACTTABLE(CWeaponHealthKit);

//#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponHealthKit::CWeaponHealthKit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponHealthKit::GetDamageForActivity( Activity hitActivity )
{
	// Health kits don't do any damage, silly!
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponHealthKit::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = SharedRandomFloat( "crowbarpax", 1.0f, 2.0f );
	punchAng.y = SharedRandomFloat( "crowbarpay", -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng );
}


#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
ConVar sk_healthkit_lead_time( "sk_healthkit_lead_time", "0.9" );

int CWeaponHealthKit::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = sk_healthkit_lead_time.GetFloat();
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
void CWeaponHealthKit::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
		//ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponHealthKit::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
void CWeaponHealthKit::Drop( const Vector &vecVelocity )
{
	// Don't allow us to drop our health kit if we just successfully healed someone!
	// This could have been a nasty bug if we didn't catch this...
	if ( !m_bSuccessfulHeal )
	{
#ifndef CLIENT_DLL
		BaseClass::Drop(vecVelocity);
#endif
	}
}

void CWeaponHealthKit::Precache( void )
{
	//Call base class first
	BaseClass::Precache();

	PrecacheScriptSound( "HealthVial.Touch" );
}

void CWeaponHealthKit::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK2) && (gpGlobals->curtime >= m_flNextSecondaryAttack) )
	{
		SecondaryAttack();
		return;
	}
	else if ( pOwner->m_nButtons & IN_FIREMODE )
	{
		return;
	}

	BaseClass::ItemPostFrame();

	if ( m_bSuccessfulHeal && gpGlobals->curtime >= (m_flNextPrimaryAttack - 0.25f) )
	{
		m_bSuccessfulHeal = false;

#ifndef CLIENT_DLL
		UTIL_Remove(this);

		if ( pOwner )
		{
			// We should always have a pair of these on us anyway, so switch to them after we've used our health kit.
			pOwner->Weapon_Switch( pOwner->Weapon_OwnsThisType( "weapon_brassknuckles" ) );
		}
#endif
	}
}

// S:O - Health Kit - Primary Attack
// Fully heals the owner!

void CWeaponHealthKit::PrimaryAttack()
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	if( gpGlobals->curtime >= m_flNextPrimaryAttack )
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

		if ( !pPlayer )
			return;

		if ( pPlayer->m_iHealth < pPlayer->m_iMaxHealth )
		{
			pPlayer->m_iHealth = pPlayer->m_iMaxHealth;

#ifndef CLIENT_DLL
			EmitSound( "HealthVial.Touch" );
#endif

			// Call all appropriate animations.
			SendWeaponAnim( ACT_VM_HITCENTER );
			pPlayer->SetAnimation( PLAYER_ATTACK1 );
			ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

			AddViewKick();

			m_bSuccessfulHeal = true;
		}
		else
		{
			m_bSuccessfulHeal = false;
		}

		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
	}

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

// S:O - Health Kit - Secondary Attack
// Fully heals friendly players and NPCs!

void CWeaponHealthKit::SecondaryAttack()
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	if( gpGlobals->curtime >= m_flNextPrimaryAttack )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetOwner() );

		if ( !pPlayer )
			return;

		// Set up traceline & direction vector.
		trace_t tr;
		Vector vecDir;

		// Set up and get the angles & the vectors.
		Vector vecStart = pPlayer->Weapon_ShootPosition( );
		pPlayer->EyeVectors( &vecDir, NULL, NULL );
		Vector vecStop = vecStart + vecDir * GetRange();

		// Do the actual traceline.
		UTIL_TraceLine( vecStart, vecStop, MASK_SHOT_HULL, pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( pPlayer->GetTeamNumber() == 3 && pPlayer->IsAlive() )
		{
			// Lots of checks, I know, but we have to be sure everything is right before we go ahead and heal someone.
			if ( (tr.m_pEnt) && (tr.m_pEnt->IsPlayer()) && (tr.m_pEnt->IsAlive()) && (tr.m_pEnt->GetTeamNumber() == 3) && (tr.m_pEnt->m_iHealth < tr.m_pEnt->m_iMaxHealth) && (!m_bSuccessfulHeal) )
			{
				tr.m_pEnt->m_iHealth = tr.m_pEnt->m_iMaxHealth;

	#ifndef CLIENT_DLL
				EmitSound( "HealthVial.Touch" );
	#endif

				// Call all appropriate animations.
				SendWeaponAnim( ACT_VM_MISSCENTER );
				pPlayer->SetAnimation( PLAYER_ATTACK1 );
				ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

				AddViewKick();

				m_bSuccessfulHeal = true;
			}
			else if ( (tr.m_pEnt) && (tr.m_pEnt->IsNPC()) && (FClassnameIs(tr.m_pEnt, "npc_citizen")) && (tr.m_pEnt->m_iHealth < tr.m_pEnt->m_iMaxHealth) && (!m_bSuccessfulHeal) )
			{
				tr.m_pEnt->m_iHealth = tr.m_pEnt->m_iMaxHealth;

	#ifndef CLIENT_DLL
				EmitSound( "HealthVial.Touch" );
	#endif

				// Give the owner some cash for being nice enough to heal an ally
				int money = pPlayer->m_iMoney;
				pPlayer->m_iMoney = money + 500;

				// Give the owner some experience for being nice enough to heal an ally
				int experience = pPlayer->GetXP();
				pPlayer->m_iExp = experience + 5;

				// Call all appropriate animations.
				SendWeaponAnim( ACT_VM_MISSCENTER );
				pPlayer->SetAnimation( PLAYER_ATTACK1 );
				ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

				AddViewKick();

				m_bSuccessfulHeal = true;
			}
			else
			{
				m_bSuccessfulHeal = false;
			}
		}
		else if ( pPlayer->GetTeamNumber() == 4 && pPlayer->IsAlive() )
		{
			// Lots of checks, I know, but we have to be sure everything is right before we go ahead and heal someone.
			if ( (tr.m_pEnt) && (tr.m_pEnt->IsPlayer()) && (tr.m_pEnt->IsAlive()) && (tr.m_pEnt->GetTeamNumber() == 4) && (tr.m_pEnt->m_iHealth < tr.m_pEnt->m_iMaxHealth) && (!m_bSuccessfulHeal) )
			{
				tr.m_pEnt->m_iHealth = tr.m_pEnt->m_iMaxHealth;

	#ifndef CLIENT_DLL
				EmitSound( "HealthVial.Touch" );
	#endif

				// Call all appropriate animations.
				SendWeaponAnim( ACT_VM_MISSCENTER );
				pPlayer->SetAnimation( PLAYER_ATTACK1 );
				ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

				AddViewKick();

				m_bSuccessfulHeal = true;
			}
			else if ( (tr.m_pEnt) && (tr.m_pEnt->IsNPC()) && (FClassnameIs(tr.m_pEnt, "npc_combine_s")) && (tr.m_pEnt->m_iHealth < tr.m_pEnt->m_iMaxHealth) && (!m_bSuccessfulHeal) )
			{
				tr.m_pEnt->m_iHealth = tr.m_pEnt->m_iMaxHealth;

	#ifndef CLIENT_DLL
				EmitSound( "HealthVial.Touch" );
	#endif

				// Give the owner some cash for being nice enough to heal an ally
				int money = pPlayer->m_iMoney;
				pPlayer->m_iMoney = money + 500;

				// Give the owner some experience for being nice enough to heal an ally
				int experience = pPlayer->GetXP();
				pPlayer->m_iExp = experience + 5;

				// Call all appropriate animations.
				SendWeaponAnim( ACT_VM_MISSCENTER );
				pPlayer->SetAnimation( PLAYER_ATTACK1 );
				ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

				AddViewKick();

				m_bSuccessfulHeal = true;
			}
			else
			{
				m_bSuccessfulHeal = false;
			}
		}
		else
		{
			m_bSuccessfulHeal = false;
		}

		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
	}

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: If the current weapon has more ammo, reload it. Otherwise, switch 
//			to the next best weapon we've got. Returns true if it took any action.
//-----------------------------------------------------------------------------
bool CWeaponHealthKit::ReloadOrSwitchWeapons( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	Assert( pOwner );

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// weapon isn't useable, switch.
		if ( (GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == false )
		{
#ifndef CLIENT_DLL
			UTIL_Remove(this);
			// We should always have a pair of these on us anyway, so switch to them
			pOwner->Weapon_Switch( pOwner->Weapon_OwnsThisType( "weapon_brassknuckles" ) );
#endif

			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
			return true;
		}
	}
	else
	{
		// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( UsesClipsForAmmo1() && 
			 (m_iClip1 == 0) && 
			 (GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) == false && 
			 m_flNextPrimaryAttack < gpGlobals->curtime && 
			 m_flNextSecondaryAttack < gpGlobals->curtime )
		{
			// if we're successfully reloading, we're done
			if ( Reload() )
				return true;
		}
	}

	return false;
}