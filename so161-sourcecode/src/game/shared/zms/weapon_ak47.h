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

#ifndef	WEAPONAK47_H
#define	WEAPONAK47_H

#include "basegrenade_shared.h"
#include "weapon_sobase.h"

#ifdef CLIENT_DLL
#define CWeaponAK47 C_WeaponAK47
#endif

class CWeaponAK47 : public CWeaponSOBase
{
public:
	DECLARE_CLASS( CWeaponAK47, CWeaponSOBase );

	CWeaponAK47();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	Precache( void );
	
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


	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		
		cone = VECTOR_CONE_3DEGREES;

		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

private:
	CWeaponAK47( const CWeaponAK47 & );

protected:

	float					m_flDelayedFire;
	bool					m_bShotDelayed;
	int						m_nVentPose;
	
//#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
//#endif
};


#endif	//WEAPONAK47_H
