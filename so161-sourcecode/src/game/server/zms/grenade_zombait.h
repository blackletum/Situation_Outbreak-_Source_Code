//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Molotov grenades
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEZOMBAIT_H
#define	GRENADEZOMBAIT_H

#include "basegrenade_shared.h"
#include "smoke_trail.h"

class CGrenade_Zombait : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenade_Zombait, CBaseGrenade );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	Detonate( void );
	void			ZombaitTouch( CBaseEntity *pOther );
	void			ZombaitThink( void );

protected:

	SmokeTrail *m_pFireTrail;

	DECLARE_DATADESC();

private:
	void BaitDemZombies( const Vector &targetPos );
	bool m_bDetonated;
};

#endif	//GRENADEZOMBAIT_H