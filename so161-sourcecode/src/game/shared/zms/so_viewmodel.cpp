//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "predicted_viewmodel.h"
#include "so_viewmodel.h"
#include "weapon_sobase.h"
#include "weapon_sniperbase.h"

#ifdef CLIENT_DLL
#include "prediction.h" 
#include "ivieweffects.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( so_viewmodel, CSOViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( SOViewModel, DT_SOViewModel )

BEGIN_NETWORK_TABLE( CSOViewModel, DT_SOViewModel )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CSOViewModel::CSOViewModel() : m_LagAnglesHistory("CSOViewModel::m_LagAnglesHistory")
{
	m_vLagAngles.Init();
	m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );
}
#else
CSOViewModel::CSOViewModel()
{
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSOViewModel::~CSOViewModel()
{
}

extern ConVar cl_wpn_sway_interp;
extern ConVar cl_wpn_sway_scale;

void CSOViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
#ifdef CLIENT_DLL
	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	float scale = cl_wpn_sway_scale.GetFloat();
	CWeaponSOBase *pWeapon = dynamic_cast< CWeaponSOBase* >( GetOwningWeapon() );

	//Allow weapon lagging
	if ( pWeapon )
		scale = pWeapon->SwayScale();

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
	Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= scale;
	origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;
#endif
}


void CSOViewModel::CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
	// UNDONE: Calc this on the server?  Disabled for now as it seems unnecessary to have this info on the server
#if defined( CLIENT_DLL )
	QAngle vmangoriginal = eyeAngles;
	QAngle vmangles = eyeAngles;
	Vector vmorigin = eyePosition;

	CWeaponSOBase *pWeapon = dynamic_cast< CWeaponSOBase* >( GetWeapon() );
	//Allow weapon lagging
	if ( pWeapon )
	{
		pWeapon->AddViewmodelBob( this, vmorigin, vmangles );
		CalcIronsights( vmorigin, vmangles );
	}

	// Add model-specific bob even if no weapon associated (for head bob for off hand models)
	//AddViewModelBob( owner, vmorigin, vmangles );
	// Add lag
	CalcViewModelLag( vmorigin, vmangles, vmangoriginal );

#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
	{
		// Let the viewmodel shake at about 10% of the amplitude of the player's view
		vieweffects->ApplyShake( vmorigin, vmangles, 0.1 );	
	}
#endif

	SetLocalOrigin( vmorigin );
	SetLocalAngles( vmangles );

#endif
}

#ifdef CLIENT_DLL
void CSOViewModel::CalcIronsights( Vector& pos, QAngle& ang )
{
	CWeaponSOBase *pWeapon = dynamic_cast< CWeaponSOBase* >( GetWeapon() );

	if ( !pWeapon )
		return;

	/*
	Delta is basically the percentage (decimal)
	*/
	float delta = clamp( ( ( gpGlobals->curtime - pWeapon->m_flIronsightTime ) / pWeapon->GetIronsightTime() ), 0.0f, 1.0f );
	float exp = ( pWeapon->IsIronsighted() ) ? delta : 1.0f - delta; //reverse interpolation

	if( exp == 0.0f ) //fully not ironsighted; save performance
		return;

	Vector newPos = pos;
	QAngle newAng = ang;

	Vector vForward, vRight, vUp, vOffset;
	AngleVectors( newAng, &vForward, &vRight, &vUp );
	vOffset = pWeapon->GetIronsightPosition();

	newPos += vForward * vOffset.x;
	newPos += vRight * vOffset.y;
	newPos += vUp * vOffset.z;
	newAng += pWeapon->GetIronsightAngles();
	//fov is handled by CBaseCombatWeapon

	pos += ( newPos - pos ) * exp;
	ang += ( newAng - ang ) * exp;

}

extern ConVar vm_ironsight_adjust;
bool CSOViewModel::ShouldDraw( )
{
	CWeaponSOSniperBase *pSniperWeapon = dynamic_cast< CWeaponSOSniperBase* >( GetOwningWeapon() );

	if( pSniperWeapon && pSniperWeapon->ShouldDrawScope() && !vm_ironsight_adjust.GetBool() )
		return false;

	return BaseClass::ShouldDraw();
}
#endif