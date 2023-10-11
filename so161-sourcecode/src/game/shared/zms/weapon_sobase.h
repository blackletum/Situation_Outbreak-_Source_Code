#ifndef SO_WEAPON_BASE_H
#define SO_WEAPON_BASE_H

#include "cbase.h"
#include "npcevent.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponSOBase C_WeaponSOBase
#endif

class CWeaponSOBase : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponSOBase, CBaseHL2MPCombatWeapon );

	CWeaponSOBase(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:

	virtual void		ItemPostFrame( void );

	virtual float		GetAccuracyModifier( void );
	virtual float		GetIronsightAccuracy( void ) { return 0.75f; } // quarter of the spread
	virtual void		SecondaryAttack();
	int					WeaponSoundRealtime( WeaponSound_t shoot_type );

	virtual bool		Deploy( );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void		Drop( const Vector &vecVelocity );
	virtual bool		Reload( void );
	virtual void		FinishReload( void );

	virtual void		FireBullets( const FireBulletsInfo_t &info );
	virtual void		PrimaryAttack( void );

	virtual void		WeaponIdle( void );

	virtual void		AlternateFire( void );

	// activities
	virtual	Activity	GetDrawActivity( void ) { return ACT_VM_DRAW; } // anim to play when we whip it out
	virtual Activity	GetReloadActivity( void ) { return ACT_VM_RELOAD; } // reload act (ie. empty reloads)
	virtual Activity	GetIdleActivity( void ) { return ACT_VM_IDLE; } // idle act

	virtual int			BulletsToShoot( void ) { return 1; }

#ifdef CLIENT_DLL
	virtual float		SwayScale( void );
	virtual float		BobScale( void );

	virtual	float		CalcViewmodelBob( void );

	virtual void		OverrideMouseInput( float *x, float *y );
#else	
	virtual int			CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual void		FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	virtual void		FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	virtual void		Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	virtual void		Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	virtual bool		ShouldForceNPCFire( void ) { return false; }
#endif

	// Ironsights
	CNetworkVar		( bool, m_bIronsighted );
	CNetworkVar		( float, m_flIronsightTime );

	// Ironsight funcs
	virtual bool		HasIronsights( void );
	float				GetIronsightFOV( void );
	float				GetIronsightTime( void );
	Vector				GetIronsightPosition( void ) const;
	QAngle				GetIronsightAngles( void ) const;
	float				GetWeaponPrice( void ) const;
	float				GetWeaponAmmoPrice( void ) const;
	bool				IsIronsighted( void ) { return m_bIronsighted; }
	void				SetIronsights( bool b );
	void				ToggleIronsights( void ) { SetIronsights( !m_bIronsighted ); }
	void				EnterIronsights( void );
	void				ExitIronsights( void );

	// utility function
	static void			DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

private:
	bool				m_bHasIronsightedThisClick;

protected:
	int					m_nShotsFired;	// Number of consecutive shots fired
	float				m_flNextSoundTime;	// real-time clock of when to make next sound

};

#endif