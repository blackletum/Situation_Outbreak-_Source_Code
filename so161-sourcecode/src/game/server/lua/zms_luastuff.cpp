#include "cbase.h"
#include "zms_luagameplay.h"
#include "gameinterface.h"
#include "networkstringtable_gamedll.h"

// AT THE END OF THE INCLUDES, JORDAN!
// THE END!!!!!! >=D
#include "tier0/memdbgon.h"

extern INetworkStringTable *g_pStringTableDownloadables;
extern "C"
{
	#include <../../lua5.1/include/lua.h>
	#include <../../lua5.1/include/lauxlib.h>
	#include <../../lua5.1/include/lualib.h>
}

	//////////////////////////////////////////////
	// Util Functions
	//////////////////////////////////////////////
int luaClientPrintAll(lua_State *L)
{
	int n = lua_gettop(L);    /* number of arguments */
	switch(n)
	{
	case 2:
		UTIL_ClientPrintAll( lua_tointeger(L,1), lua_tostring(L,2));
		break;
	case 3:
		UTIL_ClientPrintAll( lua_tointeger(L,1), lua_tostring(L,2),lua_tostring(L,3));
		break;
	case 4:
		UTIL_ClientPrintAll( lua_tointeger(L,1), lua_tostring(L,2),lua_tostring(L,3),lua_tostring(L,4));
		break;
	case 5:
		UTIL_ClientPrintAll( lua_tointeger(L,1), lua_tostring(L,2),lua_tostring(L,3),lua_tostring(L,4),lua_tostring(L,5));
		break;
	case 6:
		UTIL_ClientPrintAll( lua_tointeger(L,1), lua_tostring(L,2),lua_tostring(L,3),lua_tostring(L,4),lua_tostring(L,5),lua_tostring(L,6));
		break;
	}
	return 0;
}
 
int luaMsg(lua_State *L)
{
	Msg("%s\n",lua_tostring(L,1));
	return 0;
}
 
int luaConMsg(lua_State *L)
{
	return luaMsg(L);
}
 
int luaGetTime(lua_State *L)
{
	lua_pushnumber( L, gpGlobals->curtime );
	return 1;
}

int luaAddDownload(lua_State *L)
{
    if ( !networkstringtable ) {
        engine->LogPrint ( "networkstringtable PHAIL\n" );
        return 0;
    }
    INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
    if (!pDownloadablesTable)
        return 0;

        if (pDownloadablesTable->FindStringIndex(lua_tostring(L,1)) == INVALID_STRING_INDEX) {
          bool save = engine->LockNetworkStringTables(false);
          pDownloadablesTable->AddString(true, lua_tostring(L,1));
          engine->LockNetworkStringTables(save);
        }
        return 0;

}
int luaWeaponPrintName(lua_State *L)
{
	ZMSLuaGamePlay *g = GetGP();
	const char* luaWep = lua_tostring(L,1);

	if (!luaWep)
	{
		lua_pushlstring(L, "NULL NAME", 9);
		return 1;
	}

	char wep[255];
	Q_snprintf(wep, 255, "weapon_%s", luaWep);


	int id = g->AliastoWeaponID(wep);
	const char* name = g->WeaponIDtoAlias(id);

	if (!name)
	{
		lua_pushlstring(L, "NULL NAME", 9);
		return 1;
	}

	lua_pushstring( L,  name);
	return 1;
}

int luaWeaponName(lua_State *L)
{
	ZMSLuaGamePlay *g = GetGP();
	const char* weapName = g->WeaponIDtoAlias(lua_tointeger(L,1));
	if (!weapName)
	{
		lua_pushlstring( L,  "NULL", 4);
		return 1;
	}

	char smallname[255];
	Q_snprintf(smallname, 255, "%s", weapName+7);

	lua_pushlstring( L,  smallname, strlen(smallname));
	return 1;
}

int luaCreateCVar(lua_State *L)
{
	int n = lua_gettop(L); 

	if (n != 3)
		return 0;
	ConVar (lua_tostring(L, 1), lua_tostring(L, 2), FCVAR_GAMEDLL, lua_tostring(L, 3));
	
	return 0;
}

int luaLoadConfig(lua_State *L)
{
	//CGEGameMode* pGameMode = GEGamePlay()->GetCurrentGameMode();

	//if (pGameMode)
	//	pGameMode->LoadConfig();

	return 0;
}


int luaGetCVar(lua_State *L)
{
	const ConVar *cvar = dynamic_cast< const ConVar* >( g_pCVar->FindCommandBase( lua_tostring(L,1) ) );

	if (!cvar)
	{
		lua_pushstring( L,  "NULL CVAR");
		return 1;
	}

	lua_pushstring( L, cvar->GetString());
	return 1;
}

int luaWeaponId(lua_State *L)
{
	ZMSLuaGamePlay *g = GetGP();
	const char* luaWep = lua_tostring(L,1);

	if (!luaWep)
	{
		lua_pushinteger(L, -1);
		return 1;
	}

	char wep[255];
	Q_snprintf(wep, 255, "weapon_%s", luaWep);

	lua_pushinteger( L,  g->AliastoWeaponID(wep));
	return 1;
}

	//////////////////////////////////////////////
	// Player Functions
	//////////////////////////////////////////////
int luaIsValidPlayer(lua_State *L)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );
	if (!pPlayer) {
		lua_pushboolean(L, 0);
		return 1;
	}
	if( FNullEnt( pPlayer->edict() ) || !pPlayer->IsPlayer() || !pPlayer->IsConnected())
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}

int luaGetPlayerTeam(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) {
		lua_pushinteger(L, TEAM_UNASSIGNED);
		return 1;
	}

	lua_pushinteger(L, pPlayer->GetTeamNumber());
	return 1;
}



int luaIsPlayerDead(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) 
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	else
	{
		bool dead = !pPlayer->IsAlive();

		lua_pushboolean(L, dead);
		return 1;
	}
}

int luaSwitchToWeapon(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;

	const char* luaWep = lua_tostring(L,2);

	if (!luaWep)
		return 0;

	char wep[255];
	Q_snprintf(wep, 255, "weapon_%s", luaWep);

	CBaseCombatWeapon *pWeapon = pPlayer->Weapon_OwnsThisType(wep);
	pPlayer->Weapon_Switch(pWeapon);

	return 0;
}

int luaGiveNamedWeapon(lua_State *L)
{

	//ZMSLuaGamePlay *g = GetGP();
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;
	
	const char* luaWep = lua_tostring(L,2);

	if (!luaWep)
		return 0;

	char wep[255];
	Q_snprintf(wep, 255, "weapon_%s", luaWep);

	// S:O -
	// TODO:
	//int ammoAmount = lua_tointeger(L,3);
	//if (ammoAmount > 0)
	//{
		//int id = g->AliastoWeaponID( wep );
		//const char* ammoId = GetAmmoForWeapon(id);

		//pPlayer->GiveAmmo( ammoAmount, ammoId );
	//}

	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pPlayer->GiveNamedItem(wep);

	if (lua_toboolean(L,4) == 1 && pWeapon)
	{
		pPlayer->RemoveAmmo( pWeapon->GetDefaultClip1(), pWeapon->m_iPrimaryAmmoType );
	}
	return 0;
}

int luaStripAllWeapons(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;

	if ( pPlayer->GetActiveWeapon() )
	{
		pPlayer->GetActiveWeapon()->Holster();
	}
	pPlayer->RemoveAllItems(false);
	
	return 0;
}
int luaIncrementPlayerLevel(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	CHL2MP_Player *p = dynamic_cast<CHL2MP_Player*>(pPlayer);
	int level = lua_tointeger(L,2);
	p->m_iLevel = p->m_iLevel + level;
	return 0;
}
int luaGetPlayerLevel(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	CHL2MP_Player *p = dynamic_cast<CHL2MP_Player*>(pPlayer);
	lua_pushinteger(L, p->GetLevel());
	return 1;
}
int luaAddPlayerDeaths(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	int cnt = lua_tointeger(L,2);
	if (!pPlayer)
		return 0;

	pPlayer->IncrementDeathCount(cnt);
	return 0;
}

int luaResetPlayerDeaths(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;
	else
		pPlayer->ResetDeathCount();
	return 0;
}

int luaPlayerName(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) {
		lua_pushnil(L);
		return 1;
	}

	lua_pushstring(L, pPlayer->GetPlayerName());
	return 1;
}

int luaGetPlayerDeaths(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) {
		lua_pushinteger(L, -1);
		return 1;
	}
	lua_pushinteger(L, pPlayer->DeathCount());
	return 1;
}

int luaPlayerSetMaxHealth(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if(!pPlayer)
		return 0;
	else
		pPlayer->SetMaxHealth(lua_tointeger(L,2));
	return 0;
}

int luaPlayerSetHealth(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;
	else
		pPlayer->SetHealth(lua_tointeger(L,2));
	return 0;
}
int luaPlayerMaxHealth(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) {
		lua_pushnil(L);
		return 1;
	}

	lua_pushinteger(L,pPlayer->GetMaxHealth());
	return 1;
}


int luaPlayerHealth(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) {
		lua_pushnil(L);
		return 1;
	}

	lua_pushinteger(L,pPlayer->GetHealth());
	return 1;
}

int luaGetPlayerScore(lua_State *L)
{
	CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer) {
		lua_pushnil(L);
		return 1;
	}

	lua_pushinteger(L, pPlayer->GetScore());
	return 1;
}

int luaAddToPlayerScore(lua_State *L)
{
	CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;

	int score = lua_tointeger(L,2);
	pPlayer->IncrementScore( score );

	return 0;
}

int luaSetPlayerTeam(lua_State *L)
{
	// Dynamic cast required to call top level team change functions
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );
	int team = lua_tointeger(L,2);
	if (!pPlayer)
		return 0;

	// Force the team change
	pPlayer->ChangeTeam( team);
	return 1;
}


int luaSetPlayerChar(lua_State *L)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );

	if (pPlayer) 
		pPlayer->SetModel(lua_tostring(L,2));

	return 0;
}

int luaRespawnPlayer(lua_State *L)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );

	if (pPlayer) 
		pPlayer->ForceRespawn();

	return 0;
}

int luaKillPlayer(lua_State *L)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );

	// Force the player to kill themselves
	if (pPlayer)
		pPlayer->CommitSuicide( false, true );

	return 0;
}


int luaPlayerAmmoCount(lua_State *L)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );

	// Force the player to kill themselves
	if (!pPlayer)
	{
		lua_pushinteger(L,0);
	}
	else
	{
		CBaseCombatWeapon* pWeap = pPlayer->GetActiveWeapon();

		if (!pWeap)
		{
			lua_pushinteger(L,0);
		}
		else
		{
			lua_pushinteger(L, pPlayer->GetAmmoCount( pWeap->GetPrimaryAmmoType()) );
		}
	}

	return 1;
}

int luaIsPlayerEnemy(lua_State *L)
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,1)) );
	CBasePlayer *pEnemy = ToBasePlayer( UTIL_PlayerByIndex(lua_tointeger(L,2)) );

	if (!pPlayer || !pEnemy)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L,  (GameRules()->PlayerRelationship(pPlayer, pEnemy) == GR_NOTTEAMMATE));

	return 1;
}

int luaDistanceBetween(lua_State *L)
{
	CBaseEntity *pEntOne = UTIL_EntityByIndex(lua_tointeger(L,1));
	CBaseEntity *pEntTwo = UTIL_EntityByIndex(lua_tointeger(L,2));

	int distance = 0;

	if (pEntOne && pEntTwo)
		distance = pEntOne->GetAbsOrigin().DistTo(pEntTwo->GetAbsOrigin());

	lua_pushinteger(L, distance);
	return 1;
};

int luaClientPrintPlayer(lua_State *L)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(lua_tointeger(L,1));
	if (!pPlayer)
		return 0;

	CRecipientFilter filter;
	filter.AddRecipient( pPlayer );

	int n = lua_gettop(L)-1;    /* number of arguments */
	switch(n)
	{
	case 2:
		UTIL_ClientPrintFilter( filter, lua_tointeger(L,2), lua_tostring(L,3));
		break;
	case 3:
		UTIL_ClientPrintFilter( filter, lua_tointeger(L,2), lua_tostring(L,3),lua_tostring(L,4));
		break;
	case 4:
		UTIL_ClientPrintFilter( filter, lua_tointeger(L,2), lua_tostring(L,3),lua_tostring(L,4),lua_tostring(L,5));
		break;
	case 5:
		UTIL_ClientPrintFilter( filter, lua_tointeger(L,2), lua_tostring(L,3),lua_tostring(L,4),lua_tostring(L,5),lua_tostring(L,6));
		break;
	case 6:
		UTIL_ClientPrintFilter( filter, lua_tointeger(L,2), lua_tostring(L,3),lua_tostring(L,4),lua_tostring(L,5),lua_tostring(L,6),lua_tostring(L,7));
		break;
	}
	return 0;
}

int luaEndRound(lua_State *L)
{
	CHL2MPRules *pRules = HL2MPRules();
	pRules->SetRoundState( RoundEnding, lua_tointeger(L,1));
	return 0;
}

int luaGetAlivePlayers(lua_State *L)
{
	CHL2MPRules *pRules = HL2MPRules();
	int alive = pRules->GetAlivePlayers(lua_tointeger(L,1));
	lua_pushinteger(L,alive);
	return 1;
}