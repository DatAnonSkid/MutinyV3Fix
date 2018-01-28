#include "player_lagcompensation.h"
#include "VMProtectDefs.h"
#include "Logging.h"

//Invalidates a specific tick range
void LagCompensation::InvalidateTicks(CustomPlayer* pCPlayer, int from, int to)
{
	std::deque<StoredNetvars*> *Records = &pCPlayer->PersistentData.m_PlayerRecords;
	for (from; from <= to; from++)
	{
		StoredNetvars *record = Records->at(from);
		//Don't bother continuing the loop if we already invalidated the rest of the ticks
		if (!record->IsValidForBacktracking)
			break;
		record->IsValidForBacktracking = false;
		ClearImportantTicks(pCPlayer, record);
	}
}

//Removes excess ticks and invalidates ticks that are too old or invalid ticks
void LagCompensation::ValidateTickRecords(CustomPlayer *const pCPlayer, CBaseEntity*const Entity)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("ValidateTickRecords\n");
#endif

	//First, validate all player tick records
	const auto Records = &pCPlayer->PersistentData.m_PlayerRecords;

	//Remove excess records
	if (Records->size() > 72)
	{
		StoredNetvars* oldestrecord = Records->back();
		//If the oldest tick is an important update, then get rid of it
		ClearImportantTicks(pCPlayer, oldestrecord);
		//Free the memory used by the oldest tick record
		delete oldestrecord;
		//Delete the oldest tick record
		Records->pop_back();
	}

	const int maxindex = Records->size() - 1;
	//Note: This is < maxindex because we handle two indexes in one iteration
	for (int index = 0; index < maxindex; index++)
	{
		StoredNetvars *thisrecord = Records->at(index);
		//If this record is already invalidated, then just leave the loop
		if (!thisrecord->IsValidForBacktracking)
			break;

		//Check to see if this tick is still new enough to backtrack to
		if (!bTickIsValid(thisrecord->tickcount))
		{
			//Invalidate this and every tick older
			InvalidateTicks(pCPlayer, index, maxindex);
			break;
		}

		//Don't attempt to check a previous record if there's only one
		if (maxindex > 0)
		{
			int nextrecord = index + 1; //Note: No overflow check needed
			StoredNetvars *prevrecord = Records->at(nextrecord);
			//If this record is already invalidated, then just leave the loop
			if (!prevrecord->IsValidForBacktracking)
				break;

			//Check to see if the previous tick is still new enough to backtrack to
			if (!bTickIsValid(prevrecord->tickcount))
			{
				//Invalidate the previous record and every tick older than it
				InvalidateTicks(pCPlayer, nextrecord, maxindex);
				break;
			}

			//Check to see if the player teleported too far from between this record and the one before it
			float OriginDelta = (thisrecord->origin - prevrecord->origin).LengthSqr();

			if (OriginDelta > (64.0f * 64.0f))
			{
				//The player teleported too far. Invalidate this and any prior ticks
				InvalidateTicks(pCPlayer, nextrecord, maxindex);
				break;
			}
		}
	}

	//Then, remove excess lower body yaw records
	const auto LowerBodyRecords = &pCPlayer->PersistentData.m_PlayerLowerBodyRecords;

	if (LowerBodyRecords->size() > 32)
	{
		//Retrieve the oldest tick record
		StoredLowerBodyYaw* oldestrecord = LowerBodyRecords->back();
		//Free the memory used by the oldest tick record
		delete oldestrecord;
		//Delete the oldest tick record
		LowerBodyRecords->pop_back();
	}

	//Finally, invalidate the blood record if it's too old
	if (pCPlayer->LatestBloodTick && !bTickIsValid(pCPlayer->LatestBloodTick->tickcount))
	{
		delete pCPlayer->LatestBloodTick;
		pCPlayer->LatestBloodTick = nullptr;
	}
}

//Clears all lag compensation records for this player
void LagCompensation::ClearLagCompensationRecords(CustomPlayer*const pCPlayer)
{
	Personalize* pPersistentData = &pCPlayer->PersistentData;
	if (!pPersistentData->m_PlayerRecords.empty())
	{
		//Free memory used by records
		for (auto record : pPersistentData->m_PlayerRecords)
			delete record;
		pPersistentData->ShotsMissed = 0;
		pPersistentData->m_PlayerRecords.clear();
		pCPlayer->LowerBodyCanBeDeltaed = false;
		pCPlayer->LatestFiringTick = nullptr;
		pCPlayer->LatestLowerBodyUpdate = nullptr;
		pCPlayer->LatestPitchUpTick = nullptr;
		pCPlayer->LatestRealTick = nullptr;
	}

	if (!pPersistentData->m_PlayerLowerBodyRecords.empty())
	{
		//Free memory used by records
		for (auto record : pPersistentData->m_PlayerLowerBodyRecords)
			delete record;
		pPersistentData->m_PlayerLowerBodyRecords.clear();
	}

	if (pCPlayer->LatestBloodTick)
	{
		delete pCPlayer->LatestBloodTick;
		pCPlayer->LatestBloodTick = nullptr;
	}

	ClearPredictedAngles(pCPlayer);
}

//Marks all lag compensation records as dormant so lag compensation doesn't think we're teleporting
void LagCompensation::MarkLagCompensationRecordsDormant(CustomPlayer*const pCPlayer)
{
	Personalize* pPersistentData = &pCPlayer->PersistentData;
	if (!pPersistentData->m_PlayerRecords.empty())
	{
		for (auto record : pPersistentData->m_PlayerRecords)
		{
			//Don't continue through if we already marked records as dormant
			if (record->dormant)
				break;

			record->dormant = true;
			record->IsValidForBacktracking = false;
		}
	}
}

//Clears all blood records for this player
void LagCompensation::ClearBloodRecords(CustomPlayer*const pCPlayer)
{
	Personalize* pPersistentData = &pCPlayer->PersistentData;
	if (!pPersistentData->m_BloodRecords.empty())
	{
		//Free memory used by records
		for (auto record : pPersistentData->m_BloodRecords)
			delete record;
		pPersistentData->m_BloodRecords.clear();
	}
}

//If the important tick matches the record, delete it
void LagCompensation::ClearImportantTicks(CustomPlayer *const pCPlayer, StoredNetvars *const record)
{
	if (record == pCPlayer->LatestFiringTick)
		pCPlayer->LatestFiringTick = nullptr;
	if (record == pCPlayer->LatestLowerBodyUpdate)
		pCPlayer->LatestLowerBodyUpdate = nullptr;
	if (record == pCPlayer->LatestPitchUpTick)
		pCPlayer->LatestPitchUpTick = nullptr;
	if (record == pCPlayer->LatestRealTick)
		pCPlayer->LatestRealTick = nullptr;
	if (record == pCPlayer->LatestReloadingTick)
		pCPlayer->LatestReloadingTick = nullptr;
	if (record == pCPlayer->LatestBloodTick)
		pCPlayer->LatestBloodTick = nullptr;
}

//Mark predicted angles as invalid
void LagCompensation::ClearPredictedAngles(CustomPlayer *const pCPlayer)
{
	pCPlayer->PredictedAverageFakeAngle = -999.9f;
	pCPlayer->PredictedRandomFakeAngle = -999.9f;
	pCPlayer->PredictedLinearFakeAngle = -999.9f;
	pCPlayer->PredictedStaticFakeAngle = -999.9f;
	pCPlayer->LowerBodyCanBeDeltaed = false;
}

void LagCompensation::ValidateNetVarsWithProxy(CustomPlayer *const pCPlayer)
{
	CBaseEntity *Entity = pCPlayer->BaseEntity;
	float flSimulationTime = Entity->GetSimulationTime();

	StoredNetvars *ProxyVars = &pCPlayer->CurrentNetvarsFromProxy;
	if (ProxyVars->simulationtime < flSimulationTime)
	{
		ProxyVars->simulationtime = flSimulationTime;
	}
	else if (ProxyVars->simulationtime > Entity->GetSimulationTime())
	{
		Entity->SetSimulationTime(ProxyVars->simulationtime);
	}
	if (ProxyVars->networkorigin == vecZero && Entity->GetNetworkOrigin() != vecZero)
	{
		ProxyVars->networkorigin = Entity->GetNetworkOrigin();
	}
	else if (Entity->GetNetworkOrigin() == vecZero && ProxyVars->networkorigin != vecZero)
	{
		Entity->SetNetworkOrigin(ProxyVars->networkorigin);
	}
	if (ProxyVars->origin == vecZero && Entity->GetOrigin() != vecZero)
	{
		ProxyVars->origin = Entity->GetOrigin();
	}
	else if (Entity->GetOrigin() == vecZero && ProxyVars->origin != vecZero)
	{
		Entity->SetOrigin(ProxyVars->origin);
	}
	if (ProxyVars->absorigin == vecZero && *Entity->GetAbsOrigin() != vecZero)
	{
		ProxyVars->absorigin = *Entity->GetAbsOrigin();
	}
	if (ProxyVars->eyeangles == angZero && Entity->GetEyeAngles() != angZero)
	{
		ProxyVars->eyeangles = Entity->GetEyeAngles();
	}
	else if (Entity->GetEyeAngles() == angZero && ProxyVars->eyeangles != angZero)
	{
		Entity->GetEyeAngles() = angZero;
	}
	if (ProxyVars->velocity == vecZero && Entity->GetVelocity() != vecZero)
	{
		ProxyVars->velocity = Entity->GetVelocity();
	}
}