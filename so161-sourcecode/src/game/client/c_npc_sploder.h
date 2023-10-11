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

class C_SploderZombie : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_SploderZombie, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_SploderZombie();
	virtual			~C_SploderZombie();

private:
	C_SploderZombie( const C_SploderZombie & ); // not defined, not accessible
};