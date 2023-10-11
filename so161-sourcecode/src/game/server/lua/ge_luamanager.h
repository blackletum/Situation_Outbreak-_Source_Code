///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_luamanager.h
//   Description :
//      [TODO: Write the purpose of ge_luamanager.h.]
//
//   Created On: 3/5/2009 4:58:58 PM
//   Created By:  <mailto:admin[at]lodle[dot]net>
////////////////////////////////////////////////////////////////////////////
 
#ifndef MC_GE_LUAMANAGER_H
#define MC_GE_LUAMANAGER_H
#ifdef _WIN32
#pragma once
#endif
 
extern "C"
{
   #include <lua.h>
   #include <lauxlib.h>
   #include <lualib.h>
}
 
#include <vector>
 
inline void CallLUA(lua_State *L, int iV1, int iV2, int iV3, const char* pszFunc = "")
{
	// iV1 = # Arguments pushed onto the stack, DO NOT INCLUDE THE FUNCTION!
	// iV2 = # returns to push onto the stack
	// iV3 = Where to store the error
	if (lua_pcall(L, iV1, iV2, iV3) != 0) 
	{
        Warning("[LUA-ERR] Error running function \"%s\": %s\n", pszFunc, lua_tostring(L, -1));
		lua_pop(L, 1);
	}
};
 
#define REG_FUNCTION_GLOBAL( name )			\
	extern int lua##name(lua_State *L);	\
	lua_register(GetLua(), # name , lua##name );
 
#define REG_FUNCTION( name )				\
	extern int lua##name(lua_State *L);	\
	lua_register(GetLua(), # name , lua##name );
 
#define LG_DEFINE_INT(_name,_value) \
	lua_pushinteger(GetLua(),_value); lua_setglobal(GetLua(), _name);
#define LG_DEFINE_STRING(_name,_value) \
	lua_pushstring(GetLua(),_value); lua_setglobal(GetLua(), _name);
#define LG_DEFINE_BOOL(_name,_value) \
	lua_pushboolean(GetLua(),_value); lua_setglobal(GetLua(), _name);
 
#define LG_DEFINE_INT_GLOBAL(_name,_value) \
	lua_pushinteger(L,_value); lua_setglobal(L, _name);
#define LG_DEFINE_STRING_GLOBAL(_name,_value) \
	lua_pushstring(L,_value); lua_setglobal(L, _name);
#define LG_DEFINE_BOOL_GLOBAL(_name,_value) \
	lua_pushboolean(L,_value); lua_setglobal(L, _name);
 
#define MAX_EVENT_STRING 255
 
 
class LuaHandle
{
public:
	LuaHandle();
	~LuaHandle();
 
	virtual void Init()=0;
	virtual void Shutdown()=0;
 
	virtual void RegFunctions()=0;
	virtual void RegGlobals()=0;
 
	void InitDll();
	void ShutdownDll();
 
	lua_State* GetLua(){return pL;}
 
	//this needs to be called in the child class constructor
	void Register();

private:
	lua_State *pL;
};
 
 extern LuaHandle* GetNLuaHandle();
class CGELUAManager
{
public:
	CGELUAManager();
	~CGELUAManager();
 
	virtual void InitDll();
	virtual void ShutdownDll();
 
	void RegisterLuaHandle(LuaHandle* handle);
	void DeRegisterLuaHandle(LuaHandle* handle);
	//lua_State* GetLua(){return pL;}
protected:
	void InitLUA();
	void CloseLUA();
 
private:
	std::vector<LuaHandle*> m_vHandles;
	bool m_bInit;
	lua_State *pL;
};
 
extern CGELUAManager* GELua();
 
#endif //MC_GE_LUAMANAGER_H