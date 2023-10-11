//========= Copyright © 2009 Agent Red Productions					 ============//
//
// Purpose: Template for scope based weapons
// Author: Stephen Swires
//
//=============================================================================//

#ifndef SO_SNIPER_BASE
#define SO_SNIPER_BASE

#include "cbase.h"
#include "weapon_sobase.h"

#ifdef CLIENT_DLL
#define CWeaponSOSniperBase C_WeaponSOSniperBase
#endif

class CWeaponSOSniperBase : public CWeaponSOBase
{
	DECLARE_CLASS( CWeaponSOSniperBase, CWeaponSOBase );
public:

	CWeaponSOSniperBase( void );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	PrimaryAttack( void );
	virtual void	ItemPostFrame( void );

	virtual float	GetAccuracyModifier( void );
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_2DEGREES;
		return cone;
	}


	virtual bool	UnscopeAfterShot( void ) { return false; } // unscope after shooting?
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	bool			IsScoped( void );
	virtual bool	ShouldDrawScope( void ); // not the same as IsScoped in that it takes into account the scoping time

	// bolt action snipers
	virtual bool	IsBoltAction( void ) { return false; }
	bool			IsCocking( void ) { return m_bIsCocking; }
	void			CockBolt( void );
	void			FinishCocking( void );

private:

	CNetworkVar( bool, m_bNeedsCocking );
	CNetworkVar( bool, m_bIsCocking );
	CNetworkVar( float, m_flEndCockTime );

	CWeaponSOSniperBase( const CWeaponSOSniperBase & );
};

#endif