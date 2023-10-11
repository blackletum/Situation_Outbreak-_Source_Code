#include "cbase.h"
#include "lua/zms_lua.h"
#include "zms_luagameplay.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "hl2mp_gamerules.h"
#include "lua/ge_luamanager.h"
#include "tier0/memdbgon.h"

ZMSLuaGamePlay* g_LuaGamePlay = NULL;

// WARNING:
// Much of this needs to be updated before we decide to finally get LUA working...

ZMSLuaGamePlay::ZMSLuaGamePlay(void)
{
	g_LuaGamePlay = this;
	Msg("In Gameplay Const!\n");
	think = "Think";
}
ZMSLuaGamePlay::~ZMSLuaGamePlay(void)
{
}
void ZMSLuaGamePlay::Think()
{
	if(GetLuaHandle()->m_bLuaLoaded)
	{
		lua_getglobal(GetLua(), "Think");
		lua_pushinteger(GetLua(), 1);
		CallLUA(GetLua(),1,0,0, "Think");
	}
}
void ZMSLuaGamePlay::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	if(GetLuaHandle()->m_bLuaLoaded)
		{
          const char* szEventFunc = "OnPlayerKilled";
		  int pKiller = 0;
		  int pVictim2 = 0;
		  int wep = 0;
			if(info.GetAttacker()->IsNPC())
			{
				wep = 17;
				pKiller = info.GetAttacker()->entindex();
			}
		  if(info.GetAttacker() && pVictim && info.GetWeapon())
		  {
			  pVictim2 = pVictim->entindex();
			  pKiller = info.GetAttacker()->entindex();
			  wep = AliastoWeaponID(info.GetWeapon()->GetClassname());
		  }
		  lua_getglobal(GetLua(), szEventFunc);
          lua_pushnumber(GetLua(), pVictim2);
          lua_pushnumber(GetLua(), pKiller);
		  lua_pushnumber(GetLua(), wep);
          CallLUA(GetLua(), 3, 0, 0, szEventFunc);
		}
}

void ZMSLuaGamePlay::PlayerSay(CBasePlayer *pPlayer, const char *text)
{
		if(GetLuaHandle()->m_bLuaLoaded)
		{
          const char* szEventFunc = "PlayerSay";
          lua_getglobal(GetLua(), szEventFunc);
		  lua_pushnumber(GetLua(), pPlayer->entindex());
		  lua_pushstring(GetLua(), text);
		  CallLUA(GetLua(), 2, 0, 0, szEventFunc);
		}
}
void ZMSLuaGamePlay::PlayerConnect( CBasePlayer *pPlayer)
{
		if(GetLuaHandle()->m_bLuaLoaded)
		{
          const char* szEventFunc = "PlayerConnect";
          lua_getglobal(GetLua(), szEventFunc);
		  lua_pushnumber(GetLua(), pPlayer->entindex());
		  CallLUA(GetLua(), 1, 0, 0, szEventFunc);
		}

}
void ZMSLuaGamePlay::DropWeapon(CBaseCombatWeapon *pWep, CBaseCombatCharacter *pPlayer)
{
		if(GetLuaHandle()->m_bLuaLoaded)
		{
		  int ent = -1;
	      const char* szEventFunc = "DropWeapon";
		  int weapon = AliastoWeaponID(pWep->GetClassname());
		  if(pPlayer->IsPlayer())
		  {
			CBasePlayer *p = dynamic_cast<CBasePlayer*>(pPlayer);
			ent = p->entindex();
		  }
          lua_getglobal(GetLua(), szEventFunc);
		  lua_pushnumber(GetLua(), ent);
		  lua_pushnumber(GetLua(), weapon);
		  CallLUA(GetLua(), 2, 0, 0, szEventFunc);
		}
}
void ZMSLuaGamePlay::PlayerSpawn(CBasePlayer *pPlayer)
{
		if(GetLuaHandle()->m_bLuaLoaded)
		{
	      const char* szEventFunc = "PlayerSpawn";
          lua_getglobal(GetLua(), szEventFunc);
		  lua_pushnumber(GetLua(), pPlayer->entindex());
		  CallLUA(GetLua(), 1, 0, 0, szEventFunc);
		}
}
void ZMSLuaGamePlay::PlayerDisconnect(CBasePlayer *pPlayer)
{
		if(GetLuaHandle()->m_bLuaLoaded)
		{
	      const char* szEventFunc = "PlayerDisconnect";
          lua_getglobal(GetLua(), szEventFunc);
		  lua_pushnumber(GetLua(), pPlayer->entindex());
		  CallLUA(GetLua(), 1, 0, 0, szEventFunc);
		}
}

void ZMSLuaGamePlay::PlayerHurt(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	if( GetLuaHandle()->m_bLuaLoaded )
	{
		int attacker = -1;
		int weapon = 0;

		CBaseEntity *pAttacker = info.GetAttacker();

		if( pAttacker->IsPlayer() && info.GetWeapon() )
		{	
			attacker = info.GetAttacker()->entindex();
			Msg("%s\n", info.GetWeapon()->GetClassname());

			weapon = AliastoWeaponID(info.GetWeapon()->GetClassname());
		}
		else if( FClassnameIs(pAttacker, "npc_fastzombie") || FClassnameIs(pAttacker, "npc_reaper_nojump") || FClassnameIs(pAttacker, "npc_poisonzombie") || FClassnameIs(pAttacker, "npc_zombie") || FClassnameIs(pAttacker, "npc_sploderzombie") || FClassnameIs(pAttacker, "npc_cloud") || FClassnameIs(pAttacker, "npc_ghost") )
		{
			weapon = 8;
		}
		else if( pAttacker->IsNPC() )
		{
			weapon = 19;
		}
		else if(!pAttacker->IsNPC() || !pAttacker->IsPlayer())
		{
			weapon = 0;
		}

		const char* szEventFunc = "PlayerKilled";
		lua_getglobal(GetLua(), szEventFunc);
		lua_pushnumber(GetLua(), pVictim->entindex());
		lua_pushnumber(GetLua(), weapon);
		lua_pushnumber(GetLua(), attacker );
		CallLUA(GetLua(), 3, 0, 0, szEventFunc);
	}
}
inline ZMSLuaGamePlay* GetGP()
{
	return static_cast<ZMSLuaGamePlay*>(g_LuaGamePlay);
}


// Jordan - Lua needs #'s LOL
// 
int ZMSLuaGamePlay::AliastoWeaponID(const char *wep)
{
	if(FStrEq(wep,"weapon_deagle"))
		return 1;
	else if(FStrEq(wep,"weapon_incendiary"))
		return 2;
	else if(FStrEq(wep,"weapon_cleaver"))
		return 3;
	else if(FStrEq(wep,"weapon_brokenbottle"))
		return 4;
	else if(FStrEq(wep,"weapon_mac10"))
		return 5;
	else if(FStrEq(wep,"weapon_m4"))
		return 6;
	else if(FStrEq(wep,"weapon_rpg"))
		return 7;
	else if(FStrEq(wep,"weapon_zombie"))
		return 8;
	else if(FStrEq(wep,"weapon_doublebarrel"))
		return 9;
	else if(FStrEq(wep,"weapon_frag"))
		return 10;
	else if(FStrEq(wep,"weapon_ak47"))
		return 11;
	else if(FStrEq(wep,"weapon_9mm"))
		return 12;
	else if(FStrEq(wep,"weapon_mp5k"))
		return 13;
	else if(FStrEq(wep, "weapon_axe"))
		return 14;
	else if(FStrEq(wep, "weapon_sniper"))
		return 15;
	else if(FStrEq(wep, "weapon_brassknuckles"))
		return 16;
	else if(FStrEq(wep, "weapon_katana"))
		return 17;
	else if(FStrEq(wep, "weapon_chainsaw"))
		return 18;

	// WARNING: THIS LIST IS OUT OF DATE!
	// There are more weapons than just the ones listed above and below now.
	// kkthxbai >=D

// If Weapon Class != Any on list, return 0
return 0;
}

// Jordan - Reverse of the AliastoWeaponID
//
const char *ZMSLuaGamePlay::WeaponIDtoAlias(int wep)
{
	if(wep == 1)
		return "Deagle";
	else if(wep == 2)
		return "Incendiary Grenade";
	else if(wep == 3)
		return "Cleaver";
	else if(wep == 4)
		return "Broken Bottle";
	else if(wep == 5)
		return "Mac10";
	else if(wep == 6)
		return "M16";
	else if(wep == 7)
		return "RPG";
	else if(wep == 8)
		return "Zombie Claws";
	else if(wep == 9)
		return "Shotgun";
	else if(wep == 10)
		return "HE Grenade";
	else if(wep == 11)
		return "AK47";
	else if(wep == 12)
		return "9mm";
	else if(wep == 13)
		return "MP5K";
	else if(wep == 14)
		return "Axe";
	else if(wep == 15)
		return "Sniper Rifle";
	else if(wep == 16)
		return "Brass Knuckles";
	else if(wep == 17)
		return "Katana";
	else if(wep == 18)
		return "Chainsaw";
	else if(wep == 0)
		return "World";
// If Weapon Class # != Any on list, return NULL
return "NULL";
}
