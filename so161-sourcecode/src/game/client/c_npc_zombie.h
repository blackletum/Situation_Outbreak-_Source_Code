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

class C_Zombie : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Zombie, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Zombie();
	virtual			~C_Zombie();

private:
	C_Zombie( const C_Zombie & ); // not defined, not accessible
};