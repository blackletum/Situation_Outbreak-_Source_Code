//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPON_M4_H
#define	WEAPON_M4_H

#include "basegrenade_shared.h"
#include "weapon_sobase.h"

#ifdef CLIENT_DLL
#define CWeaponM4 C_WeaponM4
#endif

class CWeaponM4 : public CWeaponSOBase
{
public:
	DECLARE_CLASS( CWeaponM4, CWeaponSOBase );

	CWeaponM4();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	Precache( void );

	//void	SecondaryAttack( void );
	void	DelayedAttack( void );

	//const char *GetTracerType( void ) { return "AR2Tracer"; }

	void	AddViewKick( void );

	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	void	FireNPCSecondaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	void	Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int		GetMinBurst( void ) { return 2; }
	int		GetMaxBurst( void ) { return 5; }
	float	GetFireRate( void ) { return 0.1f; }

	bool	CanHolster( void );
	bool	Reload( void );

#ifndef CLIENT_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	Activity	GetPrimaryAttackActivity( void );
#endif

	void	DoImpactEffect( trace_t &tr, int nDamageType );

	virtual bool Deploy( void );

	// TODO: Model an M4 with an optional grenade launcher, perhaps?
	//virtual void AlternateFire( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

private:
	CWeaponM4( const CWeaponM4 & );

protected:

	float					m_flDelayedFire;
	bool					m_bShotDelayed;
	int						m_nVentPose;

	//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
	//#endif
};


#endif	//WeaponM4_H
