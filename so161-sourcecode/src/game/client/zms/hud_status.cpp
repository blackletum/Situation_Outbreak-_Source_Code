#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#ifndef CLIENT_DLL
#include "util.h"
#endif
#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Label.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "c_ai_basenpc.h"
#include "vgui_avatarimage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudTargetStatus : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudTargetStatus, Panel );
 
public:
   	CHudTargetStatus( const char *pElementName );

    void Init( void );
    void Reset( void );
	void VidInit( void );
	void OnThink();
	void CalculateTargetStatus( void );
	void UpdateExistingTarget( C_BasePlayer *pTargetPlayer, C_AI_BaseNPC *pTargetNPC, const char *TargetCondition, vgui::Label *m_pTextCondition );
	void UpdateTargetStatus( C_BasePlayer *pTargetPlayer, C_AI_BaseNPC *pTargetNPC, const char *TargetName, vgui::Label *m_pTextName, const char *TargetCondition, vgui::Label *m_pTextCondition, vgui::ImagePanel *m_pAvatar );
	void UpdatePlayerAvatar( C_BasePlayer *pPlayer, vgui::ImagePanel *m_pAvatar );
	bool TargetEntityCheck( CBaseEntity *pTargetEntity, int m_iSlot );

protected:
	vgui::Label* m_pTextName1;
	vgui::Label* m_pTextName2;
	vgui::Label* m_pTextName3;
	vgui::Label* m_pTextCondition1;
	vgui::Label* m_pTextCondition2;
	vgui::Label* m_pTextCondition3;
	vgui::ImagePanel* m_pAvatar1;
	vgui::ImagePanel* m_pAvatar2;
	vgui::ImagePanel* m_pAvatar3;
	CBaseEntity *pTargetEntity;
	CBaseEntity *pTargetedEntity1;
	CBaseEntity *pTargetedEntity2;
	CBaseEntity *pTargetedEntity3;

private:
	float m_fCalcStatusUpdateDelay;
	float ratio;
	int newTargetHealth;
	int newTargetMaxHealth;
	bool m_bSlot1Taken;
	bool m_bSlot2Taken;
	bool m_bSlot3Taken;
	bool m_bSkipCheck;
};

extern bool AvatarIndexLessFunc2( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs;
}

//DECLARE_HUDELEMENT( CHudTargetStatus );
CHudTargetStatus::CHudTargetStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTargetStatus" )
{
    Panel *pParent = g_pClientMode->GetViewport();
    SetParent( pParent );

	SetBgColor( Color( 0,0,0,200 ) );
  
    SetVisible( false );
    SetAlpha( 0 );
	SetProportional(true);

	m_pTextName1 = new vgui::Label( this, "TargetNameStatusText1", "" );
	m_pTextName1->SetContentAlignment( Label::a_west );
	m_pTextName1->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextName1->SetVisible( true );
	m_pTextName1->SetAlpha( 255 );
	m_pTextName2 = new vgui::Label( this, "TargetNameStatusText2", "" );
	m_pTextName2->SetContentAlignment( Label::a_west );
	m_pTextName2->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextName2->SetVisible( true );
	m_pTextName2->SetAlpha( 255 );
	m_pTextName3 = new vgui::Label( this, "TargetNameStatusText3", "" );
	m_pTextName3->SetContentAlignment( Label::a_west );
	m_pTextName3->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextName3->SetVisible( true );
	m_pTextName3->SetAlpha( 255 );

	m_pTextCondition1 = new vgui::Label( this, "TargetConditionStatusText1", "" );
	m_pTextCondition1->SetContentAlignment( Label::a_west );
	m_pTextCondition1->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextCondition1->SetVisible( true );
	m_pTextCondition1->SetAlpha( 255 );
	m_pTextCondition2 = new vgui::Label( this, "TargetConditionStatusText2", "" );
	m_pTextCondition2->SetContentAlignment( Label::a_west );
	m_pTextCondition2->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextCondition2->SetVisible( true );
	m_pTextCondition2->SetAlpha( 255 );
	m_pTextCondition3 = new vgui::Label( this, "TargetConditionStatusText3", "" );
	m_pTextCondition3->SetContentAlignment( Label::a_west );
	m_pTextCondition3->SetFgColor( Color( 255, 255, 255 ) );
	m_pTextCondition3->SetVisible( true );
	m_pTextCondition3->SetAlpha( 255 );

	m_pAvatar1 = new vgui::ImagePanel( this, "TargetAvatarStatusImage1" );
	m_pAvatar1->SetVisible( true );
	m_pAvatar1->SetAlpha( 255 );
	m_pAvatar2 = new vgui::ImagePanel( this, "TargetAvatarStatusImage2" );
	m_pAvatar2->SetVisible( true );
	m_pAvatar2->SetAlpha( 255 );
	m_pAvatar3 = new vgui::ImagePanel( this, "TargetAvatarStatusImage3" );
	m_pAvatar3->SetVisible( true );
	m_pAvatar3->SetAlpha( 255 );
}

void CHudTargetStatus::Init()
{
	Reset();
}

void CHudTargetStatus::Reset()
{
	m_fCalcStatusUpdateDelay = 0.0f;

	pTargetEntity = NULL;
	pTargetedEntity1 = NULL;
	pTargetedEntity2 = NULL;
	pTargetedEntity3 = NULL;

	m_bSlot1Taken = false;
	m_bSlot2Taken = false;
	m_bSlot3Taken = false;
	m_bSkipCheck = true;
}

void CHudTargetStatus::VidInit()
{
	Reset();
}

void CHudTargetStatus::OnThink()
{
	// We don't want to clog up the tubes, so let's check every three seconds instead of every frame.
	if ( gpGlobals->curtime >= m_fCalcStatusUpdateDelay )
	{
		if ( !m_bSkipCheck )
		{
			m_bSlot1Taken = false;
			m_bSlot2Taken = false;
			m_bSlot3Taken = false;
			CalculateTargetStatus();
		}
		else
		{
			m_bSkipCheck = false;
		}

		m_fCalcStatusUpdateDelay = gpGlobals->curtime + 1.5f;
	}
}

void CHudTargetStatus::UpdatePlayerAvatar( C_BasePlayer *pPlayer, vgui::ImagePanel *m_pAvatar )
{
	if ( m_pAvatar->GetImage() )
	{
		((CAvatarImage*)m_pAvatar->GetImage())->ClearAvatarSteamID();
	}

	if ( pPlayer && steamapicontext->SteamUtils() )
	{
		int iIndex = pPlayer->entindex();
		player_info_t pi;
		if ( engine->GetPlayerInfo(iIndex, &pi) )
		{
			if ( pi.friendsID )
			{
				CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

				if ( !m_pAvatar->GetImage() )
				{
					CAvatarImage *pImage = new CAvatarImage();
					m_pAvatar->SetImage( pImage );
				}

				CAvatarImage *pAvImage = ((CAvatarImage*)m_pAvatar->GetImage());
				pAvImage->SetAvatarSteamID( steamIDForPlayer );

				// Indent the image. These are deliberately non-resolution-scaling.
				pAvImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling

				m_pAvatar->SetSize( pAvImage->GetWide(), GetTall() );
				m_pAvatar->GetImage()->Paint();
				m_pAvatar->SetVisible( true );
				m_pAvatar->SetAlpha( 255 );
			}
			else
			{
				m_pAvatar->SetVisible( false );
				m_pAvatar->SetAlpha( 0 );
			}
		}
		else
		{
			m_pAvatar->SetVisible( false );
			m_pAvatar->SetAlpha( 0 );
		}
	}
	else
	{
		m_pAvatar->SetVisible( false );
		m_pAvatar->SetAlpha( 0 );
	}
}

void CHudTargetStatus::UpdateExistingTarget( C_BasePlayer *pTargetPlayer, C_AI_BaseNPC *pTargetNPC, const char *TargetCondition, vgui::Label *m_pTextCondition )
{
	// Players take priority
	if ( (pTargetPlayer) && (pTargetPlayer->GetTeamNumber() == 3 || pTargetPlayer->GetTeamNumber() == 4) && (pTargetPlayer->IsAlive()) )
	{
		newTargetHealth = pTargetPlayer->GetHealth();
		newTargetMaxHealth = pTargetPlayer->GetMaxHealth();
		ratio = (float)newTargetHealth / (float)newTargetMaxHealth;

		const char *TargetCondition = "NULL";

		if ( (ratio <= 1.00) && (ratio >= 0.00) )
		{
			if ( ratio >= 0.75 )
				TargetCondition = "Healthy";
			else if ( ratio >= 0.50 )
				TargetCondition = "Wounded";
			else if ( ratio >= 0.25 )
				TargetCondition = "Critical";
			else if ( ratio < 0.25 )
				TargetCondition = "Dying";
		}

		// If we didn't match the player with a valid condition, don't show our text
		if ( V_strcmp( TargetCondition, "NULL" ) == 0 )
		{
			m_pTextCondition->SetVisible( false );
			m_pTextCondition->SetAlpha( 0 );
		}
		else
		{
			m_pTextCondition->SetText( TargetCondition );
			m_pTextCondition->SizeToContents();
			m_pTextCondition->SetPos( 16, 112 );
			m_pTextCondition->SetVisible( true );
			m_pTextCondition->SetAlpha( 255 );
		}
	}
	else if ( pTargetNPC )
	{
		newTargetHealth = pTargetNPC->GetHealth();
		newTargetMaxHealth = pTargetNPC->GetMaxHealth();
		ratio = (float)newTargetHealth / (float)newTargetMaxHealth;

		const char *TargetCondition = "NULL";

		if ( (ratio <= 1.00) && (ratio >= 0.00) )
		{
			if ( ratio >= 0.75 )
				TargetCondition = "Healthy";
			else if ( ratio >= 0.50 )
				TargetCondition = "Wounded";
			else if ( ratio >= 0.25 )
				TargetCondition = "Critical";
			else if ( ratio < 0.25 )
				TargetCondition = "Dying";
		}

		// If we didn't match the NPC with a valid condition, don't show our text
		if ( V_strcmp( TargetCondition, "NULL" ) == 0 )
		{
			m_pTextCondition->SetVisible( false );
			m_pTextCondition->SetAlpha( 0 );
		}
		else
		{
			m_pTextCondition->SetText( TargetCondition );
			m_pTextCondition->SizeToContents();
			m_pTextCondition->SetPos( 16, 112 );
			m_pTextCondition->SetVisible( true );
			m_pTextCondition->SetAlpha( 255 );
		}
	}
	else
	{
		m_pTextCondition->SetVisible( false );
		m_pTextCondition->SetAlpha( 0 );
	}
}

void CHudTargetStatus::UpdateTargetStatus( C_BasePlayer *pTargetPlayer, C_AI_BaseNPC *pTargetNPC, const char *TargetName, vgui::Label *m_pTextName, const char *TargetCondition, vgui::Label *m_pTextCondition, vgui::ImagePanel *m_pAvatar )
{
	// Players take priority
	if ( (pTargetPlayer) && (pTargetPlayer->GetTeamNumber() == 3 || pTargetPlayer->GetTeamNumber() == 4) && (pTargetPlayer->IsAlive()) )
	{
		const char *TargetName = "NULL";

		if ( pTargetPlayer->GetPlayerName() )
			TargetName = pTargetPlayer->GetPlayerName();

		// If we didn't match the player with a valid name, don't show our text
		if ( V_strcmp( TargetName, "NULL" ) == 0 )
		{
			m_pTextName->SetVisible( false );
			m_pTextName->SetAlpha( 0 );
		}
		else
		{
			m_pTextName->SetText( TargetName );
			m_pTextName->SizeToContents();
			m_pTextName->SetVisible( true );
			m_pTextName->SetAlpha( 255 );
		}

		newTargetHealth = pTargetPlayer->GetHealth();
		newTargetMaxHealth = pTargetPlayer->GetMaxHealth();
		ratio = (float)newTargetHealth / (float)newTargetMaxHealth;

		const char *TargetCondition = "NULL";

		if ( (ratio <= 1.00) && (ratio >= 0.00) )
		{
			if ( ratio >= 0.75 )
				TargetCondition = "Healthy";
			else if ( ratio >= 0.50 )
				TargetCondition = "Wounded";
			else if ( ratio >= 0.25 )
				TargetCondition = "Critical";
			else if ( ratio < 0.25 )
				TargetCondition = "Dying";
		}

		// If we didn't match the player with a valid condition, don't show our text
		if ( V_strcmp( TargetCondition, "NULL" ) == 0 )
		{
			m_pTextCondition->SetVisible( false );
			m_pTextCondition->SetAlpha( 0 );
		}
		else
		{
			m_pTextCondition->SetText( TargetCondition );
			m_pTextCondition->SizeToContents();
			m_pTextCondition->SetVisible( true );
			m_pTextCondition->SetAlpha( 255 );
		}

		UpdatePlayerAvatar( pTargetPlayer, m_pAvatar );
	}
	else if ( pTargetNPC )
	{
		m_pTextName->SetText( "Survivor" );
		m_pTextName->SizeToContents();
		m_pTextName->SetVisible( true );
		m_pTextName->SetAlpha( 255 );

		newTargetHealth = pTargetNPC->GetHealth();
		newTargetMaxHealth = pTargetNPC->GetMaxHealth();
		ratio = (float)newTargetHealth / (float)newTargetMaxHealth;

		const char *TargetCondition = "NULL";

		if ( (ratio <= 1.00) && (ratio >= 0.00) )
		{
			if ( ratio >= 0.75 )
				TargetCondition = "Healthy";
			else if ( ratio >= 0.50 )
				TargetCondition = "Wounded";
			else if ( ratio >= 0.25 )
				TargetCondition = "Critical";
			else if ( ratio < 0.25 )
				TargetCondition = "Dying";
		}

		// If we didn't match the NPC with a valid condition, don't show our text
		if ( V_strcmp( TargetCondition, "NULL" ) == 0 )
		{
			m_pTextCondition->SetVisible( false );
			m_pTextCondition->SetAlpha( 0 );
		}
		else
		{
			m_pTextCondition->SetText( TargetCondition );
			m_pTextCondition->SizeToContents();
			m_pTextCondition->SetVisible( true );
			m_pTextCondition->SetAlpha( 255 );
		}

		// NPCs don't have avatars at the moment
		m_pAvatar->SetVisible( false );
		m_pAvatar->SetAlpha( 0 );
	}
}

// Returns true if this target isn't currently being tracked
bool CHudTargetStatus::TargetEntityCheck( CBaseEntity *pTargetEntity, int m_iSlot )
{
	if ( m_iSlot == 1 )
	{
		if ( (pTargetEntity == pTargetedEntity2) || (pTargetEntity == pTargetedEntity3) )
			return false;
	}
	else if ( m_iSlot == 2 )
	{
		if ( (pTargetEntity == pTargetedEntity1) || (pTargetEntity == pTargetedEntity3) )
			return false;
	}
	else if ( m_iSlot == 3 )
	{
		if ( (pTargetEntity == pTargetedEntity1) || (pTargetEntity == pTargetedEntity2) )
			return false;
	}

	if ( !pTargetEntity->IsAlive() )
		return false;

	return true;
}

void CHudTargetStatus::CalculateTargetStatus( void )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if ( (pLocalPlayer) && (pLocalPlayer->GetTeamNumber() == 3 || pLocalPlayer->GetTeamNumber() == 4) && (pLocalPlayer->IsAlive()) )
	{
		SetVisible( true );
		SetAlpha( 255 );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MoneyHONEY");

		if ( !pTargetedEntity1 )
		{
			m_pTextName1->SetVisible( false );
			m_pTextName1->SetAlpha( 0 );
			m_pTextCondition1->SetVisible( false );
			m_pTextCondition1->SetAlpha( 0 );
			m_pAvatar1->SetVisible( false );
			m_pAvatar1->SetAlpha( 0 );

			pTargetedEntity1 = NULL;
			m_bSlot1Taken = false;
		}

		if ( !pTargetedEntity2 )
		{
			m_pTextName2->SetVisible( false );
			m_pTextName2->SetAlpha( 0 );
			m_pTextCondition2->SetVisible( false );
			m_pTextCondition2->SetAlpha( 0 );
			m_pAvatar2->SetVisible( false );
			m_pAvatar2->SetAlpha( 0 );

			pTargetedEntity2 = NULL;
			m_bSlot2Taken = false;
		}

		if ( !pTargetedEntity3 )
		{
			m_pTextName3->SetVisible( false );
			m_pTextName3->SetAlpha( 0 );
			m_pTextCondition3->SetVisible( false );
			m_pTextCondition3->SetAlpha( 0 );
			m_pAvatar3->SetVisible( false );
			m_pAvatar3->SetAlpha( 0 );

			pTargetedEntity3 = NULL;
			m_bSlot3Taken = false;
		}

		for ( CEntitySphereQuery sphere( pLocalPlayer->GetAbsOrigin(), 500.0f ); ( pTargetEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			CBasePlayer *pTargetPlayer = ToBasePlayer( pTargetEntity );
			C_AI_BaseNPC *pTargetNPC = pTargetEntity->MyNPCPointer();

			if ( pTargetEntity && !pTargetEntity->IsAlive() )
			{
				Warning("TARGET IS DEAD!\n");

				if ( pTargetEntity == pTargetedEntity1 )
				{
					Warning("TARGET IS REALLY DEAD!\n");
					m_pTextName1->SetVisible( false );
					m_pTextName1->SetAlpha( 0 );
					m_pTextCondition1->SetVisible( false );
					m_pTextCondition1->SetAlpha( 0 );
					m_pAvatar1->SetVisible( false );
					m_pAvatar1->SetAlpha( 0 );

					pTargetedEntity1 = NULL;
					m_bSlot1Taken = false;
				}
				else if ( pTargetEntity == pTargetedEntity2 )
				{
					m_pTextName2->SetVisible( false );
					m_pTextName2->SetAlpha( 0 );
					m_pTextCondition2->SetVisible( false );
					m_pTextCondition2->SetAlpha( 0 );
					m_pAvatar2->SetVisible( false );
					m_pAvatar2->SetAlpha( 0 );

					pTargetedEntity2 = NULL;
					m_bSlot2Taken = false;
				}
				else if ( pTargetEntity == pTargetedEntity3 )
				{
					m_pTextName3->SetVisible( false );
					m_pTextName3->SetAlpha( 0 );
					m_pTextCondition3->SetVisible( false );
					m_pTextCondition3->SetAlpha( 0 );
					m_pAvatar3->SetVisible( false );
					m_pAvatar3->SetAlpha( 0 );

					pTargetedEntity3 = NULL;
					m_bSlot3Taken = false;
				}
			}

			if ( (pTargetPlayer && (pTargetEntity != pLocalPlayer)) || (pTargetNPC && FClassnameIs(pTargetNPC, "class C_NPC_Citizen")) )
			{
				if ( !m_bSlot1Taken && TargetEntityCheck(pTargetEntity, 1) )
				{
					Warning("NO TARGET FOUND! FINDING TARGET...\n");
					const char *TargetName1 = "NULL";
					const char *TargetCondition1 = "NULL";

					UpdateTargetStatus( pTargetPlayer, 
										pTargetNPC, 
										TargetName1, 
										m_pTextName1, 
										TargetCondition1, 
										m_pTextCondition1, 
										m_pAvatar1 );

					m_pAvatar1->SetPos( 16, 16 );
					m_pTextName1->SetPos( 16, 64 );
					m_pTextCondition1->SetPos( 16, 112 );

					pTargetedEntity1 = pTargetEntity;
					m_bSlot1Taken = true;
				}
				else if ( !m_bSlot2Taken && TargetEntityCheck(pTargetEntity, 2) )
				{
					const char *TargetName2 = "NULL";
					const char *TargetCondition2 = "NULL";

					UpdateTargetStatus( pTargetPlayer, 
										pTargetNPC, 
										TargetName2, 
										m_pTextName2, 
										TargetCondition2, 
										m_pTextCondition2, 
										m_pAvatar2 );

					m_pAvatar2->SetPos( 16, 162 );
					m_pTextName2->SetPos( 16, 210 );
					m_pTextCondition2->SetPos( 16, 258 );

					pTargetedEntity2 = pTargetEntity;
					m_bSlot2Taken = true;
				}
				else if ( !m_bSlot3Taken && TargetEntityCheck(pTargetEntity, 3) )
				{
					const char *TargetName3 = "NULL";
					const char *TargetCondition3 = "NULL";

					UpdateTargetStatus( pTargetPlayer, 
										pTargetNPC, 
										TargetName3, 
										m_pTextName3, 
										TargetCondition3, 
										m_pTextCondition3, 
										m_pAvatar3 );

					m_pAvatar3->SetPos( 16, 308 );
					m_pTextName3->SetPos( 16, 356 );
					m_pTextCondition3->SetPos( 16, 404 );

					pTargetedEntity3 = pTargetEntity;
					m_bSlot3Taken = true;
				}
			}
		}
	}
	else
	{
		SetVisible( false );
		SetAlpha( 0 );

		m_pTextName1->SetVisible( false );
		m_pTextName1->SetAlpha( 0 );
		m_pTextCondition1->SetVisible( false );
		m_pTextCondition1->SetAlpha( 0 );
		m_pAvatar1->SetVisible( false );
		m_pAvatar1->SetAlpha( 0 );

		m_pTextName2->SetVisible( false );
		m_pTextName2->SetAlpha( 0 );
		m_pTextCondition2->SetVisible( false );
		m_pTextCondition2->SetAlpha( 0 );
		m_pAvatar2->SetVisible( false );
		m_pAvatar2->SetAlpha( 0 );

		m_pTextName3->SetVisible( false );
		m_pTextName3->SetAlpha( 0 );
		m_pTextCondition3->SetVisible( false );
		m_pTextCondition3->SetAlpha( 0 );
		m_pAvatar3->SetVisible( false );
		m_pAvatar3->SetAlpha( 0 );

		m_bSlot1Taken = false;
		m_bSlot2Taken = false;
		m_bSlot3Taken = false;
	}
}