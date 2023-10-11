//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: MapOverview.cpp: implementation of the CHudOverview class.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud_overview.h"
#include <vgui/isurface.h>
#include <vgui/ILocalize.h>
#include <filesystem.h>
#include <keyvalues.h>
#include <convar.h>
#include <mathlib/mathlib.h>
#include <iviewport.h>
#include <igameresources.h>
#include "gamevars_shared.h"
#include "spectatorgui.h"
#include "c_playerresource.h"

#include "Sprite.h"
#include "SpriteTrail.h"

#include "clientmode.h"
#include <vgui_controls/AnimationController.h>

#include "c_npc_turret_floor.h"
#include "c_npc_citizen17.h"
#include "c_npc_combine_s.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static ConVar radar("radar", "1", 0, "Toggles map overview/radar system (useage enforced by server using so_hud)");
extern ConVar so_radar;

CHudOverview::CHudOverview( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudOverview" )
{
	Panel *pParent = g_pClientMode->GetViewport(); 

    SetParent( pParent );    
    
    SetVisible( true );//initially set our radar to visible even as a spectator 
	
	SetBounds( 0,0, 256, 256 );
	SetBgColor( Color( 0,0,0,128 ) );
	SetPaintBackgroundEnabled( false );
	SetVisible( true );

	// Make sure we actually have the font...
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	
	m_hIconFont = pScheme->GetFont( "DefaultSmall" );
	
	m_nMapTextureID = -1;
	m_MapKeyValues = NULL;

	m_MapOrigin = Vector( 0, 0, 0 );
	m_fMapScale = 1.0f;
	m_bFollowAngle = false;
    SetMode( MAP_MODE_FULL );

	m_fZoom = 1.0f;
	m_MapCenter = Vector2D( 512, 512 );
	m_ViewOrigin = Vector2D( 512, 512 );
	m_fViewAngle = 0;
	m_fTrailUpdateInterval = 1.0f;

	m_bShowNames = true;
	m_bShowHealth = true;
	m_bShowTrails = true;

	m_flChangeSpeed = 1000;
	m_flIconSize = 132.0f;

	m_ObjectCounterID = 1;

	//Reset
	m_fNextUpdateTime = 0;

	InitTeamColorsAndIcons();

	m_nIcon = surface()->CreateNewTextureID();
	//g_pMapOverview = this;  // for cvars access etc	

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudOverview::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings( scheme );

	SetBgColor( Color( 0,0,0,128 ) );
	SetPaintBackgroundEnabled( false );
}

void CHudOverview::Init( void )
{
	// register for events as client listener
	gameeventmanager->AddListener( this, "game_newmap", false );
}


void CHudOverview::InitTeamColorsAndIcons()
{
	Q_memset( m_TeamIcons, 0, sizeof(m_TeamIcons) );
	Q_memset( m_TeamColors, 0, sizeof(m_TeamColors) );
	Q_memset( m_ObjectIcons, 0, sizeof(m_ObjectIcons) );

	m_TextureIDs.RemoveAll();
}


int CHudOverview::AddIconTexture(const char *filename)
{
	int index = m_TextureIDs.Find( filename );

    if ( m_TextureIDs.IsValidIndex( index ) )
	{
		// already known, return texture ID
		return m_TextureIDs.Element(index);
	}

	index = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( index , filename, true, false);

	m_TextureIDs.Insert( filename, index );

	return index;
}

CHudOverview::~CHudOverview()
{
	if ( m_MapKeyValues )
		m_MapKeyValues->deleteThis();

	//g_pMapOverview = NULL;
	gameeventmanager->RemoveListener(this);

	//TODO release Textures ? clear lists
}
//finds our overview file we created earlier and loads it
void CHudOverview::SetMap(const char * levelname)
{
	// Reset players and objects, even if the map is the same as the previous one
	m_Objects.RemoveAll();
	
	m_fNextTrailUpdate = m_fWorldTime;

	InitTeamColorsAndIcons();

	// load new KeyValues
	if ( m_MapKeyValues && Q_strcmp( levelname, m_MapKeyValues->GetName() ) == 0 )
	{
		return;	// map didn't change
	}

	if ( m_MapKeyValues )
		m_MapKeyValues->deleteThis();

	m_MapKeyValues = new KeyValues( levelname );

	char tempfile[MAX_PATH];
	Q_snprintf( tempfile, sizeof( tempfile ), "resource/overviews/%s.txt", levelname );
	
	if ( !m_MapKeyValues->LoadFromFile( filesystem, tempfile, "GAME" ) )
	{
		DevMsg( 1, "Error! CHudOverview::SetMap: couldn't load file %s.\n", tempfile );
		m_nMapTextureID = -1;
		return;
	}

	// TODO release old texture ?

	m_nMapTextureID = surface()->CreateNewTextureID();

	//if we have not uploaded yet, lets go ahead and do so
	surface()->DrawSetTextureFile( m_nMapTextureID, m_MapKeyValues->GetString("material"), true, false);

	int wide, tall;

	surface()->DrawGetTextureSize( m_nMapTextureID, wide, tall );

	if ( wide != tall )
	{
		DevMsg( 1, "Error! CHudOverview::SetMap: map image must be a square.\n" );
		m_nMapTextureID = -1;
		return;
	}

	m_MapOrigin.x	= m_MapKeyValues->GetInt("pos_x");
	m_MapOrigin.y	= m_MapKeyValues->GetInt("pos_y");
	m_fMapScale		= m_MapKeyValues->GetFloat("scale",1.0f);
	m_bRotateMap	= m_MapKeyValues->GetInt("rotate")!=0;
	m_fFullZoom = m_fZoom = m_MapKeyValues->GetFloat("zoom", 1.0f );
}

void CHudOverview::OnThink( void )
{
	togglePrint(); 
 
	//NeedsUpdate
	if ( m_fNextUpdateTime < gpGlobals->curtime == true)
	{
		Update();
		m_fNextUpdateTime = gpGlobals->curtime + 0.2f; // update 5 times a second
	}
}

void CHudOverview::SetMode(int mode)
{
	m_flChangeSpeed = 0; // change size instantly

	if ( mode == MAP_MODE_OFF )
	{
		SetVisible( false );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapOff" );
	}
	
	else if ( mode == MAP_MODE_FULL )
	{
		if ( m_nMode != MAP_MODE_OFF )
			m_flChangeSpeed = 1000; // zoom effect

		//new
		C_BasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();

		if ( pPlayer )//always follow the local player
			//SetFollowEntity
            m_nFollowEntity = pPlayer->entindex();
		//new

		//SetFollowEntity
		//m_nFollowEntity = 0;

		SetVisible( true );

		if ( mode != m_nMode )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomToLarge" );
		}
	}

	// finally set mode
	m_nMode = mode;
}

void CHudOverview::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		SetMap( event->GetString("mapname") );
	}
}

void CHudOverview::Update( void )
{
	// update settings
	m_bShowNames = true; //overview_names.GetBool();
	m_bShowHealth = true;//overview_health.GetBool();
	m_bFollowAngle = true;//!overview_locked.GetBool();
	m_fTrailUpdateInterval = 1.0;// overview_tracks.GetInt();

	m_fWorldTime = gpGlobals->curtime;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;
	
	int specmode = GetSpectatorMode();

	if ( specmode == OBS_MODE_IN_EYE || specmode == OBS_MODE_CHASE )
	{
		// follow target
		//SetFollowEntity
		m_nFollowEntity = GetSpectatorTarget();
	}
	else 
	{
		// follow ourself otherwise
		//SetFollowEntity
		m_nFollowEntity = pPlayer->entindex();
	}
}

void CHudOverview::DrawMapTexture()
{
	// now draw a box around the outside of this panel
	int x0, y0, x1, y1;
	int wide, tall;

	GetSize(wide, tall);
	x0 = 0; y0 = 0; x1 = wide - 2; y1 = tall - 2 ;
	

	if ( m_nMapTextureID < 0 )
		return;
	
	Vertex_t points[4] =
	{
		//upper left 0,0
		Vertex_t( MapToPanel ( Vector2D(0,0) ), Vector2D(0,0) ),

		//UPPER RIGHT 1023, 0
		Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,0) ), Vector2D(1,0) ),

		//LOWER LEFT 1023, 1
		Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,OVERVIEW_MAP_SIZE-1) ), Vector2D(1,1) ),

		Vertex_t( MapToPanel ( Vector2D(0,OVERVIEW_MAP_SIZE-1) ), Vector2D(0,1) )
	};

	int alpha = 255.0f * .5; clamp( alpha, 1, 255 );
	
	surface()->DrawSetColor( 255,255,255, alpha );
	surface()->DrawSetTexture( m_nMapTextureID );
	surface()->DrawTexturedPolygon( 4, points );
}

Vector2D CHudOverview::MapToPanel( const Vector2D &mappos )
{
	int pwidth, pheight; 
	Vector2D panelpos;
	float viewAngle = m_fViewAngle - 90.0f;

	GetSize(pwidth, pheight);

	Vector offset;
	offset.x = mappos.x - m_MapCenter.x;
	offset.y = mappos.y - m_MapCenter.y;
	offset.z = 0;

	if ( !m_bFollowAngle )
	{
		if ( m_bRotateMap )
			viewAngle = 90.0f;
		else
			viewAngle = 0.0f;
	}

	VectorYawRotate( offset, viewAngle, offset );

	// find the actual zoom from the animationvar m_fZoom and the map zoom scale
	//float fScale = (m_fZoom * m_fFullZoom) / OVERVIEW_MAP_SIZE;
	//float fScale = 2.25f / OVERVIEW_MAP_SIZE;
	float fScale = m_fZoom / OVERVIEW_MAP_SIZE;

	offset.x *= fScale;
	offset.y *= fScale;

	panelpos.x = (pwidth * 0.5f) + (pheight * offset.x);
	panelpos.y = (pheight * 0.5f) + (pheight * offset.y);

	return panelpos;
}


void CHudOverview::Paint()
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	
	if(!localPlayer || !localPlayer->IsAlive()  || localPlayer->GetTeamNumber() == TEAM_SPECTATOR)
		return;	

	UpdateFollowEntity();
	DrawMapTexture();
	DrawMapPlayers();
	BaseClass::Paint();
}

void CHudOverview::SetCenter(const Vector2D &mappos)
{
	int width, height;

	GetSize( width, height);

	m_ViewOrigin = mappos;
	m_MapCenter = mappos;

	float fTwiceZoom = m_fZoom * m_fFullZoom * 2;

	width = height = OVERVIEW_MAP_SIZE / (fTwiceZoom);

	if ( m_MapCenter.x < width )
		m_MapCenter.x = width;

	if ( m_MapCenter.x > (OVERVIEW_MAP_SIZE-width) )
		m_MapCenter.x = (OVERVIEW_MAP_SIZE-width);

	if ( m_MapCenter.y < height )
		m_MapCenter.y = height;

	if ( m_MapCenter.y > (OVERVIEW_MAP_SIZE-height) )
		m_MapCenter.y = (OVERVIEW_MAP_SIZE-height);

	//center if in full map mode
	//if ( m_fZoom <= 1.0 )
	//{
	//	m_MapCenter.x = OVERVIEW_MAP_SIZE/2;
	//	m_MapCenter.y = OVERVIEW_MAP_SIZE/2;
	//}
}

void CHudOverview::SetData(KeyValues *data)
{
	m_fZoom = 1.00f;
	m_nFollowEntity =  data->GetInt( "entity", m_nFollowEntity );
}

Vector2D CHudOverview::WorldToMap( const Vector &worldpos )
{
	Vector2D offset( worldpos.x - m_MapOrigin.x, worldpos.y - m_MapOrigin.y);

	offset.x /=  m_fMapScale;
	offset.y /= -m_fMapScale;

	return offset;
}

bool CHudOverview::DrawIcon( MapObject_t *obj )
{
//	CHudTexture *textureID = obj->icon;
	
	Vector pos = obj->position;
	float scale = obj->size;
	float angle = obj->angle[YAW];
	const char *radarimage = obj->radarimage;
	
	Vector offset;	offset.z = 0;
	
	Vector2D pospanel = WorldToMap( pos );
	pospanel = MapToPanel( pospanel );

	if ( !IsInPanel( pospanel ) )
		return false; // player is not within overview panel

	offset.x = -scale;	offset.y = scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos1 = WorldToMap( pos + offset );

	offset.x = scale;	offset.y = scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos2 = WorldToMap( pos + offset );

	offset.x = scale;	offset.y = -scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos3 = WorldToMap( pos + offset );

	offset.x = -scale;	offset.y = -scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos4 = WorldToMap( pos + offset );

	Vertex_t points[4] =
	{
		Vertex_t( MapToPanel ( pos1 ), Vector2D(0,0) ),
		Vertex_t( MapToPanel ( pos2 ), Vector2D(1,0) ),
		Vertex_t( MapToPanel ( pos3 ), Vector2D(1,1) ),
		Vertex_t( MapToPanel ( pos4 ), Vector2D(0,1) )
	};
	
	
	//surface()->DrawSetColor(obj->color.r(), obj->color.g(), obj->color.b(), 192);
	surface()->DrawSetTextureFile( m_nIcon, radarimage, true, false);
	
	surface()->DrawTexturedPolygon( 4, points );

	int d = GetPixelOffset( scale);	

	pospanel.y += d + 4;

	return true;
}

bool CHudOverview::CanPlayerBeSeen(C_BasePlayer *pPlayer)
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !localPlayer )
		return false;

	// don't draw ourself
	//if ( localPlayer->entindex() == (pPlayer->entindex()) )
	//	return false;

	// if local player is on spectator team, he can see everyone
	if ( localPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return false;

	// we never track unassigned or real spectators
	if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return false;

	// if observer is an active player, check mp_forcecamera:

	if ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE )
		return false;

	if ( mp_forcecamera.GetInt() == OBS_ALLOW_TEAM )
	{
		// true if both players are on the same team
		return (localPlayer->GetTeamNumber() == pPlayer->GetTeamNumber() );
	}

	// by default we can see all players
	return true;
}

bool CHudOverview::IsInPanel(Vector2D &pos)
{
	int x,y,w,t;

	GetBounds( x,y,w,t );

	return ( pos.x >= 0 && pos.x < w && pos.y >= 0 && pos.y < t );
}

int CHudOverview::GetPixelOffset( float height )
{
	Vector2D pos2 = WorldToMap( Vector( height,0,0) );
	pos2 = MapToPanel( pos2 );

	Vector2D pos3 = WorldToMap( Vector(0,0,0) );
	pos3 = MapToPanel( pos3 );

	int a = pos2.y-pos3.y; 
	int b = pos2.x-pos3.x;

	return (int)sqrt((float)(a*a+b*b)); // number of panel pixels for "scale" units in world
}
void CHudOverview::togglePrint()
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !radar.GetBool() || (localPlayer && (localPlayer->GetTeamNumber() != 3 && localPlayer->GetTeamNumber() != 4)) || (so_radar.GetInt() == 0) )
		this->SetVisible(false);
	else
		this->SetVisible(true);
}

void CHudOverview::UpdateFollowEntity()
{
	if ( m_nFollowEntity != 0 )
	{
		C_BaseEntity *ent = ClientEntityList().GetEnt( m_nFollowEntity );

		if ( ent )
		{
			SetCenter( WorldToMap(ent->EyePosition()) );
			//SetAngle
			m_fViewAngle = ent->EyeAngles()[YAW];
		}
	}
	else
	{
		SetCenter( Vector2D(OVERVIEW_MAP_SIZE/2,OVERVIEW_MAP_SIZE/2) );
		//SetAngle
		m_fViewAngle = 0;
	}
}

void CHudOverview::DrawMapPlayers()
{
	surface()->DrawSetTextFont( m_hIconFont );
	
	for ( int i=0; i<500; i++ )
	{
		C_BaseEntity *pEnt = (C_BaseEntity *)ClientEntityList().GetBaseEntity( i );

		if ( pEnt && pEnt->IsPlayer() )
		{
			if ( pEnt->GetTeamNumber() == TEAM_SPECTATOR )
				continue;

			if ( !pEnt->IsAlive() )
				continue;

			const char *radarImage = "NULL";

			// We're a player, so draw the inside of our dot green to make us stand out from NPCs on the radar
			if ( pEnt->GetTeamNumber() == 3 )
				radarImage = "sprites/radar_survivors_player";
			else if ( pEnt->GetTeamNumber() == 4 )
				radarImage = "sprites/radar_military_player";
			else
				radarImage = "sprites/radar_zombies_player";

			MapObject_t tempObj;
			
			memset( &tempObj, 0, sizeof(MapObject_t) );

			//tempObj.position = pEnt->EyePosition();
			tempObj.size = m_flIconSize;

			/*CBasePlayer *pPlayer = static_cast<CBasePlayer*>(pEnt);

			if ( pPlayer->IsLocalPlayer() )
			{
				tempObj.position = pEnt->GetLocalOrigin();
				tempObj.angle = pEnt->GetLocalAngles();
			}
			else
			{
				tempObj.position = pEnt->GetNetworkOrigin();
				tempObj.angle = pEnt->GetNetworkAngles();
			}*/

			tempObj.position = pEnt->EyePosition();
			tempObj.angle = pEnt->EyeAngles();

			//tempObj.angle = pEnt->GetAbsAngles();
			tempObj.radarimage = radarImage;
			DrawIcon( &tempObj );
		}
		else if ( pEnt && pEnt->IsNPC() )
		{
			if ( !pEnt->IsAlive() )
				continue;

			// Check to see if we're dealing with a survivor or turret NPC for the color of our dot on the radar!
			C_NPC_Citizen *pSurvivor = dynamic_cast<C_NPC_Citizen*>(pEnt);
			C_NPC_FloorTurret *pTurret = dynamic_cast<C_NPC_FloorTurret*>(pEnt);
			C_NPC_CombineS *pSoldier = dynamic_cast<C_NPC_CombineS*>(pEnt);

			const char *radarImage = "NULL";

			// We're an NPC, so let's make sure players know that by giving us a white interior dot color on the radar
			if ( pSurvivor || pTurret )
				radarImage = "sprites/radar_survivors";
			else if ( pSoldier )
				radarImage = "sprites/radar_military";
			else
				radarImage = "sprites/radar_zombies";

			MapObject_t tempObj;
			
			memset( &tempObj, 0, sizeof(MapObject_t) );

			tempObj.position = pEnt->EyePosition();
			tempObj.size = m_flIconSize;
			tempObj.angle = pEnt->GetAbsAngles();
			tempObj.radarimage = radarImage;
			DrawIcon( &tempObj );
		}
	}
}