//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_FloorTurret : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_NPC_FloorTurret, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_NPC_FloorTurret();
	virtual			~C_NPC_FloorTurret();

private:
	C_NPC_FloorTurret( const C_NPC_FloorTurret & ); // not defined, not accessible
};