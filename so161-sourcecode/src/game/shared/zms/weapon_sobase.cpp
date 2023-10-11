#include "cbase.h"
#include "weapon_sobase.h"
#include "hl2mp_weapon_parse.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_pumpshotty.h"
#include "weapon_autoshotty.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

// ironsight tweaking
ConVar vm_ironsight_adjust( "vm_ironsight_adjust", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Edit ironsight offsets" );
ConVar vm_ironsight_x( "vm_ironsight_x", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight X pos" );
ConVar vm_ironsight_y( "vm_ironsight_y", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight Y pos" );
ConVar vm_ironsight_z( "vm_ironsight_z", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight Z pos" );
ConVar vm_ironsight_pitch( "vm_ironsight_pitch", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight pitch angle" );
ConVar vm_ironsight_yaw( "vm_ironsight_yaw", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight yaw angle" );
ConVar vm_ironsight_roll( "vm_ironsight_roll", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight roll angly" );
ConVar vm_ironsight_fov( "vm_ironsight_fov", "60", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight FOV" );
ConVar vm_ironsight_time( "vm_ironsight_time", "0.5", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Adjust ironsight time" );
ConVar vm_ironsight_drawcrosshair( "vm_ironsight_drawcrosshair", "0", FCVAR_REPLICATED | FCVAR_ARCHIVE, "Draw crosshair in ironsight mode" );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSOBase, DT_WeaponSOBase )

BEGIN_NETWORK_TABLE( CWeaponSOBase, DT_WeaponSOBase )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bIronsighted ) ),
	RecvPropFloat( RECVINFO( m_flIronsightTime ) ),
#else
	SendPropBool( SENDINFO( m_bIronsighted ) ),
	SendPropFloat( SENDINFO( m_flIronsightTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSOBase )
	DEFINE_PRED_FIELD( m_bIronsighted, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flIronsightTime, FIELD_TIME, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

CWeaponSOBase::CWeaponSOBase()
{
	m_flIronsightTime = 0.0f;
	m_bIronsighted = false;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOperator - 
//-----------------------------------------------------------------------------
void CWeaponSOBase::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

	pOperator->FireBullets( BulletsToShoot(), vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, GetHL2MPWpnData().m_iPlayerDamage );

	// NOTENOTE: This is overriden on the client-side
	// pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;
}
#endif

void CWeaponSOBase::SecondaryAttack()
{
	if( HasIronsights() /* && !m_bHasIronsightedThisClick*/ )
	{
		m_bHasIronsightedThisClick = true;
		ToggleIronsights();

		m_flNextSecondaryAttack = gpGlobals->curtime + 0.25f;
	}
}

float CWeaponSOBase::GetAccuracyModifier()
{
	float accuracy = 1.0f;
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetOwner() );

	if( pPlayer )
	{
		if( IsIronsighted() )
			accuracy *= GetIronsightAccuracy();

		if( pPlayer->IsWalking() )
			accuracy *= 0.75f;

		if( !!( pPlayer->GetFlags() & FL_DUCKING ) )
			accuracy *= 0.8f;
	}

	return accuracy;
}

bool CWeaponSOBase::Reload( void )
{
	if( m_bIronsighted )
		ExitIronsights();

	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), GetReloadActivity() );
	
	if ( fRet )
	{
		ToHL2MPPlayer(GetOwner())->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
	}
	
	return fRet;
}

bool CWeaponSOBase::Deploy()
{
	MDLCACHE_CRITICAL_SECTION();
	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), GetDrawActivity(), (char*)GetAnimPrefix() );
}

bool CWeaponSOBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	if( m_bIronsighted )
		ExitIronsights();

	return BaseClass::Holster( pSwitchingTo );
}

#ifdef CLIENT_DLL
#define	SO_WEAPON_BOB_CYCLE_MIN		1.0f
#define	SO_WEAPON_BOB_CYCLE_MAX		0.45f
#define	SO_WEAPON_BOB				0.002f
#define	SO_WEAPON_BOB_UP			0.5f

extern float g_lateralBob;
extern float g_verticalBob;

float CWeaponSOBase::CalcViewmodelBob( void )
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;

	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it
	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	speed = clamp( speed, -320, 320 );

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime/SO_WEAPON_BOB_CYCLE_MAX)*SO_WEAPON_BOB_CYCLE_MAX;
	cycle /= SO_WEAPON_BOB_CYCLE_MAX;

	if ( cycle < SO_WEAPON_BOB_UP )
	{
		cycle = M_PI * cycle / SO_WEAPON_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-SO_WEAPON_BOB_UP)/(1.0 - SO_WEAPON_BOB_UP);
	}

	g_verticalBob = speed*0.005f;
	g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

	g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f ) * BobScale();

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime/SO_WEAPON_BOB_CYCLE_MAX*2)*SO_WEAPON_BOB_CYCLE_MAX*2;
	cycle /= SO_WEAPON_BOB_CYCLE_MAX*2;

	if ( cycle < SO_WEAPON_BOB_UP )
	{
		cycle = M_PI * cycle / SO_WEAPON_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-SO_WEAPON_BOB_UP)/(1.0 - SO_WEAPON_BOB_UP);
	}

	g_lateralBob = speed*0.005f;
	g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
	g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f ) * BobScale();

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

float CWeaponSOBase::SwayScale( void )
{
	if( m_bIronsighted )
		return 0.25f;

	return 1.0f;
}

ConVar vm_ironsight_bobscale( "vm_ironsight_bobscale", "0.15", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_CHEAT );
float CWeaponSOBase::BobScale( void )
{
	if( m_bIronsighted )
		return vm_ironsight_bobscale.GetFloat();

	return 1.0f;
}
#endif


// ironsight macros
bool CWeaponSOBase::HasIronsights( void )
{
	if( vm_ironsight_adjust.GetBool() == true )
		return true;

	return GetHL2MPWpnData().bHasIronsights;
}

float CWeaponSOBase::GetIronsightFOV( void )
{
	if( vm_ironsight_adjust.GetBool() )
		return vm_ironsight_fov.GetFloat();

	return GetHL2MPWpnData().flIronsightFOV;
}

float CWeaponSOBase::GetIronsightTime( void )
{
	if( vm_ironsight_adjust.GetBool() )
		return vm_ironsight_time.GetFloat();

	return GetHL2MPWpnData().flIronsightTime;
}

Vector CWeaponSOBase::GetIronsightPosition( void ) const
{
	if( vm_ironsight_adjust.GetBool() )
		return Vector( vm_ironsight_x.GetFloat(), vm_ironsight_y.GetFloat(), vm_ironsight_z.GetFloat() );

	return GetHL2MPWpnData().vecIronsightOffset;
}

QAngle CWeaponSOBase::GetIronsightAngles( void ) const
{
	if( vm_ironsight_adjust.GetBool() )
		return QAngle( vm_ironsight_pitch.GetFloat(), vm_ironsight_yaw.GetFloat(), vm_ironsight_roll.GetFloat() );

	return GetHL2MPWpnData().angIronsightAngs;
}

float CWeaponSOBase::GetWeaponPrice( void ) const
{
	return GetHL2MPWpnData().m_iWeaponPrice;
}

float CWeaponSOBase::GetWeaponAmmoPrice( void ) const
{
	return GetHL2MPWpnData().m_iWeaponPriceAmmo;
}

void CWeaponSOBase::SetIronsights( bool b )
{
	if( !HasIronsights() || m_bInReload || b == m_bIronsighted )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if( !pPlayer )
		return;

	m_bIronsighted = b;

	if( m_flIronsightTime + GetIronsightTime() > gpGlobals->curtime )
	{
		float diff =  m_flIronsightTime + GetIronsightTime() - gpGlobals->curtime;
		m_flIronsightTime = gpGlobals->curtime - diff;
	}
	else
		m_flIronsightTime = gpGlobals->curtime;

	if( gpGlobals->curtime >= m_flNextPrimaryAttack )
		SetWeaponIdleTime( gpGlobals->curtime );

#ifndef CLIENT_DLL
	pPlayer->SetFOV( this, b ? GetIronsightFOV() : 0, GetIronsightTime() );
#endif
}

void CWeaponSOBase::EnterIronsights( void )
{
	SetIronsights( true );
}


void CWeaponSOBase::ExitIronsights( void )
{
	SetIronsights( false );
}

#ifdef CLIENT_DLL
ConVar so_cl_ironsight_sensitivity( "so_cl_ironsight_sensitivity", "0.75", FCVAR_CLIENTDLL + FCVAR_ARCHIVE, "Sensitivity multiplier for ironsight mode.", true, 0.1f, true, 2.0f );
void CWeaponSOBase::OverrideMouseInput( float *x, float *y )
{
	if ( m_bIronsighted )
	{
		*x *= so_cl_ironsight_sensitivity.GetFloat();
		*y *= so_cl_ironsight_sensitivity.GetFloat();
	}
}
#else // server
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSOBase::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( BulletsToShoot(), vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, GetHL2MPWpnData().m_iPlayerDamage, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSOBase::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSOBase::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	DevMsg( "CWeaponSOBase got anim event %d\n", pEvent->event );
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SMG1:
		case EVENT_WEAPON_AR2:
		case EVENT_WEAPON_SHOTGUN_FIRE:
		case EVENT_WEAPON_SNIPER_RIFLE_FIRE:
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;

			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			{
				vecShootOrigin = pOperator->Weapon_ShootPosition();
			}

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
		}
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
void CWeaponSOBase::DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime )
{
	#define	KICK_MIN_X			0.2f	//Degrees
	#define	KICK_MIN_Y			0.2f	//Degrees
	#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;

	//Find how far into our accuracy degradation we are
	float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset( 10 );

	//Apply this to the view angles as well
	vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
	vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
	vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

	RandomSeed( iSeed );

	//Wibble left and right
	if ( RandomInt( -1, 1 ) >= 0 )
		vecScratch.y *= -1;

	iSeed++;

	//Wobble up and down
	if ( RandomInt( -1, 1 ) >= 0 )
		vecScratch.z *= -1;

	//Clip this to our desired min/max
	UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );

	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch( vecScratch * 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: Make enough sound events to fill the estimated think interval
// returns: number of shots needed
//-----------------------------------------------------------------------------
int CWeaponSOBase::WeaponSoundRealtime( WeaponSound_t shoot_type )
{
	int numBullets = 0;

	// ran out of time, clamp to current
	if (m_flNextSoundTime < gpGlobals->curtime)
	{
		m_flNextSoundTime = gpGlobals->curtime;
	}

	// make enough sound events to fill up the next estimated think interval
	float dt = clamp( m_flAnimTime - m_flPrevAnimTime, 0, 0.2 );
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}

	return numBullets;
}


//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
extern ConVar so_infinite_ammo;
void CWeaponSOBase::FinishReload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner)
	{
		// If I use primary clips, reload primary
		if ( UsesClipsForAmmo1() )
		{
			int primary	= min( GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
			m_iClip1 = m_bMagazineStyleReloads ? GetMaxClip1() : m_iClip1 + primary;

			// S:O - Clip System for Weapons
			if( !so_infinite_ammo.GetBool() )
			{
				pOwner->RemoveAmmo( m_bMagazineStyleReloads ? 1 : primary, m_iPrimaryAmmoType);
			}

			/*m_iClip1 += primary;
			pOwner->RemoveAmmo( primary, m_iPrimaryAmmoType);*/
		}

		// If I use secondary clips, reload secondary
		if ( UsesClipsForAmmo2() )
		{
			int secondary = min( GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
			m_iClip2 += secondary;
			pOwner->RemoveAmmo( secondary, m_iSecondaryAmmoType );
		}

		if ( m_bReloadsSingly )
		{
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSOBase::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Debounce the recoiling counter
	if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
	{
		m_nShotsFired = 0;
	}

	// ironsights, made it "semi-automatic"
	if( HasIronsights() && ( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		if( !m_bHasIronsightedThisClick )
		{
			m_bHasIronsightedThisClick = true;
			ToggleIronsights();
		}

		return;
	}
	else if( HasIronsights() && m_bHasIronsightedThisClick )
		m_bHasIronsightedThisClick = false;

	// S:O -
	// Not entirely sure exactly what this does, but it appears to be necessary, so let's keep it for now
	CWeaponPumpShotty *pPumpShotty = dynamic_cast<CWeaponPumpShotty *>( this );
	CWeaponAutoShotty *pAutoShotty = dynamic_cast<CWeaponAutoShotty *>( this );
	if ( !pPumpShotty && !pAutoShotty )
	{
		BaseClass::ItemPostFrame();

		if ( (pOwner->m_nButtons & IN_FIREMODE) && (gpGlobals->curtime >= m_flNextSecondaryAttack) && ((pOwner->GetWaterLevel() != 3) || (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == true)) )
		{
			AlternateFire();
		}
	}
}

// We must hide the scope if the weapon is ever dropped.
void CWeaponSOBase::Drop( const Vector &vecVelocity )
{
	if( IsIronsighted() )
		ExitIronsights();

	BaseClass::Drop( vecVelocity );
}

void CWeaponSOBase::AlternateFire( void )
{
	// By default this does nothing, since this should never be called directly.
	// Some weapons override this function to allow for alternate firing modes (EX: MP5K).
	return;
}

//==================================================
// Purpose: 
//==================================================
void CWeaponSOBase::WeaponIdle( void )
{
	//Only the player fires this way so we can cast
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner )
		return;

	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( GetIdleActivity() );
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() ); // hey! why not just put it here?
	}
}

void CWeaponSOBase::FireBullets( const FireBulletsInfo_t &info )
{
	if(CBasePlayer *pPlayer = ToBasePlayer ( GetOwner() ) )
	{
		pPlayer->FireBullets(info);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponSOBase::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	// Abort here to handle burst and auto fire modes
	if ( (UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType) ) )
		return;

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CHLMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if ( UsesClipsForAmmo1() )
	{
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	CHL2MP_Player *pHL2MPPlayer = ToHL2MPPlayer( pPlayer );

	// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire * BulletsToShoot();
	info.m_vecSrc = pHL2MPPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	info.m_vecSpread = pHL2MPPlayer->GetAttackSpread( this );
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;
	
	FireBullets( info );

	//Factor in the view kick
	AddViewKick();

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

}