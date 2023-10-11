/*
include "cbase.h"
include "zms_lua.h"
include "KeyValues.h"
include "filesystem.h"
include "lua/ge_luamanager.h"
// memdbgon must be the last include file in a .cpp file!!!

include "tier0/memdbgon.h"

MyLuaHandle gLuaMng;
MyLuaHandle* ZMSLua()
{
	return &gLuaMng;
}
MyLuaHandle* g_LuaHandle = NULL;

MyLuaHandle* GetLuaHandle()
{
  return g_LuaHandle;
}

MyLuaHandle::~MyLuaHandle()
{

}

MyLuaHandle::MyLuaHandle() : LuaHandle()
{
	Msg("In Lua Constructor!\n");
	g_LuaHandle = this;
	Register();
}

 

void MyLuaHandle::Init()
{
  Msg("\n");
  Msg("\nIn MyLuaHandle Init\n");
  const char* luaFile = "lua\\myLuaFile.lua";
  //Load into buffer
  FileHandle_t f = filesystem->Open( luaFile, "rb", "MOD" );
  if (!f)
    return;
  // load file into a null-terminated buffer
  int fileSize = filesystem->Size(f);
  unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize( f, fileSize + 1 );
  char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer( f, bufSize );
  Assert(buffer);
  ((IFileSystem *)filesystem)->ReadEx( buffer, bufSize, fileSize, f ); // read into local buffer
  buffer[fileSize] = '\0'; // null terminate file as EOF
  filesystem->Close( f ); // close file after reading
  int error = luaL_loadbuffer( GELua()->GetLua(), buffer, fileSize, luaFile );
  if (error)
  {
    Warning("[LUA-ERR] %s\n", lua_tostring(GELua()->GetLua(), -1));
    lua_pop(GELua()->GetLua(), 1);  // pop error message from the stack
    Warning("[LUA-ERR] One or more errors occured while loading lua script!\n");
    return;
  }
  CallLUA(GELua()->GetLua(), 0, LUA_MULTRET, 0, luaFile );
  m_bLuaLoaded = true;
}

 

void MyLuaHandle::RegGlobals()
{

//  LG_DEFINE_INT("FOR_ALL_PLAYERS", -1);
  LG_DEFINE_INT("INVALID_ENTITY", -1);
  LG_DEFINE_INT("NULL", 0);
  //LG_DEFINE_INT("GE_MAX_HEALTH", MAX_HEALTH);
  //LG_DEFINE_INT("GE_MAX_ARMOR", MAX_ARMOR);
  LG_DEFINE_INT("MAX_PLAYERS", gpGlobals->maxClients);

  //Team Indices
  LG_DEFINE_INT("TEAM_NONE",TEAM_UNASSIGNED);
  LG_DEFINE_INT("TEAM_SPECTATOR",TEAM_SPECTATOR);
  LG_DEFINE_INT("TEAM_SURVIVORS",TEAM_SURVIVORS);
  LG_DEFINE_INT("TEAM_ZOMBIES",TEAM_ZOMBIES);
  LG_DEFINE_INT("TEAM_VIP", TEAM_VIP);

//ClientPrintAll Types
  LG_DEFINE_INT("HUD_PRINTNOTIFY",HUD_PRINTNOTIFY);
  LG_DEFINE_INT("HUD_PRINTCONSOLE",HUD_PRINTCONSOLE);
  LG_DEFINE_INT("HUD_PRINTTALK",HUD_PRINTTALK);
  LG_DEFINE_INT("HUD_PRINTCENTER",HUD_PRINTCENTER);
}

 

void MyLuaHandle::RegFunctions()
{
//////////////////////////////////////////////
// Player Functions
//////////////////////////////////////////////
	REG_FUNCTION_GLOBAL( IsValidPlayer );			// 1.5
	REG_FUNCTION_GLOBAL( IsPlayerDead );			// 1.5
	REG_FUNCTION_GLOBAL( GetPlayerTeam );			// 1.5
	REG_FUNCTION_GLOBAL( SetPlayerTeam );			// 1.5

	REG_FUNCTION_GLOBAL( GetPlayerScore );			// 1.5
	REG_FUNCTION_GLOBAL( AddToPlayerScore );		// 1.5
	REG_FUNCTION_GLOBAL( PlayerHealth );			// 1.5
	REG_FUNCTION_GLOBAL( PlayerSetHealth );			// 1.5
	REG_FUNCTION_GLOBAL( PlayerMaxHealth );			// 1.5
	REG_FUNCTION_GLOBAL( PlayerSetMaxHealth );		// 1.5

	REG_FUNCTION_GLOBAL( PlayerName );				// 1.5

	REG_FUNCTION_GLOBAL( StripAllWeapons );			// 1.5
	REG_FUNCTION_GLOBAL( SwitchToWeapon );			// 1.5
	REG_FUNCTION_GLOBAL( GiveNamedWeapon );			// 1.5


	REG_FUNCTION_GLOBAL( GetPlayerDeaths );			// 1.5
	REG_FUNCTION_GLOBAL( AddPlayerDeaths );			// 1.5
	REG_FUNCTION_GLOBAL( ResetPlayerDeaths );		// 1.5

	REG_FUNCTION_GLOBAL( SetPlayerChar );			// 1.5

	REG_FUNCTION_GLOBAL( RespawnPlayer );			// 1.5
	REG_FUNCTION_GLOBAL( KillPlayer );				// 1.5

	REG_FUNCTION_GLOBAL( PlayerAmmoCount );
	REG_FUNCTION_GLOBAL( IsPlayerEnemy );

	//////////////////////////////////////////////
	// Util Functions
	//////////////////////////////////////////////
	REG_FUNCTION_GLOBAL( Msg );						// 1.5
	REG_FUNCTION_GLOBAL( ConMsg );					// 1.5
	REG_FUNCTION_GLOBAL( ClientPrintAll );			// 1.5
	REG_FUNCTION_GLOBAL( GetTime );					// 1.5

	REG_FUNCTION_GLOBAL( ClientPrintPlayer );		// 1.5
	REG_FUNCTION_GLOBAL( WeaponPrintName );			// 1.5
	REG_FUNCTION_GLOBAL( WeaponName );				// 1.5
	REG_FUNCTION_GLOBAL( WeaponId );				// 1.5

	REG_FUNCTION_GLOBAL( CreateCVar );				// 1.5 !! NON FUNCTIONAL
	REG_FUNCTION_GLOBAL( LoadConfig );				// 1.5 !! NON FUNCTIONAL
	REG_FUNCTION_GLOBAL( GetCVar );					// 1.5 

	REG_FUNCTION_GLOBAL( DistanceBetween );

}


void MyLuaHandle::Shutdown()
{
}

*/


//hl2aitools

 

#include "cbase.h"

 

#include "zms_lua.h"

#include "KeyValues.h"
#include "filesystem.h"
#include "hl2mp_gamerules.h"
#include "zms_luagameplay.h"
// memdbgon must be the last include file in a .cpp file!!!

#include "tier0/memdbgon.h"

 

MyLuaHandle* g_LuaHandle = NULL;

MyLuaHandle* GetLuaHandle()

{

  return g_LuaHandle;

}

 

MyLuaHandle::MyLuaHandle() : LuaHandle()

{

  g_LuaHandle = this;

  Register();

}
MyLuaHandle::~MyLuaHandle() 
{
}

 

void MyLuaHandle::Init()
{
  //Msg("\nIn MyLuahandle Init\n");
  const char* luaFile = "lua\\myLuaFile.lua";
  //Load into buffer
  FileHandle_t f = filesystem->Open( luaFile, "rb", "MOD" );
  if (!f)
    return;
  // load file into a null-terminated buffer
  int fileSize = filesystem->Size(f);
  unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize( f, fileSize + 1 );
  char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer( f, bufSize );
  Assert(buffer);
  ((IFileSystem *)filesystem)->ReadEx( buffer, bufSize, fileSize, f ); // read into local buffer
  buffer[fileSize] = '\0'; // null terminate file as EOF
  filesystem->Close( f ); // close file after reading
  int error = luaL_loadbuffer( GetLua(), buffer, fileSize, luaFile );
  if (error)
  {
    Warning("[LUA-ERR] %s\n", lua_tostring(GetLua(), -1));
    lua_pop(GetLua(), 1);  // pop error message from the stack
    Warning("[LUA-ERR] One or more errors occured while loading lua script!\n");
    return;
  }
  CallLUA(GetLua(), 0, LUA_MULTRET, 0, luaFile );
  lua_getglobal(GetLua(), "LoadScript");
  m_bLuaLoaded = true;
//  ZMSLuaGamePlay *p = GetGP();
 // p->m_bLua = true;

}

 

void MyLuaHandle::RegGlobals()
{
//  LG_DEFINE_INT("FOR_ALL_PLAYERS", -1);
  LG_DEFINE_INT("INVALID_ENTITY", -1);
  LG_DEFINE_INT("NULL", 0);
  //LG_DEFINE_INT("GE_MAX_HEALTH", MAX_HEALTH);
  //LG_DEFINE_INT("GE_MAX_ARMOR", MAX_ARMOR);
  LG_DEFINE_INT("MAX_PLAYERS", gpGlobals->maxClients);

  //Team Indices
  LG_DEFINE_INT("TEAM_NONE",TEAM_UNASSIGNED);
  LG_DEFINE_INT("TEAM_SPECTATOR",TEAM_SPECTATOR);
  LG_DEFINE_INT("TEAM_SURVIVORS",TEAM_SURVIVORS);
  LG_DEFINE_INT("TEAM_MILITARY", TEAM_MILITARY);
  LG_DEFINE_INT("TEAM_ZOMBIES",TEAM_ZOMBIES);
  LG_DEFINE_INT("TEAM_VIP", TEAM_VIP);

//ClientPrintAll Types
  LG_DEFINE_INT("HUD_PRINTNOTIFY",HUD_PRINTNOTIFY);
  LG_DEFINE_INT("HUD_PRINTCONSOLE",HUD_PRINTCONSOLE);
  LG_DEFINE_INT("HUD_PRINTTALK",HUD_PRINTTALK);
  LG_DEFINE_INT("HUD_PRINTCENTER",HUD_PRINTCENTER);
}

 

void MyLuaHandle::RegFunctions()
{
//////////////////////////////////////////////
// Player Functions
//////////////////////////////////////////////
	REG_FUNCTION_GLOBAL( IsValidPlayer );			// 1.5
	REG_FUNCTION_GLOBAL( IsPlayerDead );			// 1.5
	REG_FUNCTION_GLOBAL( GetPlayerTeam );			// 1.5
	REG_FUNCTION_GLOBAL( SetPlayerTeam );			// 1.5

	REG_FUNCTION_GLOBAL( GetPlayerScore );			// 1.5
	REG_FUNCTION_GLOBAL( AddToPlayerScore );		// 1.5
	REG_FUNCTION_GLOBAL( PlayerHealth );			// 1.5
	REG_FUNCTION_GLOBAL( PlayerSetHealth );			// 1.5
	REG_FUNCTION_GLOBAL( PlayerMaxHealth );			// 1.5
	REG_FUNCTION_GLOBAL( PlayerSetMaxHealth );		// 1.5

	REG_FUNCTION_GLOBAL( PlayerName );				// 1.5

	REG_FUNCTION_GLOBAL( StripAllWeapons );			// 1.5
	REG_FUNCTION_GLOBAL( SwitchToWeapon );			// 1.5
	REG_FUNCTION_GLOBAL( GiveNamedWeapon );			// 1.5


	REG_FUNCTION_GLOBAL( GetPlayerDeaths );			// 1.5
	REG_FUNCTION_GLOBAL( AddPlayerDeaths );			// 1.5
	REG_FUNCTION_GLOBAL( ResetPlayerDeaths );		// 1.5

	REG_FUNCTION_GLOBAL( SetPlayerChar );			// 1.5

	REG_FUNCTION_GLOBAL( RespawnPlayer );			// 1.5
	REG_FUNCTION_GLOBAL( KillPlayer );				// 1.5

	REG_FUNCTION_GLOBAL( PlayerAmmoCount );
	REG_FUNCTION_GLOBAL( IsPlayerEnemy );
	REG_FUNCTION_GLOBAL( GetPlayerLevel );
	REG_FUNCTION_GLOBAL( IncrementPlayerLevel );

	REG_FUNCTION_GLOBAL( GetAlivePlayers);

	//////////////////////////////////////////////
	// Util Functions
	//////////////////////////////////////////////
	REG_FUNCTION_GLOBAL( Msg );						// 1.5
	REG_FUNCTION_GLOBAL( ConMsg );					// 1.5
	REG_FUNCTION_GLOBAL( ClientPrintAll );			// 1.5
	REG_FUNCTION_GLOBAL( GetTime );					// 1.5

	REG_FUNCTION_GLOBAL( ClientPrintPlayer );		// 1.5
	REG_FUNCTION_GLOBAL( WeaponPrintName );			// 1.5
	REG_FUNCTION_GLOBAL( WeaponName );				// 1.5
	REG_FUNCTION_GLOBAL( WeaponId );				// 1.5

	REG_FUNCTION_GLOBAL( CreateCVar );				// 1.5 !! NON FUNCTIONAL
	REG_FUNCTION_GLOBAL( LoadConfig );				// 1.5 !! NON FUNCTIONAL
	REG_FUNCTION_GLOBAL( GetCVar );					// 1.5 

	REG_FUNCTION_GLOBAL( DistanceBetween );
	REG_FUNCTION_GLOBAL(EndRound);
}
 
void MyLuaHandle::Shutdown()

{

}

void MyLuaHandle::ReloadScript( const CCommand &args)
{
	m_bLuaLoaded = true;
	Warning("Reloading Lua Script\n");
	const char* luaFile = args.ArgS();
	Msg(luaFile);
  //Load into buffer
  FileHandle_t f = filesystem->Open( luaFile, "rb", "MOD" );
  if (!f)
  {
	  Warning("Lua File not availible!\n");
	 return;
  }
  // load file into a null-terminated buffer
  int fileSize = filesystem->Size(f);
  unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize( f, fileSize + 1 );
  char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer( f, bufSize );
  Assert(buffer);
  ((IFileSystem *)filesystem)->ReadEx( buffer, bufSize, fileSize, f ); // read into local buffer
  buffer[fileSize] = '\0'; // null terminate file as EOF
  filesystem->Close( f ); // close file after reading
  int error = luaL_loadbuffer( GetLua(), buffer, fileSize, luaFile );
  if (error)
  {
    Warning("[LUA-ERR] %s\n", lua_tostring(GetLua(), -1));
    lua_pop(GetLua(), 1);  // pop error message from the stack
    Warning("[LUA-ERR] One or more errors occured while loading lua script!\n");
    return;
  }
  CallLUA(GetLua(), 0, LUA_MULTRET, 0, luaFile );
  lua_pushstring(GetLua(), "Loaded Successfully!");
 lua_getglobal(GetLua(), "LoadScript");
  m_bLuaLoaded = true;
}
static void ReloadScript(const CCommand &args)
{
	GetLuaHandle()->ReloadScript(args);
}
//static ConCommand luar("lua_load", ReloadScript, "Reload a lua script");


 