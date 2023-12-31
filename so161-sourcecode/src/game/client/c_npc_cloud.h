//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_Cloud : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_Cloud, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_Cloud();
	virtual			~C_Cloud();

private:
	C_Cloud( const C_Cloud & ); // not defined, not accessible
};