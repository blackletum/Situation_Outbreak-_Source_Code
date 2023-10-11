//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the zombie, a horrific once-human headcrab victim.
//
// The zombie has two main states: Full and Torso.
//
// In Full state, the zombie is whole and walks upright as he did in Half-Life.
// He will try to claw the player and swat physics items at him. 
//
// In Torso state, the zombie has been blasted or cut in half, and the Torso will
// drag itself along the ground with its arms. It will try to claw the player.
//
// In either state, a severely injured Zombie will release its headcrab, which
// will immediately go after the player. The Zombie will then die (ragdoll). 
//
//=============================================================================//

#include "cbase.h"
#include "npc_BaseZombie.h"
#include "player.h"
#include "game.h"
#include "ai_network.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_memory.h"
#include "ai_senses.h"
#include "bitstring.h"
#include "EntityFlame.h"
#include "hl2_shareddefs.h"
#include "npcevent.h"
#include "activitylist.h"
#include "entitylist.h"
#include "gib.h"
//#include "soundenvelope.h"
#include "ndebugoverlay.h"
#include "rope.h"
#include "rope_shared.h"
#include "igamesystem.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "hl2_gamerules.h"
#include "weapon_physcannon.h"
#include "ammodef.h"
#include "vehicle_base.h"
#include "hl2mp/hl2mp_gamerules.h"
//#include "doors.h"

// S:O - Additional Includes
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "npc_combines.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_npc_head;
#define ZOMBIE_BULLET_DAMAGE_SCALE 0.5f

int g_interactionZombieMeleeWarning;

/*envelopePoint_t envDefaultZombieMoanVolumeFast[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envDefaultZombieMoanVolume[] =
{
	{	1.0f, 0.1f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};*/


// if the zombie doesn't find anything closer than this, it doesn't swat.
#define ZOMBIE_FARTHEST_PHYSICS_OBJECT 80.0*12.0
#define ZOMBIE_PHYSICS_SEARCH_DEPTH 100

// Don't swat objects unless player is closer than this.
// S:O - Make this pretty far!
#define ZOMBIE_PLAYER_MAX_SWAT_DIST 10000

//
// The heaviest physics object that a zombie should try to swat. (kg)
// S:O - Our zombies are CRAZY!!
#define ZOMBIE_MAX_PHYSOBJ_MASS 1000

//
// Zombie tries to get this close to a physics object's origin to swat it
#define ZOMBIE_PHYSOBJ_SWATDIST		80

//
// Because movement code sometimes doesn't get us QUITE where we
// want to go, the zombie tries to get this close to a physics object
// Zombie will end up somewhere between PHYSOBJ_MOVE_TO_DIST & PHYSOBJ_SWATDIST
#define ZOMBIE_PHYSOBJ_MOVE_TO_DIST	48

//
// How long between physics swat attacks (in seconds). 
// S:O - GO CRAZY!!
#define ZOMBIE_SWAT_DELAY			1


//
// After taking damage, ignore further damage for n seconds. This keeps the zombie
// from being interrupted while.
//
#define ZOMBIE_FLINCH_DELAY			3


#define ZOMBIE_BURN_TIME		10 // If ignited, burn for this many seconds
#define ZOMBIE_BURN_TIME_NOISE	2  // Give or take this many seconds.

//=========================================================
// private activities
//=========================================================
int CNPC_BaseZombie::ACT_ZOM_SWATLEFTMID;
int CNPC_BaseZombie::ACT_ZOM_SWATRIGHTMID;
int CNPC_BaseZombie::ACT_ZOM_SWATLEFTLOW;
int CNPC_BaseZombie::ACT_ZOM_SWATRIGHTLOW;
int CNPC_BaseZombie::ACT_ZOM_FALL;

//ConVar	sk_zombie_dmg_one_slash( "sk_zombie_dmg_one_slash","0");
//ConVar	sk_zombie_dmg_both_slash( "sk_zombie_dmg_both_slash","0");

// When a zombie spawns, he will select a 'base' pitch value
// that's somewhere between basepitchmin & basepitchmax
ConVar zombie_basemin( "zombie_basemin", "100" );
ConVar zombie_basemax( "zombie_basemax", "100" );

ConVar zombie_changemin( "zombie_changemin", "0" );
ConVar zombie_changemax( "zombie_changemax", "0" );

// play a sound once in every zombie_stepfreq steps
ConVar zombie_stepfreq( "zombie_stepfreq", "4" );
ConVar zombie_moanfreq( "zombie_moanfreq", "1" );

ConVar zombie_decaymin( "zombie_decaymin", "0.1" );
ConVar zombie_decaymax( "zombie_decaymax", "0.4" );

ConVar zombie_ambushdist( "zombie_ambushdist", "16000" );

// S:O - Additional ConVars
extern ConVar so_dyndiff_level;
extern ConVar so_gamemode;
extern ConVar so_infection_system;
extern ConVar so_infection_chance;

//=========================================================
// For a couple of reasons, we keep a running count of how
// many zombies in the world are angry at any given time.
//=========================================================
static int s_iAngryZombies = 0;

//=========================================================
//=========================================================
class CAngryZombieCounter : public CAutoGameSystem
{
public:
	CAngryZombieCounter( char const *name ) : CAutoGameSystem( name )
	{
	}
	// Level init, shutdown
	virtual void LevelInitPreEntity()
	{
		s_iAngryZombies = 0;
	}
};

CAngryZombieCounter	AngryZombieCounter( "CAngryZombieCounter" );


int AE_ZOMBIE_ATTACK_RIGHT;
int AE_ZOMBIE_ATTACK_LEFT;
int AE_ZOMBIE_ATTACK_BOTH;
int AE_ZOMBIE_SWATITEM;
int AE_ZOMBIE_STARTSWAT;
int AE_ZOMBIE_STEP_LEFT;
int AE_ZOMBIE_STEP_RIGHT;
int AE_ZOMBIE_SCUFF_LEFT;
int AE_ZOMBIE_SCUFF_RIGHT;
int AE_ZOMBIE_ATTACK_SCREAM;
int AE_ZOMBIE_GET_UP;
int AE_ZOMBIE_POUND;
int AE_ZOMBIE_ALERTSOUND;


//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_BaseZombie )

	DEFINE_SOUNDPATCH( m_pMoanSound ),
	DEFINE_FIELD( m_fIsHeadless, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( m_bHeadShot, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flBurnDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBurnDamageResetTime, FIELD_TIME ),
	DEFINE_FIELD( m_hPhysicsEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextMoanSound, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSwat, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSwatScan, FIELD_TIME ),
	DEFINE_FIELD( HoldoutTriggerAmbushDelay, FIELD_TIME ),
	DEFINE_FIELD( m_flMoanPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_iMoanSound, FIELD_INTEGER ),
	DEFINE_FIELD( m_hObstructor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bIsSlumped, FIELD_BOOLEAN ),

END_DATADESC()

//LINK_ENTITY_TO_CLASS( base_zombie, CNPC_BaseZombie );

// S:O - Connect to new client version
//IMPLEMENT_SERVERCLASS_ST(CNPC_BaseZombie, DT_NPC_BaseZombie)
//END_SEND_TABLE()

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::g_numZombies = 0;


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::CNPC_BaseZombie()
{
	// Gotta select which sound we're going to play, right here!
	// Because everyone's constructed before they spawn.
	//
	// Assign moan sounds in order, over and over.
	// This means if 3 or so zombies spawn near each
	// other, they will definitely not pick the same
	// moan loop.
	m_iMoanSound = g_numZombies;
	numzombies = 0;
	zombies = 0;
	g_numZombies++;

	m_fFadeFinish = 0;

	// Add this zombie to our global zombie list!
	gEntList.m_ZombieList.AddToTail(this);
}


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::~CNPC_BaseZombie()
{
	g_numZombies--;

	// Remove this zombie from our global zombie list!
	// NOTE: It's dead.
	gEntList.m_ZombieList.FindAndRemove(this);
}


//---------------------------------------------------------
// The closest physics object is chosen that is:
// <= MaxMass in Mass
// Between the zombie and the enemy
// not too far from a direct line to the enemy.
//---------------------------------------------------------
bool CNPC_BaseZombie::FindNearestPhysicsObject( int iMaxMass, int iMaxDistance )
{
	CBaseEntity		*pList[ ZOMBIE_PHYSICS_SEARCH_DEPTH ];
	CBaseEntity		*pNearest = NULL;
	float			flDist;
	IPhysicsObject	*pPhysObj;
	int				i;
	Vector			vecDirToEnemy;
	Vector			vecDirToObject;
	Vector			vecTargetPos;

	if (!CanSwatPhysicsObjects())
	{
		m_hPhysicsEnt = NULL;
		return false;
	}
	else if ( !GetEnemy() )
	{
		vecTargetPos = GetNavigator()->GetGoalPos();
	}
	else
		vecTargetPos = GetEnemy()->GetAbsOrigin();
	
	if (iMaxDistance == 0) iMaxDistance = ZOMBIE_FARTHEST_PHYSICS_OBJECT;

	vecDirToEnemy = vecTargetPos - GetAbsOrigin();
	float dist = VectorNormalize(vecDirToEnemy);
	vecDirToEnemy.z = 0;

	if( dist > ZOMBIE_PLAYER_MAX_SWAT_DIST )
	{
		return false;
	}

	float flNearestDist = min( dist, iMaxDistance * 0.5 );
	Vector vecDelta( flNearestDist, flNearestDist, GetHullHeight() * 2.0 );

	class CZombieSwatEntitiesEnum : public CFlaggedEntitiesEnum
	{
	public:
		CZombieSwatEntitiesEnum( CBaseEntity **pList, int listMax, int iMaxMass )
		 :	CFlaggedEntitiesEnum( pList, listMax, 0 ),
			m_iMaxMass( iMaxMass )
		{
		}

		virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
		{
			CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
			if ( pEntity && 
				 pEntity->VPhysicsGetObject() && 
				 pEntity->VPhysicsGetObject()->GetMass() <= m_iMaxMass && 
				 pEntity->VPhysicsGetObject()->IsAsleep() && 
				 pEntity->VPhysicsGetObject()->IsMoveable() )
			{
				return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
			}
			return ITERATION_CONTINUE;
		}

		int m_iMaxMass;
	};

	CZombieSwatEntitiesEnum swatEnum( pList, ZOMBIE_PHYSICS_SEARCH_DEPTH, iMaxMass );

	int count = UTIL_EntitiesInBox( GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, &swatEnum );

	Vector vecZombieKnees;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.25f ), &vecZombieKnees );

	for( i = 0 ; i < count ; i++ )
	{
		pPhysObj = pList[ i ]->VPhysicsGetObject();

		Assert( !( !pPhysObj || pPhysObj->GetMass() > iMaxMass || !pPhysObj->IsAsleep() ) );

		Vector center = pList[ i ]->WorldSpaceCenter();
		flDist = UTIL_DistApprox2D( GetAbsOrigin(), center );

		if( flDist >= flNearestDist )
			continue;

		vecDirToObject = pList[ i ]->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize(vecDirToObject);
		vecDirToObject.z = 0;

		if( DotProduct( vecDirToEnemy, vecDirToObject ) < 0.8 )
		{
			continue;
		}

		if( flDist >= UTIL_DistApprox2D( center, vecTargetPos ) )
		{
			continue;
		}

		if ( (center.z + pList[i]->BoundingRadius()) < vecZombieKnees.z )
		{
			continue;
		}

		if( center.z > EyePosition().z )
		{
			continue;
		}

		if ( !FVisible( pList[i] ) )
		{
			continue;
		}

		if ( FClassnameIs( pList[ i ], "physics_prop_ragdoll" ) )
			continue;
			
		if ( FClassnameIs( pList[ i ], "prop_ragdoll" ) )
			continue;

		pNearest = pList[ i ];
		flNearestDist = flDist;
	}

	m_hPhysicsEnt = pNearest;

	if( m_hPhysicsEnt == NULL )
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CNPC_BaseZombie::SwatObject( IPhysicsObject *pPhysObj, Vector direction )
{
	const float targetmass = pPhysObj->GetMass();

	const float liftforce = RemapVal( targetmass, 5, 350, 3000, 20000 );
	Vector uplift = Vector(0, 0, liftforce);

	const float swatforce = RemapVal( targetmass, 5, 500, 20000, 70000 );
	pPhysObj->ApplyForceCenter( direction * swatforce + uplift);
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseZombie::Classify( void )
{
	if ( IsSlumped() )
		return CLASS_NONE;

	return( CLASS_ZOMBIE ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_BaseZombie::IRelationType( CBaseEntity *pTarget )
{
	// Slumping should not affect Zombie's opinion of others
	if ( IsSlumped() )
	{
		m_bIsSlumped = false;
		Disposition_t result = BaseClass::IRelationType( pTarget );
		m_bIsSlumped = true;
		return result;
	}

	return BaseClass::IRelationType( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the maximum yaw speed based on the monster's current activity.
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::MaxYawSpeed( void )
{
	if (IsMoving() && HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
	{
		return( 15 );
	}
	else
	{
		switch( GetActivity() )
		{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			return 100;
			break;
		case ACT_RUN:
			return 15;
			break;
		case ACT_WALK:
		case ACT_IDLE:
			return 25;
			break;
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 120;
		default:
			return 90;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
	{
		return BaseClass::OverrideMoveFacing( move, flInterval );
	}

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );
	float idealYaw = UTIL_AngleMod( flMoveYaw );

	if (GetEnemy())
	{
		float flEDist = UTIL_DistApprox2D( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter() );

		if (flEDist < 256.0)
		{
			float flEYaw = UTIL_VecToYaw( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );

			if (flEDist < 128.0)
			{
				idealYaw = flEYaw;
			}
			else
			{
				idealYaw = flMoveYaw + UTIL_AngleDiff( flEYaw, flMoveYaw ) * (2 - flEDist / 128.0);
			}

			//DevMsg("was %.0f now %.0f\n", flMoveYaw, idealYaw );
		}
	}

	GetMotor()->SetIdealYawAndUpdate( idealYaw );

	// find movement direction to compensate for not being turned far enough
	float fSequenceMoveYaw = GetSequenceMoveYaw( GetSequence() );
	float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y + fSequenceMoveYaw );
	SetPoseParameter( m_poseMove_Yaw, GetPoseParameter( m_poseMove_Yaw ) + flDiff );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	bool fReturn = BaseClass::FVisible( pEntity, traceMask, ppBlocker );

	// S:O - Zombies can always see any player on the map if they are stimulated enough!
	if ( pEntity->IsPlayer() )
	{
		ClearCondition( COND_ENEMY_OCCLUDED );
		return true;
	}
	else
		return fReturn;
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseZombie::MeleeAttack1Conditions ( float flDot, float flDist )
{
	float range = GetClawAttackRange();

	if (flDist > range )
	{
		/*// Translate a hit vehicle into its passenger if found
		if ( GetEnemy() != NULL )
		{
			// If the player is holding an object, knock it down.
			if( GetEnemy()->IsPlayer() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );

				Assert( pPlayer != NULL );

				// Is the player carrying something?
				CBaseEntity *pObject = GetPlayerHeldEntity( pPlayer );

				if( !pObject )
				{
					pObject = PhysCannonGetHeldEntity( pPlayer->GetActiveWeapon() );
				}

				if( pObject )
				{
					float flDist = pObject->WorldSpaceCenter().DistTo( WorldSpaceCenter() );

					if( flDist <= GetClawAttackRange() )
						return COND_CAN_MELEE_ATTACK1;
				}
			}
		}*/
		return COND_TOO_FAR_TO_ATTACK;
	}

	if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	// Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	trace_t	tr;
	CTraceFilterNav traceFilter( this, false, this, COLLISION_GROUP_NONE );
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * GetClawAttackRange(), vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

	if( tr.fraction == 1.0 || !tr.m_pEnt )
	{

#ifdef HL2_EPISODIC

		// If our trace was unobstructed but we were shooting 
		if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
			return COND_CAN_MELEE_ATTACK1;

#endif // HL2_EPISODIC

		// This attack would miss completely. Trick the zombie into moving around some more.
		return COND_TOO_FAR_TO_ATTACK;
	}

	if( tr.m_pEnt == GetEnemy() || 
		tr.m_pEnt->IsNPC() || 
		( tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt) ) ) )
	{
		// -Let the zombie swipe at his enemy if he's going to hit them.
		// -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
		//  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
		// -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
		return COND_CAN_MELEE_ATTACK1;
	}

	Vector vecTrace = tr.endpos - tr.startpos;
	float lenTraceSq = vecTrace.Length2DSqr();

	if ( GetEnemy() && GetEnemy()->MyCombatCharacterPointer() && tr.m_pEnt == static_cast<CBaseCombatCharacter *>(GetEnemy())->GetVehicleEntity() )
	{
		if ( lenTraceSq < Square( GetClawAttackRange() * 0.75f ) )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}

	if( tr.m_pEnt->IsBSPModel() )
	{
		// The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
		// the enemy is, treat this as an obstruction.
		Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();

		if( lenTraceSq < vecToEnemy.Length2DSqr() )
		{
			return COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION;
		}
	}

#ifdef HL2_EPISODIC

	if ( !tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
	{
		//Try to swat whatever the player is standing on instead of acting like a dill.
		return COND_CAN_MELEE_ATTACK1;
	}

	// Bullseyes are given some grace on if they can be hit
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
		return COND_CAN_MELEE_ATTACK1;

#endif // HL2_EPISODIC

	// Move around some more
	return COND_TOO_FAR_TO_ATTACK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST	96.0f // Triple damage from buckshot at 8 feet (headshot only)
float CNPC_BaseZombie::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info )
{
	switch( iHitGroup )
	{
	case HITGROUP_HEAD:
		{
			if( info.GetDamageType() & DMG_BUCKSHOT )
			{
				float flDist = FLT_MAX;

				if( info.GetAttacker() )
				{
					flDist = ( GetAbsOrigin() - info.GetAttacker()->GetAbsOrigin() ).Length();
				}

				if( flDist <= ZOMBIE_BUCKSHOT_TRIPLE_DAMAGE_DIST )
				{
					return 3.0f;
				}
			}
			else
			{
				return 2.0f;
			}
		}
	}

	return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo infoCopy = info;

	// S:O - HEADSHOT!
	// Headshots do a lot of damage to zombies. A lot.
	int bloodColor = BloodColor();
	if ( ptr->hitgroup == HITGROUP_HEAD )
	{
		AddMultiDamage( info, this );
		UTIL_BloodSpray( ptr->endpos, vecDir, bloodColor, 5, FX_BLOODSPRAY_DROPS | FX_BLOODSPRAY_CLOUD );
	}

	if( infoCopy.GetDamageType() & DMG_BUCKSHOT )
	{
		// Zombie gets across-the-board damage reduction for buckshot. This compensates for the recent changes which
		// make the shotgun much more powerful, and returns the zombies to a level that has been playtested extensively.(sjb)
		// This normalizes the buckshot damage to what it used to be on normal (5 dmg per pellet. Now it's 8 dmg per pellet). 
		infoCopy.ScaleDamage( 0.625 );
	}

	UTIL_BloodImpact( ptr->endpos, vecDir, bloodColor, 100 );
	UTIL_BloodDrips( ptr->endpos, vecDir, bloodColor, 100 );

	BaseClass::TraceAttack( infoCopy, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
#define ZOMBIE_SCORCH_RATE		8
#define ZOMBIE_MIN_RENDERCOLOR	50
int CNPC_BaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// S:O - Attempted a fix to credit vehicle kills with player...
	if( inputInfo.GetDamageType() & DMG_VEHICLE && inputInfo.GetAttacker() && !inputInfo.GetAttacker()->IsPlayer() )
	{
		CPropVehicleDriveable *pVehicle = dynamic_cast< CPropVehicleDriveable* >( inputInfo.GetAttacker() );

		if( pVehicle )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pVehicle->GetDriver() );

			if( pPlayer )
			{
				info.SetAttacker( pPlayer );
				info.SetInflictor( pVehicle );

				TakeDamage( info );
				return 0;
			}
		}
	}

	CHL2MP_Player *pAttacker = dynamic_cast<CHL2MP_Player*>( inputInfo.GetAttacker() );

	if ( pAttacker )
	{
		// S:O - Award players some money for their damage
		int addmoneyratio;

		// S:O - Give players more money depending on the difficulty
		if ( so_dyndiff_level.GetInt() == 0 )
			addmoneyratio = inputInfo.GetDamage() / 1.0;
		else if ( so_dyndiff_level.GetInt() == 1 )
			addmoneyratio = inputInfo.GetDamage() / 0.9;
		else if ( so_dyndiff_level.GetInt() == 2 )
			addmoneyratio = inputInfo.GetDamage() / 0.8;
		else if ( so_dyndiff_level.GetInt() == 3 )
			addmoneyratio = inputInfo.GetDamage() / 0.7;
		else if ( so_dyndiff_level.GetInt() == 4 )
			addmoneyratio = inputInfo.GetDamage() / 0.6;
		else if ( so_dyndiff_level.GetInt() == 5 )
			addmoneyratio = inputInfo.GetDamage() / 0.5;
		else
			addmoneyratio = 0;

		int m_iCurrentMoney = pAttacker->m_iMoney;
		pAttacker->m_iMoney = m_iCurrentMoney + addmoneyratio;
	}

	if( inputInfo.GetDamageType() & DMG_BURN )
	{
		// If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
		// This is to stop zombies from burning to death 10x faster when they're standing around
		// 10 fire entities.
		if( IsOnFire() && !(inputInfo.GetDamageType() & DMG_DIRECT) )
		{
			return 0;
		}
		
		Scorch( ZOMBIE_SCORCH_RATE, ZOMBIE_MIN_RENDERCOLOR );
	}

	// Take some percentage of damage from bullets (unless hit in the crab). Always take full buckshot & sniper damage
	if ( !m_bHeadShot && (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & (DMG_BUCKSHOT|DMG_SNIPER)) )
	{
		info.ScaleDamage( ZOMBIE_BULLET_DAMAGE_SCALE );
	}

	if ( ShouldIgnite( info ) )
	{
		Ignite( 100.0f );
	}

	int tookDamage = BaseClass::OnTakeDamage_Alive( info );
	
	// Being chopped up by a sharp physics object is a pretty special case
	// so we handle it with some special code. Mainly for 
	// Ravenholm's helicopter traps right now (sjb).
	bool bChopped = IsChopped(info);
	bool bSquashed = IsSquashed(info);
	bool bKilledByVehicle = ( ( info.GetDamageType() & DMG_VEHICLE ) != 0 );

	if( (bChopped || bSquashed) && !bKilledByVehicle && !(info.GetDamageType() & DMG_REMOVENORAGDOLL) )
	{
		if( bChopped )
		{
			EmitSound( "E3_Phystown.Slicer" );
		}

		DieChopped( info );
	}

	if( tookDamage > 0 && (info.GetDamageType() & (DMG_BURN|DMG_DIRECT)) && m_ActBusyBehavior.IsActive() ) 
	{
		//!!!HACKHACK- Stuff a light_damage condition if an actbusying zombie takes direct burn damage. This will cause an
		// ignited zombie to 'wake up' and rise out of its actbusy slump. (sjb)
		SetCondition( COND_LIGHT_DAMAGE );
	}

	// IMPORTANT: always clear the headshot flag after applying damage. No early outs!
	m_bHeadShot = false;

	return tookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: make a sound Alyx can hear when in darkness mode
// Input  : volume (radius) of the sound.
// Output :
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::MakeAISpookySound( float volume, float duration )
{
/*	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		CSoundEnt::InsertSound( SOUND_COMBAT, EyePosition(), volume, duration, this, SOUNDENT_CHANNEL_SPOOKY_NOISE );
	}*/
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::CanPlayMoanSound()
{
	if( HasSpawnFlags( SF_NPC_GAG ) )
		return false;

	// Burning zombies play their moan loop at full volume for as long as they're
	// burning. Don't let a moan envelope play cause it will turn the volume down when done.
	if( IsOnFire() )
		return false;

	// Members of a small group of zombies can vocalize whenever they want
	if( s_iAngryZombies <= 4 )
		return true;

	// This serves to limit the number of zombies that can moan at one time when there are a lot. 
	if( random->RandomInt( 1, zombie_moanfreq.GetInt() * (s_iAngryZombies/2) ) == 1 )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Open a window and let a little bit of the looping moan sound
//			come through.
//-----------------------------------------------------------------------------
/*void CNPC_BaseZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( HasSpawnFlags( SF_NPC_GAG ) )
	{
		// Not yet!
		return;
	}

	if( !m_pMoanSound )
	{
		// Don't set this up until the code calls for it.
		const char *pszSound = GetMoanSound( m_iMoanSound );
		m_flMoanPitch = random->RandomInt( zombie_basemin.GetInt(), zombie_basemax.GetInt() );

		//m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( entindex(), CHAN_STATIC, pszSound, ATTN_NORM );
		CPASAttenuationFilter filter( this );
		m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, ATTN_NORM );

		ENVELOPE_CONTROLLER.Play( m_pMoanSound, 1.0, m_flMoanPitch );
	}

	//HACKHACK get these from chia chin's console vars.
	envDefaultZombieMoanVolumeFast[ 1 ].durationMin = zombie_decaymin.GetFloat();
	envDefaultZombieMoanVolumeFast[ 1 ].durationMax = zombie_decaymax.GetFloat();

	if( random->RandomInt( 1, 2 ) == 1 )
	{
		IdleSound();
	}

	float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, pEnvelope, iEnvelopeSize );

	float flPitch = random->RandomInt( m_flMoanPitch + zombie_changemin.GetInt(), m_flMoanPitch + zombie_changemax.GetInt() );
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, flPitch, 0.3 );

	m_flNextMoanSound = gpGlobals->curtime + duration + 9999;
}*/

//-----------------------------------------------------------------------------
// Purpose: Determine whether the zombie is chopped up by some physics item
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::IsChopped( const CTakeDamageInfo &info )
{
	float flDamageThreshold = min( 1, info.GetDamage() / m_iMaxHealth );

	if ( m_iHealth > 0 || flDamageThreshold <= 0.5 )
		return false;

	if ( !( info.GetDamageType() & DMG_SLASH) )
		return false;

	if ( !( info.GetDamageType() & DMG_CRUSH) )
		return false;

	if ( info.GetDamageType() & DMG_REMOVENORAGDOLL )
		return false;

	// If you take crush and slash damage, you're hit by a sharp physics item.
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if this gibbing zombie should ignite its gibs
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgniteZombieGib( void )
{
#ifdef HL2_EPISODIC
	// If we're in darkness mode, don't ignite giblets, because we don't want to
	// pay the perf cost of multiple dynamic lights per giblet.
	return ( IsOnFire() && !HL2GameRules()->IsAlyxInDarknessMode() );
#else
	return IsOnFire();
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: Handle the special case of a zombie killed by a physics chopper.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::DieChopped( const CTakeDamageInfo &info )
{
	// S:O - This should never really happen
	/*Vector forceVector( vec3_origin );

	forceVector += CalcDamageForceVector( info );

	float flFadeTime = 0.0;

	if( HasSpawnFlags( SF_NPC_FADE_CORPSE ) )
	{
		flFadeTime = 5.0;
	}

	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	if ( UTIL_ShouldShowBlood( BLOOD_COLOR_RED ) )
	{
		int i;
		Vector vecSpot;
		Vector vecDir;

		for ( i = 0 ; i < 4; i++ )
		{
			vecSpot = WorldSpaceCenter();

			vecSpot.x += random->RandomFloat( -12, 12 ); 
			vecSpot.y += random->RandomFloat( -12, 12 ); 
			vecSpot.z += random->RandomFloat( -4, 16 ); 

			UTIL_BloodDrips( vecSpot, vec3_origin, BLOOD_COLOR_RED, 50 );
		}

		for ( int i = 0 ; i < 4 ; i++ )
		{
			Vector vecSpot = WorldSpaceCenter();

			vecSpot.x += random->RandomFloat( -12, 12 ); 
			vecSpot.y += random->RandomFloat( -12, 12 ); 
			vecSpot.z += random->RandomFloat( -4, 16 );

			vecDir.x = random->RandomFloat(-1, 1);
			vecDir.y = random->RandomFloat(-1, 1);
			vecDir.z = 0;
			VectorNormalize( vecDir );

			UTIL_BloodImpact( vecSpot, vecDir, BloodColor(), 1 );
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: damage has been done. Should the zombie ignite?
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgnite( const CTakeDamageInfo &info )
{
 	if ( IsOnFire() )
	{
		// Already burning!
		return false;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		//
		// If we take more than ten percent of our health in burn damage within a five
		// second interval, we should catch on fire.
		//
		m_flBurnDamage += info.GetDamage();
		m_flBurnDamageResetTime = gpGlobals->curtime + 5;

		if ( m_flBurnDamage >= m_iMaxHealth * 0.1 )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sufficient fire damage has been done. Zombie ignites!
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

#ifdef HL2_EPISODIC
	if ( HL2GameRules()->IsAlyxInDarknessMode() == true && GetEffectEntity() != NULL )
	{
		GetEffectEntity()->AddEffects( EF_DIMLIGHT );
	}
#endif // HL2_EPISODIC

	// Set the zombie up to burn to death in about ten seconds.
	SetHealth( min( m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC * (ZOMBIE_BURN_TIME + random->RandomFloat( -ZOMBIE_BURN_TIME_NOISE, ZOMBIE_BURN_TIME_NOISE)) ) );

	// FIXME: use overlays when they come online
	//AddOverlay( ACT_ZOM_WALK_ON_FIRE, false );
	if( !m_ActBusyBehavior.IsActive() )
	{
		Activity activity = GetActivity();
		Activity burningActivity = activity;

		if ( activity == ACT_WALK )
		{
			burningActivity = ACT_WALK_ON_FIRE;
		}
		else if ( activity == ACT_RUN )
		{
			burningActivity = ACT_RUN_ON_FIRE;
		}
		else if ( activity == ACT_IDLE )
		{
			burningActivity = ACT_IDLE_ON_FIRE;
		}

		if( HaveSequenceForActivity(burningActivity) )
		{
			// Make sure we have a sequence for this activity (torsos don't have any, for instance) 
			// to prevent the baseNPC & baseAnimating code from throwing red level errors.
			SetActivity( burningActivity );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::CopyRenderColorTo( CBaseEntity *pOther )
{
	color32 color = GetRenderColor();
	pOther->SetRenderColor( color.r, color.g, color.b, color.a );
}

//-----------------------------------------------------------------------------
// Purpose: Look in front and see if the claw hit anything.
//
// Input  :	flDist				distance to trace		
//			iDamage				damage to do if attack hits
//			vecViewPunch		camera punch (if attack hits player)
//			vecVelocityPunch	velocity punch (if attack hits player)
//
// Output : The entity hit by claws. NULL if nothing.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_BaseZombie::ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin  )
{
	// Added test because claw attack anim sometimes used when for cases other than melee
	int iDriverInitialHealth = -1;
	CBaseEntity *pDriver = NULL;
	if ( GetEnemy() )
	{
		trace_t	tr;
		AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0f )
			return NULL;

		// CheckTraceHullAttack() can damage player in vehicle as side effect of melee attack damaging physics objects, which the car forwards to the player
		// need to detect this to get correct damage effects
		CBaseCombatCharacter *pCCEnemy = ( GetEnemy() != NULL ) ? GetEnemy()->MyCombatCharacterPointer() : NULL;
		CBaseEntity *pVehicleEntity;
		if ( pCCEnemy != NULL && ( pVehicleEntity = pCCEnemy->GetVehicleEntity() ) != NULL )
		{
			if ( pVehicleEntity->GetServerVehicle() && dynamic_cast<CPropVehicleDriveable *>(pVehicleEntity) )
			{
				pDriver = static_cast<CPropVehicleDriveable *>(pVehicleEntity)->GetDriver();
				if ( pDriver && pDriver->IsPlayer() )
				{
					iDriverInitialHealth = pDriver->GetHealth();
				}
				else
				{
					pDriver = NULL;
				}
			}
		}
	}

	//
	// Trace out a cubic section of our hull and see what we hit.
	//
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	CBaseEntity *pHurt = NULL;
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{ 
		// We always hit bullseyes we're targeting
		pHurt = GetEnemy();
		CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), iDamage, DMG_SLASH );
		pHurt->TakeDamage( info );
	}
	else 
	{
		// Try to hit them with a trace
		pHurt = CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_SLASH );
	}

	if ( pDriver && iDriverInitialHealth != pDriver->GetHealth() )
	{
		pHurt = pDriver;
	}

	if ( !pHurt && m_hPhysicsEnt != NULL && IsCurSchedule(SCHED_ZOMBIE_ATTACKITEM) )
	{
		pHurt = m_hPhysicsEnt;

		Vector vForce = pHurt->WorldSpaceCenter() - WorldSpaceCenter(); 
		VectorNormalize( vForce );

		vForce *= 5 * 24;

		CTakeDamageInfo info( this, this, vForce, GetAbsOrigin(), iDamage, DMG_SLASH );
		pHurt->TakeDamage( info );

		pHurt = m_hPhysicsEnt;
	}

	if ( pHurt )
	{
		AttackHitSound();

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE ) )
		{
			pPlayer->ViewPunch( qaViewPunch );
			
			pPlayer->VelocityPunch( vecVelocityPunch );
		}
		else if( !pPlayer && UTIL_ShouldShowBlood(pHurt->BloodColor()) )
		{
			// Hit an NPC. Bleed them!
			Vector vecBloodPos;

			switch( BloodOrigin )
			{
			case ZOMBIE_BLOOD_LEFT_HAND:
				if( GetAttachment( "blood_left", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), min( iDamage, 30 ) );
				break;

			case ZOMBIE_BLOOD_RIGHT_HAND:
				if( GetAttachment( "blood_right", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), min( iDamage, 30 ) );
				break;

			case ZOMBIE_BLOOD_BOTH_HANDS:
				if( GetAttachment( "blood_left", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), min( iDamage, 30 ) );

				if( GetAttachment( "blood_right", vecBloodPos ) )
					SpawnBlood( vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), min( iDamage, 30 ) );
				break;

			case ZOMBIE_BLOOD_BITE:
				// No blood for these.
				break;
			}
		}
	}
	else 
	{
		AttackMissSound();
	}

	if ( pHurt == m_hPhysicsEnt && IsCurSchedule(SCHED_ZOMBIE_ATTACKITEM) )
	{
		m_hPhysicsEnt = NULL;
		m_flNextSwat = gpGlobals->curtime + random->RandomFloat( 2, 4 );
	}

	return pHurt;
}

//-----------------------------------------------------------------------------
// Purpose: The zombie is frustrated and pounding walls/doors. Make an appropriate noise
// Input  : 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::PoundSound()
{
	trace_t		tr;
	Vector		forward;

	GetVectors( &forward, NULL, NULL );

	AI_TraceLine( EyePosition(), EyePosition() + forward * 128, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction == 1.0 )
	{
		// Didn't hit anything!
		return;
	}

	if( tr.fraction < 1.0 && tr.m_pEnt )
	{
		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if( psurf )
		{
			EmitSound( physprops->GetString(psurf->sounds.impactHard) );
			return;
		}
	}

	// Otherwise fall through to the default sound.
	CPASAttenuationFilter filter( this,"NPC_BaseZombie.PoundDoor" );
	EmitSound( filter, entindex(),"NPC_BaseZombie.PoundDoor" );
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_NPC_ATTACK_BROADCAST )
	{
		if( GetEnemy() && GetEnemy()->IsNPC() )
		{
			if( HasCondition(COND_CAN_MELEE_ATTACK1) )
			{
				// This animation is sometimes played by code that doesn't intend to attack the enemy
				// (For instance, code that makes a zombie take a frustrated swipe at an obstacle). 
				// Try not to trigger a reaction from our enemy unless we're really attacking. 
				GetEnemy()->MyNPCPointer()->DispatchInteraction( g_interactionZombieMeleeWarning, NULL, this );
			}
		}
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_POUND )
	{
		PoundSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ALERTSOUND )
	{
		AlertSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( false );
		return;
	}
	
	if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( true );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_GET_UP )
	{
		MakeAIFootstepSound( 180.0f, 3.0f );
		if( !IsOnFire() )
		{
			// If you let this code run while a zombie is burning, it will stop wailing. 
			//m_flNextMoanSound = gpGlobals->curtime;
			//MoanSound( envDefaultZombieMoanVolumeFast, ARRAYSIZE( envDefaultZombieMoanVolumeFast ) );
		}
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( false );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( true );
		return;
	}

	// all swat animations are handled as a single case.
	if ( pEvent->event == AE_ZOMBIE_STARTSWAT )
	{
		MakeAIFootstepSound( 180.0f );
		AttackSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
	{
		AttackSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SWATITEM )
	{
		CBaseEntity *pEnemy = GetEnemy();
		if ( pEnemy )
		{
			Vector v;
			CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;
			if( !pPhysicsEntity )
			{
				return;
			}
			
			IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

			if( !pPhysObj )
			{
				return;
			}

			EmitSound( "NPC_BaseZombie.Swat" );
			PhysicsImpactSound( pEnemy, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 800 );

			Vector physicsCenter = pPhysicsEntity->WorldSpaceCenter();
			v = pEnemy->WorldSpaceCenter() - physicsCenter;
			VectorNormalize(v);

			SwatObject(pPhysObj, v);

			m_hPhysicsEnt = NULL;

			m_flNextSwatScan = gpGlobals->curtime + ZOMBIE_SWAT_DELAY;

			return;
		}
		else
		{		
			CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;
			if( !pPhysicsEntity )
			{
				DevMsg( "**Zombie: Missing my physics ent!!" );
				return;
			}
			
			IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

			if( !pPhysObj )
			{
				return;
			}

			EmitSound( "NPC_BaseZombie.Swat" );

			Vector forward;
			AngleVectors( EyeAngles(), &forward, NULL, NULL );

			SwatObject(pPhysObj, forward);

			m_hPhysicsEnt = NULL;
			m_flNextSwatScan = gpGlobals->curtime + ZOMBIE_SWAT_DELAY;

			return;
		}
	}
	
	if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );
		
		right = right * 100;
		forward = forward * 200;

		ClawAttack( GetClawAttackRange(), random->RandomFloat( 10, 20 ), QAngle( -15, -20, -10 ), right + forward, ZOMBIE_BLOOD_RIGHT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );

		right = right * -100;
		forward = forward * 200;

		ClawAttack( GetClawAttackRange(), random->RandomFloat( 10, 20 ), QAngle( -15, 20, -10 ), right + forward, ZOMBIE_BLOOD_LEFT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_BOTH )
	{
		Vector forward;
		QAngle qaPunch( 45, random->RandomInt(-5,5), random->RandomInt(-5,5) );
		AngleVectors( GetLocalAngles(), &forward );
		forward = forward * 200;
		ClawAttack( GetClawAttackRange(), random->RandomFloat( 15, 25 ), qaPunch, forward, ZOMBIE_BLOOD_BOTH_HANDS );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the base zombie.
//
// !!!IMPORTANT!!! YOUR DERIVED CLASS'S SPAWN() RESPONSIBILITIES:
//
//		Call Precache();
//		Set status for m_fIsTorso & m_fIsHeadless
//		Set blood color
//		Set health
//		Set field of view
//		Call CapabilitiesClear() & then set relevant capabilities
//		THEN Call BaseClass::Spawn()
//-----------------------------------------------------------------------------
#define HOLDOUT_TRIGGER "trigger_holdout"

void CNPC_BaseZombie::Spawn( void )
{
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	AddSpawnFlags( SF_NPC_FADE_CORPSE );

	m_NPCState = NPC_STATE_IDLE;

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );
	CapabilitiesAdd( bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB );

	m_flNextSwat = gpGlobals->curtime;
	m_flNextSwatScan = gpGlobals->curtime;
	HoldoutTriggerAmbushDelay = gpGlobals->curtime + 15.0f;
	m_pMoanSound = NULL;

	m_flNextMoanSound = gpGlobals->curtime + 9999;

	SetZombieModel();

	NPCInit();

	m_bIsSlumped = false;

	GetEnemies()->SetFreeKnowledgeDuration( 1000.0 );

	m_ActBusyBehavior.SetUseRenderBounds( true );

	// S:O - Fade in when we spawn
	if ( (V_strcmp( GetClassname(), "npc_cloud" ) != 0) && (V_strcmp( GetClassname(), "npc_ghost" ) != 0) )
	{
		SetRenderColorA(0);
		m_nRenderFX = kRenderFxSolidFast;
		m_fFadeFinish = gpGlobals->curtime + 3.0f;
	}

	// S:O - Remove shadows in an attempt to improve client FPS
	SetEffects( EF_NOSHADOW );

	// S:O - GetTeamNumber() support for NPCs
	ChangeTeam( 2 );
}


//-----------------------------------------------------------------------------
// Purpose: Pecaches all resources this NPC needs.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Precache( void )
{
	PrecacheScriptSound( "E3_Phystown.Slicer" );
	PrecacheScriptSound( "NPC_BaseZombie.PoundDoor" );
	PrecacheScriptSound( "NPC_BaseZombie.Swat" );

	PrecacheParticleSystem( "blood_impact_zombie_01" );

	// S:O - Experimental simple gib system
	/*PrecacheModel( "models/gibs/hgibs_rib.mdl" );
	PrecacheModel( "models/gibs/hgibs_scapula.mdl" );
	PrecacheModel( "models/gibs/hgibs_spine.mdl" );*/

	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if( IsSlumped() && hl2_episodic.GetBool() )
	{
		if( FClassnameIs( pOther, "prop_physics" ) )
		{
			// Get up!
			m_ActBusyBehavior.StopBusying();
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::CreateBehaviors()
{
	AddBehavior( &m_ActBusyBehavior );

	return BaseClass::CreateBehaviors();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		if ( HasCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION ) && !HasCondition(COND_TASK_FAILED) && IsCurSchedule( SCHED_ZOMBIE_CHASE_ENEMY, false ) )
		{
			return SCHED_COMBAT_PATROL;
		}
		return SCHED_ZOMBIE_CHASE_ENEMY;
		break;

	case SCHED_ZOMBIE_SWATITEM:
		// If the object is far away, move and swat it. If it's close, just swat it.
		if( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
		{
			return SCHED_ZOMBIE_MOVE_SWATITEM;
		}
		else
		{
			return SCHED_ZOMBIE_SWATITEM;
		}
		break;

	case SCHED_STANDOFF:
		return SCHED_ZOMBIE_WANDER_STANDOFF;

	case SCHED_MELEE_ATTACK1:
		return SCHED_ZOMBIE_MELEE_ATTACK1;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::BuildScheduleTestBits( void )
{
	// Ignore damage if we were recently damaged or we're attacking.
	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
#ifndef HL2_EPISODIC
	else if ( m_flNextFlinch >= gpGlobals->curtime )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
#endif // !HL2_EPISODIC

	if ( GetActivity() == ACT_IDLE )
	{
		SetCustomInterruptCondition( COND_HEAR_DANGER );
		SetCustomInterruptCondition( COND_HEAR_THUMPER );
		SetCustomInterruptCondition( COND_HEAR_BULLET_IMPACT );
		SetCustomInterruptCondition( COND_HEAR_PHYSICS_DANGER );
		SetCustomInterruptCondition( COND_HEAR_MOVE_AWAY );
		SetCustomInterruptCondition( COND_HEAR_SPOOKY );
		SetCustomInterruptCondition( COND_HEAR_PLAYER );
		SetCustomInterruptCondition( COND_HEAR_COMBAT );
		SetCustomInterruptCondition( COND_HEAR_WORLD );
	}

	// Everything should be interrupted if we get killed.

	BaseClass::BuildScheduleTestBits();

	if( IsCurSchedule(SCHED_ZOMBIE_AMBUSH_MODE) )
	{
		SetCustomInterruptCondition( COND_RECEIVED_ORDERS );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when we change schedules.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnScheduleChange( void )
{
	//
	// If we took damage and changed schedules, ignore further damage for a few seconds.
	//
	if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))
	{
		m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;
	} 

	BaseClass::OnScheduleChange();
}


//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_BaseZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_ZOMBIE_WANDER_MEDIUM )
	{
		return SCHED_ZOMBIE_WANDER_FAIL;
	}

	// If we can swat physics objects, see if we can swat our obstructor
	if ( CanSwatPhysicsObjects() )
	{
		if ( IsPathTaskFailure( taskFailCode ) && 
			 m_hObstructor != NULL && m_hObstructor->VPhysicsGetObject() && 
			 m_hObstructor->VPhysicsGetObject()->GetMass() < 100 )
		{
			m_hPhysicsEnt = m_hObstructor;
			m_hObstructor = NULL;
			return SCHED_ZOMBIE_ATTACKITEM;
		}
	}

	m_hObstructor = NULL;

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::SelectSchedule ( void )
{	
	// S:O - Holdout Trigger Functionality
	CBaseEntity *pTargetHoldoutTrigger = gEntList.FindEntityByClassname( NULL, HOLDOUT_TRIGGER );

	// Only default to this behavior if we've been hanging around for a while.
	// We don't want a flood of zombies, we want them coming and going as they please.
	// This makes any maps/gamemodes with NPCs much more interesting and fair for players.
	if ( pTargetHoldoutTrigger && (gpGlobals->curtime >= HoldoutTriggerAmbushDelay) && !HasCondition( COND_CAN_MELEE_ATTACK1 ) && !HasCondition( COND_CAN_MELEE_ATTACK2 ) && !HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_CAN_RANGE_ATTACK2 ) && !HasCondition( COND_ENEMY_DEAD ) && !HasCondition( COND_ZOMBIE_CAN_SWAT_ATTACK ) && !HasCondition( COND_NEW_ENEMY ) && !GetEnemy() )
	{
		// We've found a valid holdout trigger. As long as we don't have anything important to do, move to it!
		// Search again in another fifteen seconds or so.
		HoldoutTriggerAmbushDelay = gpGlobals->curtime + 15.0f;
		return SCHED_ZOMBIE_MOVE_HOLDOUT_TRIGGER;
	}

	if ( BehaviorSelectSchedule() )
	{
		return BaseClass::SelectSchedule();
	}

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
		{
			float flDist;

			flDist = ( GetLocalOrigin() - GetEnemy()->GetLocalOrigin() ).Length();

			// If this is a new enemy that's far away, ambush!!
			if (flDist >= zombie_ambushdist.GetFloat() && MustCloseToAttack() )
			{
				return SCHED_ZOMBIE_MOVE_TO_AMBUSH;
			
			}
		}

		if ( HasCondition( COND_LOST_ENEMY ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			return SCHED_ZOMBIE_WANDER_MEDIUM;
			//return SCHED_COMBAT_PATROL;
		}

		if( HasCondition( COND_ZOMBIE_CAN_SWAT_ATTACK ) )
		{
			return SCHED_ZOMBIE_SWATITEM;
		}
		break;

	case NPC_STATE_ALERT:
		if ( HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_DEAD ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );

#ifdef DEBUG_ZOMBIES
			DevMsg("Wandering\n");
#endif

			// Just lost track of our enemy. 
			// Wander around a bit so we don't look like a dingus.
			return SCHED_ZOMBIE_WANDER_MEDIUM;
			//return SCHED_COMBAT_PATROL;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::IsSlumped( void )
{
	if( hl2_episodic.GetBool() )
	{
		if( m_ActBusyBehavior.IsInsideActBusy() && !m_ActBusyBehavior.IsStopBusying() )
		{
			return true;
		}
	}
	else
	{
		int sequence = GetSequence();
		if ( sequence != -1 )
		{
			return ( strncmp( GetSequenceName( sequence ), "slump", 5 ) == 0 );
		}
	}

	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::IsGettingUp( void )
{
	if( m_ActBusyBehavior.IsActive() && m_ActBusyBehavior.IsStopBusying() )
	{
		return true;
	}
	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::GetSwatActivity( void )
{
	// Hafta figure out whether to swat with left or right arm.
	// Also hafta figure out whether to swat high or low. (later)
	float		flDot;
	Vector		vecRight, vecDirToObj;

	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );
	
	vecDirToObj = m_hPhysicsEnt->GetLocalOrigin() - GetLocalOrigin();
	VectorNormalize(vecDirToObj);

	// compare in 2D.
	vecRight.z = 0.0;
	vecDirToObj.z = 0.0;

	flDot = DotProduct( vecRight, vecDirToObj );

	Vector vecMyCenter;
	Vector vecObjCenter;

	vecMyCenter = WorldSpaceCenter();
	vecObjCenter = m_hPhysicsEnt->WorldSpaceCenter();
	float flZDiff;

	flZDiff = vecMyCenter.z - vecObjCenter.z;

	if( flDot >= 0 )
	{
		// Right
		if( flZDiff < 0 )
		{
			return ACT_ZOM_SWATRIGHTMID;
		}

		return ACT_ZOM_SWATRIGHTLOW;
	}
	else
	{
		// Left
		if( flZDiff < 0 )
		{
			return ACT_ZOM_SWATLEFTMID;
		}

		return ACT_ZOM_SWATLEFTLOW;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::GatherConditions( void )
{
	// S:O - Fade in when we spawn (HACKHACK)
	if (m_nRenderFX == kRenderFxSolidFast && m_fFadeFinish < gpGlobals->curtime)
		m_nRenderFX = kRenderFxNone;

	ClearCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION );

	BaseClass::GatherConditions();

	if( m_NPCState == NPC_STATE_COMBAT )
	{
		// This check for !m_pPhysicsEnt prevents a crashing bug, but also
		// eliminates the zombie picking a better physics object if one happens to fall
		// between him and the object he's heading for already. 
		if( gpGlobals->curtime >= m_flNextSwatScan && (m_hPhysicsEnt == NULL) )
		{
			FindNearestPhysicsObject( ZOMBIE_MAX_PHYSOBJ_MASS );
			m_flNextSwatScan = gpGlobals->curtime + 2.0;
		}
	}

	if( (m_hPhysicsEnt != NULL) && gpGlobals->curtime >= m_flNextSwat && HasCondition( COND_SEE_ENEMY ) )
	{
		SetCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
	}
	else
	{
		ClearCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
#if 0
	DevMsg(" ** %d Angry Zombies **\n", s_iAngryZombies );
#endif

#if 0
	if( m_NPCState == NPC_STATE_COMBAT )
	{
		// Zombies should make idle sounds in combat
		if( random->RandomInt( 0, 30 ) == 0 )
		{
			IdleSound();
		}
	}	
#endif 

	//
	// Cool off if we aren't burned for five seconds or so. 
	//
	if ( ( m_flBurnDamageResetTime ) && ( gpGlobals->curtime >= m_flBurnDamageResetTime ) )
	{
		m_flBurnDamage = 0;
	}

// This is being a bitch, so let's remove it for now.
// As things currently stand, this will open doors we don't want to open by themselves, such as those tied to triggers, etc.
// Eventually we'll expand this and get every aspect of it to work. In the meantime, keep this commented out.
/*#ifndef CLIENT_DLL
	// S:O - If we're near a door, open it!
	if ( gpGlobals->curtime >= m_flDoorCheckTime )
	{
		//CBaseEntity *pZombie = dynamic_cast<CBaseEntity *>( this );

		CBaseEntity *pDoor1 = gEntList.FindEntityByClassname( NULL, "prop_door_rotating" );
		CBaseEntity *pDoor2 = gEntList.FindEntityByClassname( NULL, "func_door_rotating" );
		CBaseEntity *pDoor3 = gEntList.FindEntityByClassname( NULL, "func_door" );

		CBaseEntity *list[1024];
		int count = UTIL_EntitiesInSphere( list, 1024, GetAbsOrigin() + Vector(0, 10, 0), 400, MASK_ALL );

		for ( int i = 0; i < count; i++ )
		{
			if( pDoor1 )
			{
				// Not supported yet.
			}
			if( pDoor2 )
			{
				CBaseDoor *pDoorTarget2 = dynamic_cast<CBaseDoor *>( pDoor2 );
				if ( pDoorTarget2->m_toggle_state != TS_AT_TOP && pDoorTarget2->m_toggle_state != TS_GOING_UP && !pDoorTarget2->m_bLocked )
					pDoorTarget2->DoorGoUp();
			}
			else if( pDoor3 )
			{
				CBaseDoor *pDoorTarget3 = dynamic_cast<CBaseDoor *>( pDoor3 );
				if ( pDoorTarget3->m_toggle_state != TS_AT_TOP && pDoorTarget3->m_toggle_state != TS_GOING_UP && !pDoorTarget3->m_bLocked )
					pDoorTarget3->DoorGoUp();
			}
		}

		m_flBurnDamageResetTime = gpGlobals->curtime + 5;
	}
#endif*/
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_DIE:
		// Go to ragdoll
		KillMe();
		TaskComplete();
		break;

	case TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ:
		{
			Vector vecGoalPos;
			Vector vecDir;

			vecDir = GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();
			VectorNormalize(vecDir);
			vecDir.z = 0;

			AI_NavGoal_t goal( m_hPhysicsEnt->WorldSpaceCenter() );
			goal.pTarget = m_hPhysicsEnt;
			GetNavigator()->SetGoal( goal );

			TaskComplete();
		}
		break;

	case TASK_ZOMBIE_GET_PATH_TO_HOLDOUT_TRIGGER:
		{
			CBaseEntity *pTargetAI = gEntList.FindEntityByClassname( NULL, HOLDOUT_TRIGGER );
	
			// S:O - Double-check to make sure we have a valid holdout trigger...
			if ( pTargetAI )
			{
				Vector vecGoalPos;
				Vector vecDir;

				vecDir = GetLocalOrigin() - pTargetAI->GetLocalOrigin();
				VectorNormalize(vecDir);
				vecDir.z = 0;

				AI_NavGoal_t goal( pTargetAI->WorldSpaceCenter() );
				goal.pTarget = pTargetAI;
				GetNavigator()->SetGoal( goal );
			}
			else
			{
				// Our double-check caught something. Get the hell out of here!
				return;
			}
		}
		break;

	case TASK_ZOMBIE_SWAT_ITEM:
		{
			if( m_hPhysicsEnt == NULL )
			{
				// Physics Object is gone! Probably was an explosive 
				// or something else broke it.
				TaskFail("Physics ent NULL");
			}
			else if ( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
			{
				// Physics ent is no longer in range! Probably another zombie swatted it or it moved
				// for some other reason.
				TaskFail( "Physics swat item has moved" );
			}
			else
			{
				SetIdealActivity( (Activity)GetSwatActivity() );
			}
			break;
		}
		break;

	case TASK_ZOMBIE_DELAY_SWAT:
		m_flNextSwat = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		break;

	case TASK_ZOMBIE_WAIT_POST_MELEE:
		{
#ifndef HL2_EPISODIC
			TaskComplete();
			return;
#endif

			// Don't wait when attacking the player
			if ( GetEnemy() && GetEnemy()->IsPlayer() )
			{
				
				TaskComplete();
				return;
			}

			// Wait a single think
			SetWait( 0.1 );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_SWAT_ITEM:
		if( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_ZOMBIE_WAIT_POST_MELEE:
		{
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::Event_Killed( const CTakeDamageInfo &info )
{
	numzombies = numzombies - 1;

	if ( info.GetDamageType() & DMG_VEHICLE )
	{
		Vector vecDamageDir = info.GetDamageForce();
		VectorNormalize( vecDamageDir );
	}

	SetRenderColorA( 255 );
	m_nRenderFX = kRenderFxFadeFast;
	m_fFadeFinish = gpGlobals->curtime + 4.0f;

	/*// S:O - Slow Motion Effect
	CHL2MPRules *pRules = HL2MPRules();

	if ( pRules )
		pRules->ActivateSlowMotion();*/

	// S:O - Reward a player or NPC for killing this NPC appropriately
	CBaseEntity *pEnt = info.GetAttacker();
	if ( pEnt && pEnt->IsPlayer() )
	{
		CHL2MP_Player *pAttackerPlayer = dynamic_cast<CHL2MP_Player*>( pEnt );

		if ( pAttackerPlayer )
		{
			GetGlobalTeam( pAttackerPlayer->GetTeamNumber() )->AddScore( 1 );

			// S:O - There's a chance the player who killed us could become a zombie if they were too close to us when we died...
			// Unless we're dealing with a cloud or ghost NPC, of course...
			if ( (V_strcmp( GetClassname(), "npc_cloud" ) != 0) && (V_strcmp( GetClassname(), "npc_ghost" ) != 0) )
			{
				if( (so_infection_system.GetInt() != 0) && (so_infection_chance.GetInt() != 0) && !FStrEq(so_gamemode.GetString(), "Overlord") && (pAttackerPlayer->GetTeamNumber() == 3 || pAttackerPlayer->GetTeamNumber() == 4) && !pAttackerPlayer->IsInfected() )
				{
					CBaseEntity *pEntList[100];
					int numEnts = UTIL_EntitiesInSphere( pEntList, 100, GetAbsOrigin(), 75.0f, 0 );
					for ( int i = 0; i < numEnts; i++ )
					{
						if ( pEntList[i] && (pEntList[i] == pAttackerPlayer) )
						{
							int InfectionRand = (int)(random->RandomFloat( 1, so_infection_chance.GetInt() ));
							if ( InfectionRand >= (so_infection_chance.GetInt() - 1) )
							{
								CHL2MPRules *pRules = HL2MPRules();
								if ( !pRules )
									continue;

								pRules->ZombifyPlayer( pAttackerPlayer );
							}
						}
					}
				}
			}
		}
	}
	else if ( pEnt && pEnt->IsNPC() )
	{
		CNPC_CombineS *pSoliderNPC = dynamic_cast<CNPC_CombineS*>(pEnt);

		if ( pSoliderNPC )
			GetGlobalTeam( 4 )->AddScore( 1 );
		else
			GetGlobalTeam( 3 )->AddScore( 1 );
	}

	// S:O - Experimental simple gib system
	/*CGib::SpawnSpecificGibs( this, 3, 50, 1, "models/gibs/hgibs_rib.mdl", 5 );
	CGib::SpawnSpecificGibs( this, 2, 50, 1, "models/gibs/hgibs_scapula.mdl", 5 );
	CGib::SpawnSpecificGibs( this, 1, 50, 1, "models/gibs/hgibs_spine.mdl", 5 );
	UTIL_BloodDrips( GetAbsOrigin(), vec3_origin, BLOOD_COLOR_RED, 500 );*/

   	BaseClass::Event_Killed( info );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	bool bKilledByVehicle = ( ( info.GetDamageType() & DMG_VEHICLE ) != 0 );
	if( (!IsChopped(info) && !IsSquashed(info)) || bKilledByVehicle )
	{
		return BaseClass::BecomeRagdoll( info, forceVector );
	}

	if( !(GetFlags()&FL_TRANSRAGDOLL) )
	{
		RemoveDeferred();
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StopLoopingSounds()
{
	//ENVELOPE_CONTROLLER.SoundDestroy( m_pMoanSound );
	//m_pMoanSound = NULL;

	BaseClass::StopLoopingSounds();
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RemoveHead( void )
{
	m_fIsHeadless = true;
	SetZombieModel();
}


bool CNPC_BaseZombie::ShouldPlayFootstepMoan( void )
{
	if( random->RandomInt( 1, zombie_stepfreq.GetInt() * s_iAngryZombies ) == 1 )
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------
// Provides a standard way for the zombie to get the 
// distance to a physics ent. Since the code to find physics 
// objects uses a fast dis approx, we have to use that here
// as well.
//---------------------------------------------------------
float CNPC_BaseZombie::DistToPhysicsEnt( void )
{
	//return ( GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin() ).Length();
	if ( m_hPhysicsEnt != NULL )
		return UTIL_DistApprox2D( GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter() );
	return ZOMBIE_PHYSOBJ_SWATDIST + 1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	switch( NewState )
	{
	case NPC_STATE_COMBAT:
		{
			RemoveSpawnFlags( SF_NPC_GAG );
			s_iAngryZombies++;
		}
		break;

	default:
		if( OldState == NPC_STATE_COMBAT )
		{
			// Only decrement if coming OUT of combat state.
			s_iAngryZombies--;
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Refines a base activity into something more specific to our internal state.
//-----------------------------------------------------------------------------
Activity CNPC_BaseZombie::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_WALK && IsCurSchedule( SCHED_COMBAT_PATROL, false) )
		baseAct = ACT_RUN;

	if ( IsOnFire() )
	{
		switch ( baseAct )
		{
			case ACT_RUN_ON_FIRE:
			{
				return ( Activity )ACT_WALK_ON_FIRE;
			}

			case ACT_WALK:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_WALK_ON_FIRE;
			}

			case ACT_IDLE:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_IDLE_ON_FIRE;
			}
		}
	}

	if ( baseAct == ACT_JUMP || baseAct == ACT_HOP || baseAct == ACT_LEAP )
	{
		return ( Activity )ACT_WALK_ON_FIRE;
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_BaseZombie::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	
	if( IsCurSchedule(SCHED_BIG_FLINCH) || m_ActBusyBehavior.IsActive() )
	{
		// This zombie is assumed to be standing up. 
		// Return a position that's centered over the absorigin,
		// halfway between the origin and the head. 
		Vector vecTarget = GetAbsOrigin();
		Vector vecHead = HeadTarget( posSrc );
		vecTarget.z = ((vecTarget.z + vecHead.z) * 0.5f);
		return vecTarget;
	}

	return BaseClass::BodyTarget( posSrc, bNoisy );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_BaseZombie::HeadTarget( const Vector &posSrc )
{
	int iCrabAttachment = LookupAttachment( "headcrab" );
	Assert( iCrabAttachment > 0 );

	Vector vecPosition;

	GetAttachment( iCrabAttachment, vecPosition );

	return vecPosition;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::GetAutoAimRadius()
{
	return BaseClass::GetAutoAimRadius();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_ENTITY && gpGlobals->curtime >= m_flNextSwat )
	{
		m_hObstructor = pMoveGoal->directTrace.pObstruction;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//			&chasePosition - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	// If our enemy is in a vehicle, we need them to tell us where to navigate to them
	if ( pEnemy == NULL )
		return;

	CBaseCombatCharacter *pBCC = pEnemy->MyCombatCharacterPointer();
	if ( pBCC && pBCC->IsInAVehicle() )
	{
		Vector vecForward, vecRight;
		pBCC->GetVectors( &vecForward, &vecRight, NULL );

		chasePosition = pBCC->WorldSpaceCenter() + ( vecForward * 24.0f ) + ( vecRight * 48.0f );
		return;
	}

	BaseClass::TranslateNavGoal( pEnemy, chasePosition );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( base_zombie, CNPC_BaseZombie )

	DECLARE_TASK( TASK_ZOMBIE_DELAY_SWAT )
	DECLARE_TASK( TASK_ZOMBIE_SWAT_ITEM )
	DECLARE_TASK( TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ )
	DECLARE_TASK( TASK_ZOMBIE_GET_PATH_TO_HOLDOUT_TRIGGER )
	DECLARE_TASK( TASK_ZOMBIE_DIE )
	DECLARE_TASK( TASK_ZOMBIE_WAIT_POST_MELEE )

	DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTMID )
	DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTMID )
	DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTLOW )
	DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTLOW )
	DECLARE_ACTIVITY( ACT_ZOM_FALL )

	DECLARE_CONDITION( COND_ZOMBIE_CAN_SWAT_ATTACK )
	DECLARE_CONDITION( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION )

	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_RIGHT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_LEFT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_BOTH )
	DECLARE_ANIMEVENT( AE_ZOMBIE_SWATITEM )
	DECLARE_ANIMEVENT( AE_ZOMBIE_STARTSWAT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_LEFT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_RIGHT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_LEFT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_RIGHT )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_SCREAM )
	DECLARE_ANIMEVENT( AE_ZOMBIE_GET_UP )
	DECLARE_ANIMEVENT( AE_ZOMBIE_POUND )
	DECLARE_ANIMEVENT( AE_ZOMBIE_ALERTSOUND )

	DECLARE_INTERACTION( g_interactionZombieMeleeWarning )

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_SWATITEM,

		"	Tasks"
		"		TASK_ZOMBIE_DELAY_SWAT			3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ZOMBIE_SWAT_ITEM			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_HOLDOUT_TRIGGER,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ZOMBIE_GET_PATH_TO_HOLDOUT_TRIGGER	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TASK_FAILED"
	)

	//=========================================================
	// SwatItem
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_SWATITEM,

		"	Tasks"
		"		TASK_ZOMBIE_DELAY_SWAT			3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ZOMBIE_SWAT_ITEM			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_ATTACKITEM,

		"	Tasks"
		"		TASK_FACE_ENEMY					0"
		"		TASK_MELEE_ATTACK1				0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// ChaseEnemy
	//=========================================================
#ifdef HL2_EPISODIC
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	24"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_ZOMBIE_CAN_SWAT_ATTACK"
		"		COND_HEAVY_DAMAGE"
	)
#else 
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	24"
		"		 TASK_GET_CHASE_PATH_TO_ENEMY	600"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
		"		 TASK_FACE_ENEMY				0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_ZOMBIE_CAN_SWAT_ATTACK"
	)
#endif // HL2_EPISODIC

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_TO_AMBUSH,

		"	Tasks"
		"		TASK_WAIT						1.0" // don't react as soon as you see the player.
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_TURN_LEFT					180"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WAIT_AMBUSH"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_NEW_ENEMY"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WAIT_AMBUSH,

		"	Tasks"
		"		TASK_WAIT_FACE_ENEMY	99999"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
	)

	//=========================================================
	// Wander around for a while so we don't look stupid. 
	// this is done if we ever lose track of our enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_MEDIUM,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384" // 4 feet to 32 feet
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM" // keep doing it
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_WORLD"
		"		COND_HEAR_PLAYER"
		"		COND_HEAR_THUMPER"
		"		COND_HEAR_BULLET_IMPACT"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_STANDOFF,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384" // 4 feet to 32 feet
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_WORLD"
		"		COND_HEAR_PLAYER"
		"		COND_HEAR_THUMPER"
		"		COND_HEAR_BULLET_IMPACT"
	)

	//=========================================================
	// If you fail to wander, wait just a bit and try again.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_WAIT				1"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_WORLD"
		"		COND_HEAR_PLAYER"
		"		COND_HEAR_THUMPER"
		"		COND_HEAR_BULLET_IMPACT"
	)

	//=========================================================
	// Like the base class, only don't stop in the middle of 
	// swinging if the enemy is killed, hides, or new enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_MELEE_ATTACK1		0"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_POST_MELEE_WAIT"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// Make the zombie wait a frame after a melee attack, to
	// allow itself & it's enemy to test for dynamic scripted sequences.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_POST_MELEE_WAIT,

		"	Tasks"
		"		TASK_ZOMBIE_WAIT_POST_MELEE		0"
	)

	// S:O - Use this schedule right before a zombie_regroup or zombie_goto command to force them to obey.
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_AMBUSH_MODE,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_PVS			0"
		"	"
		"	Interrupts"
		"		COND_RECEIVED_ORDERS" 
	)

AI_END_CUSTOM_NPC()
