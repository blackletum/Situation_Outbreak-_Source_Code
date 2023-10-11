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

class C_Ghost : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Ghost, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Ghost();
	virtual			~C_Ghost();

private:
	C_Ghost( const C_Ghost & ); // not defined, not accessible
};