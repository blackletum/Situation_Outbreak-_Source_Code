#ifndef WEAPONPUMPSHOTTY_H
#define WEAPONPUMPSHOTTY_H

#include "cbase.h"
#include "npcevent.h"
#include "weapon_sobase.h"

#ifdef CLIENT_DLL
#define CWeaponPumpShotty C_WeaponPumpShotty
#endif

class CWeaponPumpShotty : public CWeaponSOBase
{
public:
	DECLARE_CLASS( CWeaponPumpShotty, CWeaponSOBase );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	CNetworkVar( bool,	m_bNeedPump );		// When emptied completely
	CNetworkVar( bool,	m_bDelayedFire1 );	// Fire primary when finished reloading
	CNetworkVar( bool,	m_bDelayedFire2 );	// Fire secondary when finished reloading
	CNetworkVar( bool,	m_bDelayedReload );	// Reload when finished pump

public:
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_7DEGREES;
		return cone;
	}

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }
	
	#ifndef CLIENT_DLL //AI Patch Addition
	virtual float			GetMinRestTime(); //AI Patch Addition
	virtual float			GetMaxRestTime(); //AI Patch Addition
#endif //AI Patch Addition

	virtual float			GetFireRate( void ); //AI Patch Addition


	bool StartReload( void );
	bool Reload( void );
	void FillClip( void );
	void FinishReload( void );
	void CheckHolsterReload( void );
	void Pump( void );
//	void WeaponIdle( void );
	void ItemHolsterFrame( void );
	void ItemPostFrame( void );
	void PrimaryAttack( void );
	//void SecondaryAttack( void );
	void DryFire( void );
	
#ifndef CLIENT_DLL //AI Patch Addition
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; } //AI Patch Addition

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles ); //AI Patch Addition
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary ); //AI Patch Addition
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator ); //AI Patch Addition
 #endif //AI Patch Addition

	DECLARE_ACTTABLE();
	CWeaponPumpShotty(void);

private:
	CWeaponPumpShotty( const CWeaponPumpShotty & );
};

#endif //WEAPONPUMPSHOTTY_H