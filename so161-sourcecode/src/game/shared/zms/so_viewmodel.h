//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SO_VIEWMODEL_H
#define SO_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"
#include "predicted_viewmodel.h"

#if defined( CLIENT_DLL )
#define CSOViewModel C_SOViewModel

ConVar haj_cl_viewmodel_cast_shadows( "so_cl_viewmodel_cast_shadows", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Should view model cast shadows?" );
ConVar haj_cl_viewmodel_receive_shadows( "so_cl_viewmodel_receive_shadows", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Should view model receive shadows/projected textures?" );
#endif

class CSOViewModel : public CPredictedViewModel
{
	DECLARE_CLASS( CSOViewModel, CPredictedViewModel );
public:

	DECLARE_NETWORKCLASS();

	CSOViewModel( void );
	virtual ~CSOViewModel( void );

	virtual void CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles );

#if defined( CLIENT_DLL )
	virtual void CalcIronsights( Vector& pos, QAngle& ang );

	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType() { if( haj_cl_viewmodel_cast_shadows.GetBool() ) { return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC; } return SHADOWS_NONE; }

	// Should this object receive shadows?
	virtual bool			ShouldReceiveProjectedTextures( int flags ) { return haj_cl_viewmodel_receive_shadows.GetBool(); }
	virtual bool			ShouldDraw( void );
#endif

private:

#if defined( CLIENT_DLL )

	// This is used to lag the angles.
	CInterpolatedVar<QAngle> m_LagAnglesHistory;
	QAngle m_vLagAngles;

	CSOViewModel( const CSOViewModel & ); // not defined, not accessible

#endif
};

#endif // PREDICTED_VIEWMODEL_H
