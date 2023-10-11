//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL2MP_WEAPON_PARSE_H
#define HL2MP_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_parse.h"
#include "networkvar.h"


//--------------------------------------------------------------------------------------------------------
class CHL2MPSWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CHL2MPSWeaponInfo, FileWeaponInfo_t );
	
	CHL2MPSWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );


public:

	// S:O - Ironsights System
	bool					bHasIronsights;
	Vector					vecIronsightOffset;
	QAngle					angIronsightAngs;
	float					flIronsightFOV;
	float					flIronsightTime;
	float					m_flIronsightWalkSpeed;

	// S:O - Weapon pricing
	float m_iWeaponPrice;
	float m_iWeaponPriceAmmo;

	int m_iPlayerDamage;
};


#endif // HL2MP_WEAPON_PARSE_H
