//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_sobase.h"

#ifdef CLIENT_DLL
#define CWeaponDoubleBarrel C_WeaponDoubleBarrel
#endif

extern ConVar sk_auto_reload_time;
extern ConVar sk_plr_num_shotgun_pellets;

class CWeaponDoubleBarrel : public CWeaponSOBase
{
public:
	DECLARE_CLASS( CWeaponDoubleBarrel, CWeaponSOBase );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	virtual float			GetFireRate( void ); //AI Patch Addition

	//bool Reload( void );
	//void FillClip( void );
	virtual Activity GetIdleActivity( void );
	void ItemHolsterFrame( void );
	//void ItemPostFrame( void );
	//void PrimaryAttack( void );

	virtual void AddViewKick();
	virtual Activity GetPrimaryAttackActivity();

	virtual Activity GetReloadActivity( void );
	virtual Activity GetDrawActivity( void );

	virtual int			BulletsToShoot( void ) { return 8; }
	virtual bool		ShouldForceNPCFire() { return true; }
	
#ifndef CLIENT_DLL //AI Patch Addition
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; } //AI Patch Addition
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator ); //AI Patch Addition
 #endif //AI Patch Addition

	DECLARE_ACTTABLE();
	CWeaponDoubleBarrel(void);

private:
	float m_flNextReload;
	float m_flFiringTime;
	CWeaponDoubleBarrel( const CWeaponDoubleBarrel & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDoubleBarrel, DT_WeaponDoubleBarrel )

BEGIN_NETWORK_TABLE( CWeaponDoubleBarrel, DT_WeaponDoubleBarrel )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponDoubleBarrel )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_doublebarrel, CWeaponDoubleBarrel );
PRECACHE_WEAPON_REGISTER(weapon_doublebarrel);

//AI Patch Addition
acttable_t	CWeaponDoubleBarrel::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_SHOTGUN,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_SHOTGUN,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_SHOTGUN,					false },
	
//=================
//AI Patch Addition
//=================
	
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
//======= End Of AI Patch ========
};

IMPLEMENT_ACTTABLE(CWeaponDoubleBarrel);


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponDoubleBarrel::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	/*switch( pEvent->event )
	{
		case EVENT_WEAPON_SHOTGUN_FIRE:
		{
			FireNPCPrimaryAttack( pOperator, false );
		}
		break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}*/
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Time between successive shots in a burst. Also returned for EP2
//			with an eye to not messing up Alyx in EP1.
//-----------------------------------------------------------------------------
float CWeaponDoubleBarrel::GetFireRate()
{
	return 0.7;
}
//======= End Of AI Patch =======

//-----------------------------------------------------------------------------
// Purpose: Animation anim
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CWeaponDoubleBarrel::GetReloadActivity( void )
{
	if( m_iClip1 == 1 )
		return ACT_VM_HITCENTER;

	return ACT_VM_HITCENTER2;
}

void CWeaponDoubleBarrel::AddViewKick()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if( !pPlayer ) return;

	QAngle punch;
	punch.Init( SharedRandomFloat( "shotgunpax", -2, -1 ), SharedRandomFloat( "shotgunpay", -2, 2 ), 0 );
	pPlayer->ViewPunch( punch );
}

Activity CWeaponDoubleBarrel::GetPrimaryAttackActivity()
{
	if ( m_iClip1 == 1 )
		return ACT_VM_PRIMARYATTACK;

	return ACT_VM_SECONDARYATTACK;
}

Activity CWeaponDoubleBarrel::GetDrawActivity()
{
	if( m_iClip1 == 0 )
		return ACT_VM_MISSCENTER2;
	else if( m_iClip1 == 1 )
		return ACT_VM_MISSCENTER;

	return ACT_VM_DRAW;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDoubleBarrel::CWeaponDoubleBarrel( void )
{
	m_flNextReload = 0.0f;
	m_flFiringTime = 0.0f;

	m_fMinRange1 = 0.0;
	m_fMaxRange1 = 500;
	m_fMinRange2 = 0.0;
	m_fMaxRange2 = 200;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDoubleBarrel::ItemHolsterFrame( void )
{
	// Return this to fix a nasty reloading bug.
	return;
}

//==================================================
// Purpose: 
//==================================================
Activity CWeaponDoubleBarrel::GetIdleActivity( void )
{
	if( m_iClip1 == 0 )
		return ACT_VM_RECOIL2;
	else if( m_iClip1 == 1 )
		return ACT_VM_RECOIL1;

	return ACT_VM_IDLE;
}