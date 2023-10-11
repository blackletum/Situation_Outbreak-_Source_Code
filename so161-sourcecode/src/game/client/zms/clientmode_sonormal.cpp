//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_sonormal.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "hl2mpclientscoreboard.h"
#include "zms/so_textwindow.h"
#include "ienginevgui.h"

#include "view_shared.h"
#include "buymenu_new.h"
#include "view.h"
#include "dlight.h"
#include "model_types.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeSONormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}

ClientModeSONormal* GetClientModeSONormal()
{
	Assert( dynamic_cast< ClientModeSONormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeSONormal* >( GetClientModeNormal() );
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual IViewPortPanel *CreatePanelByName( const char *szPanelName );
};

int ClientModeSONormal::GetDeathMessageStartHeight( void )
{
	int x = YRES(2);

	IViewPortPanel *spectator = gViewPortInterface->FindPanelByName( PANEL_SPECGUI );

	//TODO: Link to actual height of spectator bar
	if ( spectator && spectator->IsVisible() )
	{
		x += YRES(52);
	}

	return x;
}

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
	{
		newpanel = new CHL2MPClientScoreBoardDialog( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CSOTextWindow( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_SPECGUI, szPanelName) == 0 )
	{
		newpanel = new CSOSpectatorGUI( this );	
		return newpanel;
	}


	return BaseClass::CreatePanelByName( szPanelName ); 
}

//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeSONormal::ClientModeSONormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeSONormal::~ClientModeSONormal()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeSONormal::Init()
{
	BaseClass::Init();

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/CombinePanelScheme.res", "CombineScheme" );
	if (!g_hVGuiCombineScheme)
	{
		Warning( "Couldn't load combine panel scheme!\n" );
	}
}

CHandle<C_BaseAnimating> g_ClassImageWeapon;	// weapon

// Utility to determine if the vgui panel is visible
bool WillPanelBeVisible( vgui::VPANEL hPanel )
{
	while ( hPanel )
	{
		if ( !vgui::ipanel()->IsVisible( hPanel ) )
			return false;
		hPanel = vgui::ipanel()->GetParent( hPanel );
	}
	return true;
}

// Called to see if we should be creating or recreating the model instances
bool ShouldRecreateClassImageEntity( C_BaseAnimating* pEnt, const char* pNewModelName )
{
	if ( !pNewModelName || !pNewModelName[0] )
		return false;
	if ( !pEnt )
		return true;
 
	const model_t* pModel = pEnt->GetModel();
 
	if ( !pModel )
		return true;
	const char* pName = modelinfo->GetModelName( pModel );
 
	if ( !pName )
		return true;
	// reload only if names are different
	return( V_stricmp( pName, pNewModelName ) != 0 );
}

void UpdateClassImageEntity( 
		const char* pModelName,
		int x, int y, int width, int height )
{
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
 
	if ( !pLocalPlayer )
		return;
 
	C_BaseAnimating* pWeaponModel = g_ClassImageWeapon.Get();
 
	// Does the entity even exist yet?
	if ( ShouldRecreateClassImageEntity( pWeaponModel, pModelName ) )
	{
		if ( pWeaponModel )
			pWeaponModel->Remove();
 
		pWeaponModel = new C_BaseAnimating();
		pWeaponModel->InitializeAsClientEntity( pModelName, RENDER_GROUP_OPAQUE_ENTITY );
		pWeaponModel->AddEffects( EF_NODRAW ); // don't let the renderer draw the model normally
		g_ClassImageWeapon = pWeaponModel;
	}

	Vector origin = pLocalPlayer->EyePosition();
	Vector lightOrigin = origin;
 
	// find a spot inside the world for the dlight's origin, or it won't illuminate the model
	Vector testPos( origin.x - 100, origin.y, origin.z + 100 );
	trace_t tr;
	UTIL_TraceLine( origin, testPos, MASK_OPAQUE, pLocalPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0f )
		lightOrigin = tr.endpos;
	else
	{
		// Now move the model away so we get the correct illumination
		lightOrigin = tr.endpos + Vector( 1, 0, -1 );	// pull out from the solid
		Vector start = lightOrigin;
		Vector end = lightOrigin + Vector( 100, 0, -100 );
		UTIL_TraceLine( start, end, MASK_OPAQUE, pLocalPlayer, COLLISION_GROUP_NONE, &tr );
		origin = tr.endpos;
	}
 
	float ambient = engine->GetLightForPoint( origin, true ).Length();
 
	// Make a light so the model is well lit.
	// use a non-zero number so we cannibalize ourselves next frame
	dlight_t* dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC+1 );
 
	dl->flags = DLIGHT_NO_WORLD_ILLUMINATION;
	dl->origin = lightOrigin;
	// Go away immediately so it doesn't light the world too.
	dl->die = gpGlobals->curtime + 0.1f;
 
	dl->color.r = dl->color.g = dl->color.b = 250;
	if ( ambient < 1.0f )
		dl->color.exponent = 1 + (1 - ambient)*  2;
	dl->radius	= 400;

	// move player model in front of our view
	pWeaponModel->SetAbsOrigin( origin );
	pWeaponModel->SetAbsAngles( QAngle( 0, 270, 0 ) );
 
	pWeaponModel->FrameAdvance( gpGlobals->frametime );

	// Now draw it.
	CViewSetup view;
	// setup the views location, size and fov (amongst others)
	view.x = x;
	view.y = y;

	// This took a while to get exactly right, so I wouldn't recommended altering anything below...
	view.width = width*2.5;
	view.height = height*2;
 
	view.m_bOrtho = false;
	view.fov = 120;
 
	view.origin = origin + Vector( -25, -5, -5 );
 
	// make sure that we see all of the player model
	Vector vMins, vMaxs;
	pWeaponModel->C_BaseAnimating::GetRenderBounds( vMins, vMaxs );
	view.origin.z += ( vMins.z + vMaxs.z )*  0.5f;
 
	view.angles.Init();
	//view.m_vUnreflectedOrigin = view.origin;
	view.zNear = VIEW_NEARZ;
	view.zFar = 1000;
	//view.m_bForceAspectRatio1To1 = false;
 
	// render it out to the new CViewSetup area
	// it's possible that ViewSetup3D will be replaced in future code releases
	Frustum dummyFrustum;
 
	// New Function instead of ViewSetup3D...
	render->Push3DView( view, 0, NULL, dummyFrustum );
 
	if ( pWeaponModel )
	   pWeaponModel->DrawModel( STUDIO_RENDER );
 
	render->PopView( dummyFrustum );
}

void ClientModeSONormal::PostRenderVGui()
{
	// If the team menu is up, then render the model
	for ( int i=0; i < g_ClassImagePanels.Count(); i++ )
	{
		CClassImagePanel* pPanel = g_ClassImagePanels[i];
		if ( WillPanelBeVisible( pPanel->GetVPanel() ) )
		{
			// Ok, we have a visible class image panel.
			int x, y, w, h;
			pPanel->GetBounds( x, y, w, h );
			pPanel->LocalToScreen( x, y );
 
			// Allow for the border.
			x += 3;
			y += 5;
			w -= 2;
			h -= 10;
 
			UpdateClassImageEntity( g_ClassImagePanels[i]->m_ModelName, x, y, w, h );
			return;
		}
	}
}