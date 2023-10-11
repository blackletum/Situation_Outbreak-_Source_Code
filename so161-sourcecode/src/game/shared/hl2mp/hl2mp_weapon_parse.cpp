//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "hl2mp_weapon_parse.h"
#include "ammodef.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CHL2MPSWeaponInfo;
}

CHL2MPSWeaponInfo::CHL2MPSWeaponInfo()
{
	// S:O - Weapon pricing
	m_iWeaponPrice = 0.0f;
	m_iWeaponPriceAmmo = 0.0f;

	m_iPlayerDamage = 0;
}

void CHL2MPSWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// S:O - Ironsights System
	KeyValues *Ironsights = pKeyValuesData->FindKey( "Ironsights" );

	if( Ironsights )
	{
		bHasIronsights = true;

		vecIronsightOffset = Vector();
		vecIronsightOffset.x = Ironsights->GetFloat( "x", 0.0f );
		vecIronsightOffset.y = Ironsights->GetFloat( "y", 0.0f );
		vecIronsightOffset.z = Ironsights->GetFloat( "z", 0.0f );

		angIronsightAngs = QAngle();
		angIronsightAngs[ PITCH ] = Ironsights->GetFloat( "pitch", 0.0f );
		angIronsightAngs[ YAW ] = Ironsights->GetFloat( "yaw", 0.0f );
		angIronsightAngs[ ROLL ] = Ironsights->GetFloat( "roll", 0.0f );

		flIronsightTime = Ironsights->GetFloat( "time", 0.33f ); // time to ironsight

		DevMsg( "Got ironsights for %s, pos: %f %f %f, ang %f, %f, %f\n", szWeaponName, vecIronsightOffset.x,vecIronsightOffset.y,vecIronsightOffset.z,angIronsightAngs[ PITCH ],angIronsightAngs[ YAW ],angIronsightAngs[ ROLL ] );

		flIronsightFOV = Ironsights->GetFloat( "ZoomFOV", 60.0f );
		m_flIronsightWalkSpeed = Ironsights->GetFloat( "WalkSpeedMulti", 0.333f );
	}
	else
	{
		bHasIronsights = false;
	}

	// S:O - Weapon pricing
	m_iWeaponPrice = pKeyValuesData->GetFloat( "price", 0 );
	m_iWeaponPriceAmmo = pKeyValuesData->GetFloat( "priceammo", 0 );

	m_iPlayerDamage = pKeyValuesData->GetInt( "damage", 0 );
}