
#ifndef MYLUAMANAGER_H

#define MYLUAMANAGER_H

#ifdef _WIN32

#pragma once

#endif

 

#include "ge_luamanager.h"

class MyLuaHandle : public LuaHandle
{
public:
  void Init( void );
  void Shutdown( void );
  void RegFunctions( void );
  void RegGlobals( void );
  void ReloadScript( const CCommand &args);
  MyLuaHandle();
  ~MyLuaHandle();

 //lua_State* GetLua(){return pL;}
bool m_bLuaLoaded;
private:

	//lua_State *pL;
 // bool m_bLuaLoaded; // is this the right place for this?

};

extern MyLuaHandle* GetLuaHandle();

 

#endif //MC_GE_LUAMANAGER_H