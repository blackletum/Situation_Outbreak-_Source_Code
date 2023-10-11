//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_playerresource.h"
#include "vgui_EntityPanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "hud_macros.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>

using namespace vgui;

#include "ConVar.h"

#include "c_npc_cloud.h"
#include "c_npc_fastzombie.h"
#include "c_npc_PoisonZombie.h"
#include "c_npc_reaper_nojump.h"
#include "c_npc_sploder.h"
#include "c_npc_zombie.h"
#include "c_npc_combine_s.h"
#include "c_npc_citizen17.h"
#include "c_npc_turret_floor.h"

#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "fmod_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar so_gamemode;
extern ConVar so_survivor_count;
extern ConVar so_zombie_count;
extern ConVar so_military_count;
extern ConVar so_music;

//-----------------------------------------------------------------------------
// S:O - Danger Level Text Display
//-----------------------------------------------------------------------------
class CHudDangerLevel : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudDangerLevel, Panel );

public:
	CHudDangerLevel( const char *pElementName );
	~CHudDangerLevel();

	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink();
	virtual void CalculateDangerLevel( void );

private:
	int nTargetCount;
	int m_iCurrentCount;
	int m_iTotalCount;
	const char *ThreatLevelText;
	const char *ThreatLevelTextDescription;
	float CalcDangerLevelDelay;
	CBaseEntity *ppTarget[256];
};

DECLARE_HUDELEMENT( CHudDangerLevel );

CHudDangerLevel::CHudDangerLevel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDangerLevel" )
{
	Panel *pParent = g_pClientMode->GetViewport();

	SetParent( pParent );
	SetProportional( true );
	SetVisible( false );
	SetAlpha( 0 );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

CHudDangerLevel::~CHudDangerLevel()
{
	FMODManager()->StopAmbientSound( false );
}

void CHudDangerLevel::Init()
{
	Reset();
}

void CHudDangerLevel::Reset()
{
	CalcDangerLevelDelay = 0.0;
}

void CHudDangerLevel::VidInit()
{
	Reset();
}

void CHudDangerLevel::OnThink()
{
	// We don't want to clog up the tubes, so let's check every five seconds or so instead of every frame
	if ( gpGlobals->curtime >= CalcDangerLevelDelay )
	{
		if ( so_music.GetBool() )
		{
			CalculateDangerLevel();
		}

		CalcDangerLevelDelay = gpGlobals->curtime + 5.0f;
	}
}

void CHudDangerLevel::CalculateDangerLevel( void )
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( pPlayer )
	{
		if ( pPlayer->IsAlive() )
		{
			if ( pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4 )
			{
				m_iCurrentCount = 0;
				m_iTotalCount = 0;
				nTargetCount = UTIL_EntitiesInSphere( ppTarget, 256, pPlayer->GetAbsOrigin(), 500.0f, 0 );

				for ( int i = 0; i < nTargetCount; i++ )
				{
					if ( !ppTarget[i] )
						continue;

					C_Cloud *pCloudNPC = dynamic_cast<C_Cloud*>( ppTarget[i] );
					C_FastZombie *pReaperNPC = dynamic_cast<C_FastZombie*>( ppTarget[i] );
					C_ReaperNoJump *pReaperNoJumpNPC = dynamic_cast<C_ReaperNoJump*>( ppTarget[i] );
					C_NPC_PoisonZombie *pSeekerNPC = dynamic_cast<C_NPC_PoisonZombie*>( ppTarget[i] );
					C_Zombie *pCreeperNPC = dynamic_cast<C_Zombie*>( ppTarget[i] );
					C_SploderZombie *pSploderNPC = dynamic_cast<C_SploderZombie*>( ppTarget[i] );

					if ( pPlayer->GetTeamNumber() == 3 )
					{
						C_NPC_CombineS *pSoldierNPC = dynamic_cast<C_NPC_CombineS*>( ppTarget[i] );

						if ( pCloudNPC || pReaperNPC || pReaperNoJumpNPC || pSeekerNPC || pCreeperNPC || pSploderNPC || pSoldierNPC )
						{
							m_iCurrentCount++;
						}
					}
					else if ( pPlayer->GetTeamNumber() == 4 )
					{
						C_NPC_Citizen *pSurvivorNPC = dynamic_cast<C_NPC_Citizen*>( ppTarget[i] );
						C_NPC_FloorTurret *pTurretNPC = dynamic_cast<C_NPC_FloorTurret*>( ppTarget[i] );

						if ( pCloudNPC || pReaperNPC || pReaperNoJumpNPC || pSeekerNPC || pCreeperNPC || pSploderNPC || pSurvivorNPC || pTurretNPC )
						{
							m_iCurrentCount++;
						}
					}
				}

				if ( pPlayer->GetTeamNumber() == 3 )
				{
					m_iTotalCount = so_zombie_count.GetInt() + so_military_count.GetInt();
				}
				else if ( pPlayer->GetTeamNumber() == 4 )
				{
					m_iTotalCount = so_zombie_count.GetInt() + so_survivor_count.GetInt();
				}

				float m_fThreatLevel = (float)m_iCurrentCount / (float)m_iTotalCount;
				if ( (m_fThreatLevel <= 1.00) && (m_fThreatLevel >= 0.00) )
				{
					if ( m_fThreatLevel >= 0.8 )
					{
						if ( !FMODManager()->IsSoundPlaying( "events/tier4.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "events/tier4.mp3" );
					}
					else if ( m_fThreatLevel >= 0.6 )
					{
						if ( !FMODManager()->IsSoundPlaying( "events/tier3.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "events/tier3.mp3" );
					}
					else if ( m_fThreatLevel >= 0.4 )
					{
						if ( !FMODManager()->IsSoundPlaying( "events/tier2.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "events/tier2.mp3" );
					}
					else if ( m_fThreatLevel >= 0.2 )
					{
						if ( !FMODManager()->IsSoundPlaying( "events/tier1.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "events/tier1.mp3" );
					}
					else if ( m_fThreatLevel < 0.2 )
					{
						if ( !FMODManager()->IsSoundPlaying( "ambient/so-ambience1.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "ambient/so-ambience1.mp3" );
					}
					else
					{
						/*int randambientmusic = RandomInt(1,2);
						switch ( randambientmusic )
						{
							case 1:*/
								if ( !FMODManager()->IsSoundPlaying( "ambient/so-ambience1.mp3" ) )
									FMODManager()->TransitionAmbientSounds( "ambient/so-ambience1.mp3" );
								/*break;

							case 2:
								if ( !FMODManager()->IsSoundPlaying( "events/tier1.mp3" ) )
									FMODManager()->TransitionAmbientSounds( "ambient/so-ambience2.mp3" );
								break;
						}*/
					}
				}
				else
				{
					/*int randambientmusic = RandomInt(1,2);
					switch ( randambientmusic )
					{
						case 1:*/
							if ( !FMODManager()->IsSoundPlaying( "ambient/so-ambience1.mp3" ) )
								FMODManager()->TransitionAmbientSounds( "ambient/so-ambience1.mp3" );
							/*break;

						case 2:
							if ( !FMODManager()->IsSoundPlaying( "events/tier1.mp3" ) )
								FMODManager()->TransitionAmbientSounds( "ambient/so-ambience2.mp3" );
							break;
					}*/
				}
			}
			else
			{
				/*int randambientmusic = RandomInt(1,2);
				switch ( randambientmusic )
				{
					case 1:*/
						if ( !FMODManager()->IsSoundPlaying( "ambient/so-ambience1.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "ambient/so-ambience1.mp3" );
						/*break;

					case 2:
						if ( !FMODManager()->IsSoundPlaying( "events/tier1.mp3" ) )
							FMODManager()->TransitionAmbientSounds( "ambient/so-ambience2.mp3" );
						break;
				}*/
			}
		}
		else
		{
			/*int randambientmusic = RandomInt(1,2);
			switch ( randambientmusic )
			{
				case 1:*/
					if ( !FMODManager()->IsSoundPlaying( "ambient/so-ambience1.mp3" ) )
						FMODManager()->TransitionAmbientSounds( "ambient/so-ambience1.mp3" );
					/*break;

				case 2:
					if ( !FMODManager()->IsSoundPlaying( "events/tier1.mp3" ) )
						FMODManager()->TransitionAmbientSounds( "ambient/so-ambience2.mp3" );
					break;
			}*/
		}
	}
}