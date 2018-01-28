#pragma once
#include "GameEventListener.h"
void CreateEventListeners();
void GameEvent_PlayerHurt(IGameEvent* gameEvent);
void GameEvent_BulletImpact(IGameEvent* gameEvent);
void GameEvent_PlayerDeath(IGameEvent* gameEvent);
void DecayPlayerIsShot();

extern char *useridstr;
extern char *attackerstr;
extern char *hitgroupstr;
extern char *useridstr2;
extern char *dmg_healthstr;

inline CBaseEntity* GetPlayerEntityFromEvent(IGameEvent* gameEvent)
{
	DecStr(useridstr, 6);
	int iUser = gameEvent->GetInt(useridstr);
	EncStr(useridstr, 6);
	int iUserEnt = Interfaces::Engine->GetPlayerForUserID(iUser);
	return iUserEnt ? Interfaces::ClientEntList->GetClientEntity(iUserEnt) : nullptr;
}

inline int GetAttackerUserID(IGameEvent* gameEvent)
{
	DecStr(attackerstr, 8);
	int iAttacker = gameEvent->GetInt(attackerstr);
	EncStr(attackerstr, 8);
	return iAttacker;
}

inline int GetHitgroupFromEvent(IGameEvent* gameEvent)
{
	DecStr(hitgroupstr, 8);
	int iHitGroup = gameEvent->GetInt(hitgroupstr);
	EncStr(hitgroupstr, 8);
	return iHitGroup;
}

inline int GetDamageFromEvent(IGameEvent* gameEvent)
{
	DecStr(dmg_healthstr, 10);
	int DamageGivenToTarget = gameEvent->GetInt(dmg_healthstr);
	EncStr(dmg_healthstr, 10);
	return DamageGivenToTarget;
}