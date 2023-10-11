
#ifndef MYLUAGAMEPLAY_H
#define MYLUAGAMEPLAY_H

#ifdef _WIN32

#pragma once

#endif

#include "lua/zms_lua.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "hl2mp_gamerules.h"
#include "lua/ge_luamanager.h"
#include "tier0/memdbgon.h"


class ZMSLuaGamePlay: public MyLuaHandle
{
public:

	ZMSLuaGamePlay(void );
	~ZMSLuaGamePlay(void );
  void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info);
  void PlayerConnect( CBasePlayer *pPlayer);
  void PlayerSpawn(CBasePlayer *pPlayer);
  void PlayerSay(CBasePlayer *pPlayer, const char *text);
  void DropWeapon(CBaseCombatWeapon *pWep, CBaseCombatCharacter *pPlayer);
  void PlayerDisconnect(CBasePlayer *pPlayer);
  void PlayerHurt(CBasePlayer *pVictim, const CTakeDamageInfo &info);
    void GetGP(void);
  int AliastoWeaponID(const char *wep);
  const char *WeaponIDtoAlias(int wep);
    //const char *GetGameDescription( void );
	void Think(void);
  bool LuaOn(bool IsOn);
bool m_bLuaLoaded;
const char* think;
float m_fThinkTime;
private:

};
extern ZMSLuaGamePlay* GetGP();

#endif