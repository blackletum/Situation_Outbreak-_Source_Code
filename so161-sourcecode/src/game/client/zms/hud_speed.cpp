#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_hl2mp_player.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "c_prop_vehicle.h"

#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cl_hudspeed( "cl_hudspeed", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Unit of Speed: 1 for MPH; 2 for KMH", 1, 1, 2, 2);

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudSpeed : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudSpeed, CHudNumericDisplay );

public:
	CHudSpeed( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink( void );
	void CheckSpeedDisplay( void );

private:
	float CheckSpeedDisplayDelay;
};

DECLARE_HUDELEMENT( CHudSpeed );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudSpeed::CHudSpeed( const char *pElementName ) : BaseClass(NULL, "HudSpeed"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeed::Init( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeed::Reset( void )
{
	if( cl_hudspeed.GetInt() == 1 )
	{
		SetLabelText(L"MPH");
	}
	else if( cl_hudspeed.GetInt() == 2 )
	{
		SetLabelText(L"KMH");
	}

	CheckSpeedDisplayDelay = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeed::VidInit( void )
{
	Reset();
}

void CHudSpeed::OnThink( void )
{
	// We don't want to clog up the tubes, so let's check every tenth of a second instead of every frame.
	if ( gpGlobals->curtime >= CheckSpeedDisplayDelay )
	{
		CheckSpeedDisplay();
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MoneyHONEY");
		CheckSpeedDisplayDelay = gpGlobals->curtime + 0.1f;
	}
}

void CHudSpeed::CheckSpeedDisplay( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	
	if ( !pPlayer )
		return;
	
	if( (pPlayer->IsInAVehicle()) && (pPlayer->GetTeamNumber() == 3 || pPlayer->GetTeamNumber() == 4) && (pPlayer->IsAlive()) )
	{
		IClientVehicle *pVehicle = pPlayer->GetVehicle();
		
		if( pVehicle )
		{
			C_PropVehicleDriveable *pVehicle2 = dynamic_cast<C_PropVehicleDriveable*>(pVehicle);

			SetPaintEnabled(true);
			SetPaintBackgroundEnabled(true);

			if( cl_hudspeed.GetInt() == 1 )
			{
				SetLabelText(L"MPH");
				SetDisplayValue(pVehicle2->m_nSpeed);
			}
			else if ( cl_hudspeed.GetInt() == 2 )
			{
				SetLabelText(L"KMH");
				int speed = pVehicle2->m_nSpeed;
				speed = speed * 1.61;
				SetDisplayValue(speed);
			}
		}
	}
	else
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
	}
}