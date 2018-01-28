#include "VTHook.h"
#include "SpawnBlood.h"
#include "Interfaces.h"
#include "player_lagcompensation.h"
#include "LocalPlayer.h"
#include "VMProtectDefs.h"

void OnSpawnBlood(C_TEEffectDispatch* thisptr, DataUpdateType_t updateType)
{
	auto stat = START_PROFILING("OnSpawnBlood");
	CBaseEntity* pEntity = thisptr->m_EffectData.m_hEntity != -1 ? Interfaces::ClientEntList->GetClientEntityFromHandle(thisptr->m_EffectData.m_hEntity) : nullptr;
	if (pEntity && pEntity->IsPlayer() && pEntity != LocalPlayer.Entity)
	{
#ifdef _DEBUG
		Vector* origin = &thisptr->m_EffectData.m_vOrigin;
		AllocateConsole();
		wchar_t PlayerName[32];
		GetPlayerName(GetRadar(), pEntity->index, PlayerName);
		printf("Spawn Blood:\nPlayer: %S\nBlood Origin: %.2f, %.2f, %.2f\nSimulation Time: %f\nTickcount: %i\n\n", PlayerName, origin->x, origin->y, origin->z, pEntity->GetSimulationTime(), Interfaces::Globals->tickcount);
#endif
		gLagCompensation.OnPlayerBloodSpawned(pEntity, &thisptr->m_EffectData.m_vOrigin, &thisptr->m_EffectData.m_vNormal);
		Vector impactorigin = pEntity->GetOrigin() + thisptr->m_EffectData.m_vOrigin;
		Vector impactorigin2 = pEntity->GetOrigin() + thisptr->m_EffectData.m_vStart;
		Interfaces::DebugOverlay->AddBoxOverlay(impactorigin, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 0, 255, 0, 255, 1.0f);
		Interfaces::DebugOverlay->AddBoxOverlay(impactorigin2, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 255, 255, 0, 255, 1.0f);
	}
	oTE_EffectDispatch_PostDataUpdate(thisptr, updateType);
	END_PROFILING(stat);
}

void OnSpawnImpact(C_TEEffectDispatch* thisptr, DataUpdateType_t updateType)
{
	auto stat = START_PROFILING("OnSpawnImpact");
	CBaseEntity* pEntity = thisptr->m_EffectData.m_hEntity != -1 ? Interfaces::ClientEntList->GetClientEntityFromHandle(thisptr->m_EffectData.m_hEntity) : nullptr;
	if (pEntity && pEntity->IsPlayer() && pEntity != LocalPlayer.Entity)
	{
#ifdef _DEBUG
		//Vector* origin = &(pEntity->GetOrigin() + thisptr->m_EffectData.m_vOrigin);
		//AllocateConsole();
		//wchar_t PlayerName[32];
		//GetPlayerName(GetRadar(), pEntity->index, PlayerName);
		//printf("Spawn Impact:\nPlayer: %S\nBlood Origin: %.2f, %.2f, %.2f\nSimulation Time: %f\nTickcount: %i\n\n", PlayerName, origin->x, origin->y, origin->z, pEntity->GetSimulationTime(), Interfaces::Globals->tickcount);
#endif
		gLagCompensation.OnPlayerImpactSpawned(pEntity, thisptr);
	}
	oTE_EffectDispatch_PostDataUpdate(thisptr, updateType);
	END_PROFILING(stat);
}