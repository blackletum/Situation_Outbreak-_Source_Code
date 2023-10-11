//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		zombie - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "zms/weapon_zombie.h"
#include "hl2mp/hl2mp_gamerules.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
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
	#include "monstermaker.h"
	#include "npc_BaseZombie.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	ZOMBIE_RANGE			60.0f
#define	ZOMBIE_COMMAND_RANGE	10000.0f

extern ConVar so_gamemode;

//-----------------------------------------------------------------------------
// CWeaponZombie
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZombie, DT_WeaponZombie )

BEGIN_NETWORK_TABLE( CWeaponZombie, DT_WeaponZombie )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZombie )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zombie, CWeaponZombie );
PRECACHE_WEAPON_REGISTER( weapon_zombie );

//#ifndef CLIENT_DLL

acttable_t	CWeaponZombie::m_acttable[] = 
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

IMPLEMENT_ACTTABLE(CWeaponZombie);

//#endif

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponZombie::CWeaponZombie( void )
{
	//m_bDelayedAttack = false;
	//m_flDelayedAttackTime = 0.0f;
	m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );
}

void CWeaponZombie::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) /*&& (!m_bDelayedAttack)*/ )
	{
		PrimaryAttack();

		//m_bDelayedAttack = true;
		//m_flDelayedAttackTime = gpGlobals->curtime + 0.1f;
	} 
	else if ( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		SecondaryAttack();
	}
	else if ( (pOwner->m_nButtons & IN_RELOAD) && (FStrEq(so_gamemode.GetString(), "Overlord")) && (m_flNextNPCCreateTime <= gpGlobals->curtime) )
	{
		int iMoney = pOwner->m_iMoney;
		if ( iMoney >= 200 )
		{
			NPCMakerSpawn();
		}
		else
		{
			ClientPrint( pOwner, HUD_PRINTCENTER, "You don't have enough points to spawn another zombie!" );
		}

		m_flNextNPCCreateTime = gpGlobals->curtime + 1.2;
	}
	else 
	{
		WeaponIdle();
	}

	// Check to see if we should attack (with a small delay)
	//DelayedAttackCheck();
}

void CWeaponZombie::NPCMakerSpawn( void )
{
	trace_t traceHitSpawn;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;
	
#ifndef CLIENT_DLL
	int iMoney = pOwner->m_iMoney;

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	const char * m_iszNPCClassname = "npc_zombie";

	int randzombienpcspawner = RandomInt(1,4);
	switch ( randzombienpcspawner )
	{
		case 1:
			m_iszNPCClassname = "npc_zombie";
			break;

		case 2:
			m_iszNPCClassname = "npc_fastzombie";
			break;

		case 3:
			m_iszNPCClassname = "npc_poisonzombie";
			break;

		case 4:
			m_iszNPCClassname = "npc_reaper_nojump";
			break;

		// Too dangerous at the moment.
		/*case 5:
			m_iszNPCClassname = "npc_sploderzombie";
			break;*/
	}

	CAI_BaseNPC *pZombieNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName( m_iszNPCClassname ) );

	if ( pZombieNPC )
	{
		pZombieNPC->Precache();
		DispatchSpawn( pZombieNPC );

		QAngle angles;
		trace_t tr;
		Vector forward;
		pOwner->EyeVectors( &forward, NULL, NULL );
		VectorAngles( forward, angles );
		angles.x = 0; 
		angles.z = 0;
		AI_TraceLine( pOwner->EyePosition(), pOwner->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, pOwner, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
			tr.endpos.z += 12;
			pZombieNPC->Teleport( &tr.endpos, &angles, NULL );
			UTIL_DropToFloor( pZombieNPC, MASK_NPCSOLID );

			Vector	vUpBit = pZombieNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( pZombieNPC->GetAbsOrigin(), vUpBit, pZombieNPC->GetHullMins(), pZombieNPC->GetHullMaxs(), MASK_NPCSOLID, pZombieNPC, COLLISION_GROUP_NONE, &tr );

			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				pZombieNPC->SUB_Remove();
				return;
			}

			pOwner->m_iMoney = iMoney - 200;
		}
		else
		{
			pZombieNPC->Teleport( NULL, &angles, NULL );
		}

		pZombieNPC->Activate();
	}

	CBaseEntity::SetAllowPrecache( allowPrecache );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponZombie::GetDamageForActivity( Activity hitActivity )
{
	return 25.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponZombie::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = SharedRandomFloat( "zombiepax", 1.0f, 2.0f );
	punchAng.y = SharedRandomFloat( "zombiepay", -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the zombie!)
//-----------------------------------------------------------------------------
ConVar sk_zombie_lead_time( "sk_zombie_lead_time", "0.9" );

int CWeaponZombie::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the zombie!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = sk_zombie_lead_time.GetFloat();
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
void CWeaponZombie::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
void CWeaponZombie::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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
void CWeaponZombie::Drop( const Vector &vecVelocity )
{
	// This weapon cannot be dropped.
}

float CWeaponZombie::GetRange( void )
{
	return		ZOMBIE_RANGE;
}

float CWeaponZombie::GetRangeZombieCommand( void )
{
	return		ZOMBIE_COMMAND_RANGE;
}

float CWeaponZombie::GetFireRate( void )
{
	return 1.0f;
}

float CWeaponZombie::GetDamage( void )
{
	return 25.0f;
}

/////////////////////////////////////////////////////////////////////////
// S:O - Zombie Claws - Primary Attack
//
// OBJECTIVE GAMEMODE: A slash attack that does approximately 50 damage.
// ALL OTHER GAMEMODES: A slash attack that does approximately 25 damage.
/////////////////////////////////////////////////////////////////////////

/*void CWeaponZombie::DelayedAttackCheck()
{
	if ( m_bDelayedAttack && (m_flDelayedAttackTime <= gpGlobals->curtime) )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		trace_t traceHit;

		Vector swingStart = pOwner->Weapon_ShootPosition( );
		Vector forward;

		pOwner->EyeVectors( &forward, NULL, NULL );

		Vector swingEnd = swingStart + forward * GetRange();
		UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );

		Hit( traceHit );

		pOwner->SetAnimation( PLAYER_ATTACK1 );
		ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

		AddViewKick();

		m_bDelayedAttack = false;
	}
}*/

void CWeaponZombie::PrimaryAttack()
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	Swing();

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

void CWeaponZombie::Swing()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	trace_t traceHit;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );

	Hit( traceHit );

	pOwner->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	AddViewKick();

	Activity nHitActivity = ACT_VM_HITCENTER;
	SendWeaponAnim( nHitActivity );

	// Now delayed along with the call to our Hit() function
	//pOwner->SetAnimation( PLAYER_ATTACK1 );
	//ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75f;
}

void CWeaponZombie::Hit(trace_t &traceHit)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	WeaponSound( SINGLE );

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;
	if ( pHitEntity )
	{
		Vector hitDirection;
		pOwner->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );
	
#ifndef CLIENT_DLL
		// Don't allow us to do anything to a fellow zombie, whether they be a player or NPC.
		if ( (pHitEntity->IsPlayer() || pHitEntity->IsNPC()) && (pHitEntity->Classify() == CLASS_ZOMBIE) )
			return;

		if ( FStrEq(so_gamemode.GetString(), "Overlord") )
		{
			CTakeDamageInfo info( GetOwner(), GetOwner(), 50, DMG_CLUB );
			CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );
			pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
			ApplyMultiDamage();
			TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
		}
		else
		{
			CTakeDamageInfo info( GetOwner(), GetOwner(), GetDamage(), DMG_CLUB );
			CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );
			pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
			ApplyMultiDamage();
			TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
		}

		// The days of instant infection are over, I think!
		/*if( pHitEntity->IsPlayer() && !FStrEq(so_gamemode.GetString(), "Overlord") )
		{
			int InfectionRand = random->RandomInt(1,3);
			if ( InfectionRand == 3 )
			{
				CHL2MP_Player *pPlayer = dynamic_cast<CHL2MP_Player*>( pHitEntity );

				if ( pPlayer->GetTeamNumber() == 3 )
				{
					CHL2MPRules *pRules = HL2MPRules();
					if ( !pRules )
						return;

					pRules->ZombifyPlayer(pPlayer);

					CHL2MP_Player *pOwnerPlayer = dynamic_cast<CHL2MP_Player*>( pOwner );
					// S:O - Award players some experience for their infect.
					pOwnerPlayer->AddXP(5);

					// S:O - Award players some money for their infect.
					int money = pOwnerPlayer->m_iMoney;
					pOwnerPlayer->m_iMoney = money + 500;

					pOwner->IncrementFragCount(1);

					IGameEvent *event = gameeventmanager->CreateEvent( "ZombifyPlayer" );

					if( event )
					{
						event->SetInt("userid", pPlayer->GetUserID() );
						event->SetInt("attacker", pOwner->GetUserID() );
						event->SetString("weapon", "weapon_zombie" );
						event->SetInt( "priority", 7 );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}*/
#endif
	}

	ImpactEffect( traceHit );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// S:O - Zombie Claws - Secondary Attack
//
// OBJECTIVE GAMEMODE: Commands allied zombie NPCs.
// ALL OTHER GAMEMODES: A fierce attack that pushes enemy players and NPCs backwards.
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CWeaponZombie::SecondaryAttack()
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	SwingSecondary();

	if ( !FStrEq(so_gamemode.GetString(), "Overlord") )
		AddViewKick();

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

void CWeaponZombie::SwingSecondary()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

	if ( FStrEq(so_gamemode.GetString(), "Overlord") )
	{
		trace_t traceHitZombieCommand;
		Vector swingStart = pOwner->Weapon_ShootPosition( );
		Vector forward;
		pOwner->EyeVectors( &forward, NULL, NULL );
		Vector swingEndZombieCommand = swingStart + forward * GetRangeZombieCommand();
		UTIL_TraceLine( swingStart, swingEndZombieCommand, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_DEBRIS, &traceHitZombieCommand );
		Vector endPos = traceHitZombieCommand.endpos;

		// S:O - Custom animation and sound for commanding NPCs, just to make things interesting.
		SendWeaponAnim( ACT_VM_FIDGET );
		WeaponSound( SPECIAL1 );
		ZombieCommand( traceHitZombieCommand, endPos );
	}
	else
	{
		SendWeaponAnim( ACT_VM_MISSCENTER );
		WeaponSound( SINGLE );
		HitSecondary();
	}

	pOwner->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75f;
}

void CWeaponZombie::ZombieCommand( trace_t &traceHitZombieCommand, Vector &endPos )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		Vector hitDirection;
		pOwner->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );
	
		if ( traceHitZombieCommand.fraction != 1.0 )
		{
#ifndef CLIENT_DLL
			CNPC_BaseZombie *pSelector = NULL;

			for ( int i = 0; i < gEntList.m_ZombieList.Count(); i++)
			{
				pSelector = dynamic_cast<CNPC_BaseZombie*>( gEntList.m_ZombieList[i] );

				if ( pSelector )
				{
					pSelector->SetEnemy( NULL );
					pSelector->SetSchedule( SCHED_ZOMBIE_AMBUSH_MODE );
					CAI_BaseNPC::ConqCommanded(endPos, hitDirection, true, pSelector);
				}
			}
//#else
			CDisablePredictionFiltering disabler;
			CSingleUserRecipientFilter user( pOwner );
			user.MakeReliable();

			te->BeamRingPoint( user, 0, 
				endPos,//origin
				1,//start radius
				16,//end radius
				m_spriteTexture, //texture
				3,//halo index
				0,//start frame
				3,//framerate
				3.0f,//life
				128,//width
				16,//spread
				0,//amplitude
				150,//r
				0,//g
				0,//b
				10,//a
				50//speed
			);
#endif
		}

		pOwner->EmitSound( "Weapon_Zombie.Command" );
	}
}

void CWeaponZombie::HitSecondary()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

#ifndef CLIENT_DLL
	CBaseEntity *pEntList[100];
	int numEnts = UTIL_EntitiesInSphere( pEntList, 100, pOwner->GetAbsOrigin(), 75.0f, 0 );
	for ( int i = 0; i < numEnts; i++ )
	{
		// If it is not a player or NPC, ignore it
		if ( !pEntList[i]->IsPlayer() && !pEntList[i]->IsNPC() )
			continue;

		// If it is a fellow zombie of some kind, ignore it
		if ( pEntList[i]->Classify() == CLASS_ZOMBIE )
			continue;

		// If it isn't alive, ignore it
		if ( !pEntList[i]->IsAlive() )
			continue;

		float yawKick = random->RandomFloat( -10, 10 );
		float yawDirection = random->RandomFloat( -10, 10 );

		// Kick the owner's angles
		pOwner->ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

		// Kick the target's angles, too
		pEntList[i]->ViewPunch( QAngle( yawDirection, yawKick, 2 ) );

		Vector dir = pEntList[i]->GetAbsOrigin() - pOwner->GetAbsOrigin();
		VectorNormalize( dir );
		QAngle angles;
		VectorAngles( dir, angles );
		Vector forward;
		AngleVectors( angles, &forward, NULL, NULL );

		// Throw the target backwards and make a sound
		pEntList[i]->ApplyAbsVelocityImpulse( forward * 1000.0f );
		pEntList[i]->EmitSound( "NPC_Metropolice.Shove" );
	}
#endif
}