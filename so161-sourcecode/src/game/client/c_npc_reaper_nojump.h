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

class C_ReaperNoJump : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_ReaperNoJump, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

					C_ReaperNoJump();
	virtual			~C_ReaperNoJump();

private:
	C_ReaperNoJump( const C_ReaperNoJump & ); // not defined, not accessible
};