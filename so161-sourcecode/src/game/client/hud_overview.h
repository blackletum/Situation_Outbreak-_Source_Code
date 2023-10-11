//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: MiniMap.h: interface for the CMiniMap class.
//
// $NoKeywords: $
//=============================================================================//

//#if !defined HLTVPANEL_H
//#define HLTVPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <iviewport.h>
#include <mathlib/vector.h>
#include <igameevents.h>
#include <shareddefs.h>
#include <const.h>
#include "hudelement.h"
#include "Sprite.h"


#define MAX_TRAIL_LENGTH	30
#define OVERVIEW_MAP_SIZE	1024	// an overview map is 1024x1024 pixels

using namespace vgui;

class CHudOverview : public CHudElement, public /*vgui::*/Panel
{
	DECLARE_CLASS_SIMPLE( CHudOverview, /*vgui::*/Panel);

public:	
	enum
	{
		MAP_MODE_OFF = 0,
		MAP_MODE_INSET,
		MAP_MODE_FULL
	};

	CHudOverview( const char *pElementName );

	virtual ~CHudOverview();

public:	// private structures & types
	typedef struct {
		int		xpos;
		int		ypos;
	} FootStep_t;

	typedef struct MapObject_s {
		int		index;		// entity index if any
		int		icon;		// players texture icon ID
		Vector	position;	// current x,y pos
		QAngle	angle;		// view origin 0..360
		float	endtime;	// time stop showing object
		float	size;		// object size
		const char *radarimage; // S:O - Defines the base image to use for the radar
	} MapObject_t;

#define MAP_OBJECT_ALIGN_TO_MAP	(1<<0)

public: // IViewPortPanel interface:
	virtual void SetData(KeyValues *data);
	void togglePrint(); 

	virtual void OnThink();
	virtual void Update();
	virtual void Init( void );

	virtual bool IsVisible() { return BaseClass::IsVisible(); }

public: // IGameEventListener

	virtual void FireGameEvent( IGameEvent *event);
	
public:	// VGUI overrides

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	
public:
	// general settings:
	virtual void SetMap(const char * map);
	virtual void SetMode( int mode );
	virtual void SetCenter( const Vector2D &mappos); 
	virtual Vector2D WorldToMap( const Vector &worldpos );

	// rules that define if you can see a player on the overview or not
	virtual bool CanPlayerBeSeen(C_BasePlayer *pPlayer);

protected:
	virtual void	DrawMapTexture();
	virtual void	DrawMapPlayers();
	virtual void	InitTeamColorsAndIcons();

	bool			IsInPanel(Vector2D &pos);
	int				AddIconTexture(const char *filename);
	Vector2D		MapToPanel( const Vector2D &mappos );
	int				GetPixelOffset( float height );
	void			UpdateFollowEntity();
	void			UpdateObjects(); // objects bound to entities 

	virtual bool	DrawIcon( MapObject_t *obj );
	
	int				m_nMode;
	Vector2D		m_vPosition;
	Vector2D		m_vSize;
	float			m_flChangeSpeed;
	float			m_flIconSize;


	IViewPort *		m_pViewPort;

	CUtlDict< int, int> m_TextureIDs;

	CUtlVector<MapObject_t>	m_Objects;

	Color	m_TeamColors[MAX_TEAMS];
	int		m_TeamIcons[MAX_TEAMS];
	int		m_ObjectIcons[64];
	int		m_ObjectCounterID;
	vgui::HFont	m_hIconFont;

	
	bool m_bShowNames;
	bool m_bShowTrails;
	bool m_bShowHealth;
		
	int	 m_nMapTextureID;		// texture id for current overview image
	
	KeyValues * m_MapKeyValues; // keyvalues describing overview parameters

	Vector	m_MapOrigin;
	float	m_fMapScale;
	bool	m_bRotateMap;

	int		m_nFollowEntity;
	CPanelAnimationVar( float, m_fZoom, "zoom", "1.0f" );
	float	m_fFullZoom;	// best zoom factor for full map view (1.0 is map is a square) 
	Vector2D m_ViewOrigin;
	Vector2D m_MapCenter;

	float	m_fNextUpdateTime;
	float	m_fViewAngle;	// rotation of overview map
	float	m_fWorldTime;
	float   m_fNextTrailUpdate;
	float	m_fTrailUpdateInterval; // if -1 don't show trails
	bool	m_bFollowAngle;	// if true, map rotates with view angle

	int m_nIcon;
};
DECLARE_HUDELEMENT( CHudOverview);