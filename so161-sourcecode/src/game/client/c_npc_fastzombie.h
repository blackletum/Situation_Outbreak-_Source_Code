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

class C_FastZombie : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_FastZombie, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_FastZombie();
	virtual			~C_FastZombie();

private:
	C_FastZombie( const C_FastZombie & ); // not defined, not accessible
};