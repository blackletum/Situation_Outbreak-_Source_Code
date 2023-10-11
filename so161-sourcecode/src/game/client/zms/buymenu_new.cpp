//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>

#include "buymenu_new.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <FileSystem.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>

#include "cdll_util.h"
#include "IGameUIFuncs.h" // for key bindings

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

#include <game/client/iviewport.h>

#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CUtlVector<CClassImagePanel*> g_ClassImagePanels;
 
CClassImagePanel::CClassImagePanel( vgui::Panel* pParent, const char* pName )
	: vgui::ImagePanel( pParent, pName )
{
	g_ClassImagePanels.AddToTail( this );
	m_ModelName[0] = 0;
}
 
CClassImagePanel::~CClassImagePanel()
{
	g_ClassImagePanels.FindAndRemove( this );
}
 
void CClassImagePanel::ApplySettings( KeyValues* inResourceData )
{
	const char* pName = inResourceData->GetString( "3DModel" );
	if ( pName )
		V_strncpy( m_ModelName, pName, sizeof( m_ModelName ) );
 
	BaseClass::ApplySettings( inResourceData );
}
 
void CClassImagePanel::Paint()
{
	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuyMenuNew::CBuyMenuNew(IViewPort *pViewPort) : Frame(NULL, PANEL_BUY)
{
	m_pViewPort = pViewPort;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

	SetTitle("CHOOSE YOUR WEAPON", true);

	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	SetTitleBarVisible(false);
	SetProportional(true);

	LoadControlSettings("Resource/UI/BuyMenu.res");
	LoadControlSettings("Resource/UI/MainBuyMenu.res");

	m_pPanel = new EditablePanel( this, "buymenunew" );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuyMenuNew::CBuyMenuNew(IViewPort *pViewPort, const char *panelName) : Frame(NULL, panelName)
{
	m_pViewPort = pViewPort;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

	SetTitle("CHOOSE YOUR WEAPON", true);

	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	SetTitleBarVisible(false);
	SetProportional(true);

	// The rest of the configuration is team-specific (see beginning of OnThink method)
	LoadControlSettings("Resource/UI/BuyMenu.res");
	LoadControlSettings("Resource/UI/MainBuyMenu.res");

	m_pPanel = new EditablePanel( this, "buymenunew" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuyMenuNew::~CBuyMenuNew()
{
}

Panel *CBuyMenuNew::CreateControlByName(const char *controlName)
{
	if ( V_stricmp( controlName, "ClassImagePanel" ) == 0 )
	{
		return new CClassImagePanel( NULL, controlName );
	}

	if ( V_stricmp( controlName, "Button" ) == 0 )
	{
		Button *newButton = new Button( this, controlName, "", this, NULL );
		if ( newButton )
		{
			m_Buttons.AddToTail( newButton );
			return newButton;
		}
	}

	return BaseClass::CreateControlByName( controlName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBuyMenuNew::Reset()
{
	/*for ( int i = 0 ; i < GetChildCount() ; ++i )
	{
		// Hide the subpanel for the MouseOverPanelButtons
		Button *pPanel = dynamic_cast<Button *>( GetChild( i ) );

		if ( pPanel )
		{
			pPanel->HidePage();
		}
	}

	// Turn the first button back on again (so we have a default description shown)
	Assert( m_mouseoverButtons.Count() );
	for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
	{
		if ( i == 0 )
		{
			m_mouseoverButtons[i]->ShowPage();	// Show the first page
		}
		else
		{
			m_mouseoverButtons[i]->HidePage();	// Hide the rest
		}
	}*/
}

void CBuyMenuNew::OnThink()
{
	int numButtonsHighlighted = 0;

	// Cycle through each button and see if any are highlighted
	for ( int i = 0; i < m_Buttons.Count(); i++ )
	{
		Button* pPanel = m_Buttons[i];
		if ( pPanel && pPanel->IsCursorOver() )
		{
			numButtonsHighlighted++;
			UpdateShownWeaponModel( i );
		}
	}

	if ( numButtonsHighlighted == 0 )
	{
		// Use button #10 because that is our 'close' button (NULL m_ModelName)
		// This makes it so that if a player hasn't highlighted any buttons, no wepaon model is displayed
		UpdateShownWeaponModel( 10 );
	}
}

void CBuyMenuNew::UpdateShownWeaponModel( int buttonToRead )
{
	// Attempts to update the shown model on our buymenu every frame
	const char *WeaponName = GetWeaponFromButton( buttonToRead );
	for ( int i = 0; i < g_ClassImagePanels.Count(); i++ )
	{
		Q_snprintf( g_ClassImagePanels[i]->m_ModelName, sizeof(g_ClassImagePanels[i]->m_ModelName), "%s", WeaponName );
	}
}

const char *CBuyMenuNew::GetWeaponFromButton( int buttonToRead )
{
	// Matches a button with its world model
	// NOTE: REMEMBER TO UPDATE THIS LIST WHEN A NEW BUYABLE WEAPON IS ADDED!!!
	switch( buttonToRead )
	{
	case 0:
		return "models/weapons/w_deagle.mdl";
	case 1:
		return "models/weapons/w_doublebarrel.mdl";
	case 2:
		return "models/weapons/w_mp5k.mdl";
	case 3:
		return "models/weapons/w_m4.mdl";
	case 4:
		return "models/weapons/w_sniper.mdl";
	case 5:
		return "models/weapons/w_ak47.mdl";
	case 6:
		return "models/weapons/w_mac10.mdl";
	case 7:
		return "models/weapons/w_grenade.mdl";
	case 8:
		return "models/weapons/w_igrenade.mdl";
	case 9:
		return "models/weapons/w_rpg.mdl";
	case 10:
		return "NULL";	// this is our 'close' button
	case 11:
		return "models/weapons/w_semishotty.mdl";
	case 12:
		return "models/weapons/w_benelli.mdl";
	case 13:
		return "models/weapons/w_healthkit.mdl";
	case 14:
		return "models/weapons/w_zombait.mdl";
	case 15:
		return "models/weapons/w_brokenbottle.mdl";
	case 16:
		return "models/weapons/w_cleaver.mdl";
	case 17:
		return "models/weapons/w_axe.mdl";
	case 18:
		return "models/weapons/w_katana.mdl";
	case 19:
		return "models/weapons/w_chainsaw.mdl";
	default:
		return "NULL";	// in case we forget to add a weapon to this list, this is sure to remind us
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CBuyMenuNew::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	Close();

	gViewPortInterface->ShowBackGround( false );

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: shows the class menu
//-----------------------------------------------------------------------------
void CBuyMenuNew::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );

		/*// load a default class page
		for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
		{
			if ( i == 0 )
			{
				m_mouseoverButtons[i]->ShowPage();	// Show the first page
			}
			else
			{
				m_mouseoverButtons[i]->HidePage();	// Hide the rest
			}
		}
		
		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}*/
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
	
	m_pViewPort->ShowBackGround( bShow );
}

void CBuyMenuNew::SetData(KeyValues *data)
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CBuyMenuNew::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the visibility of a button by name
//-----------------------------------------------------------------------------
void CBuyMenuNew::SetVisibleButton(const char *textEntryName, bool state)
{
	Button *entry = dynamic_cast<Button *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetVisible(state);
	}
}

void CBuyMenuNew::OnKeyCodePressed(KeyCode code)
{
	if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}