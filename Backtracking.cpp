#include "player_lagcompensation.h"
#include "VMProtectDefs.h"
#include "ClientSideAnimationList.h"
#include "INetchannelInfo.h"
#include "Netchan.h"
#include "LocalPlayer.h"
#include "Logging.h"

//Backtrack the eye angles and various other netvars
void LagCompensation::FSN_BacktrackPlayer(CustomPlayer *const pCPlayer, CBaseEntity*const Entity)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("FSN_BacktrackPlayer\n");
#endif
	if (DisableAllChk.Checked)
		return;

#ifdef logicResolver
	pCPlayer->Personalize.JitterCounter++;

	if (WhaleDongTxt.iValue >= 1)//double handling but first statment wasn't working correctly
	{
		//THIS IS BROKEN ASF!!! LOWERBODY YAW ISN'T BEING GET SET, WAS TESTING AND SOMETIEMS IT SETS PLAYERS EYE ANGLES TO MORE THEN 35 FROM LBY! IT SHOULDN'T DO THIS
		//EVER!!! IF ANYONE CAN PLEASE LOOK AND HELP!
		StoredNetvars *CurrentNetVars = pCPlayer->GetCurrentRecord();
		if (CurrentNetVars)
		{
			auto LowerBodyRecords = &pCPlayer->Personalize.m_PlayerLowerBodyRecords;
			float flCurrentBodyYaw = CurrentNetVars->lowerbodyyaw;
			LogicResolver2_Log(Entity->GetEyeAngles(), QAngle(0, flCurrentBodyYaw, 0), pCPlayer->Personalize.lastLbyChcker != flCurrentBodyYaw, pCPlayer, LocalPlayer.Entity);
			pCPlayer->Personalize.lastLbyChcker = flCurrentBodyYaw;
		}
	}
#endif

	//Get the target tick to backtrack to
	StoredNetvars *backtracktick = pCPlayer->GetBestBacktrackTick();

	QAngle TargetEyeAngles; //Eye angles to backtrack to

	pCPlayer->ResolverFiredAtShootingAngles = false;

	if (backtracktick && !pCPlayer->IsBreakingLagCompensation)
	{
		pCPlayer->PersistentData.LastResolveModeUsed = Backtracked;
		//Get destination eye angles to backtrack to
		if (backtracktick == pCPlayer->LatestFiringTick)
		{
			pCPlayer->PersistentData.LastResolveModeUsed = BackTrackFire;
			pCPlayer->ResolverFiredAtShootingAngles = true;
			TargetEyeAngles = GetFiringTickAngles(pCPlayer, Entity, backtracktick);
		}
		else if (backtracktick == pCPlayer->LatestRealTick)
		{
			TargetEyeAngles = backtracktick->eyeangles;
			pCPlayer->PersistentData.LastResolveModeUsed = BackTrackReal;
		}
		else if (backtracktick == pCPlayer->LatestPitchUpTick && ValveResolverChk.Checked)
		{
			TargetEyeAngles = backtracktick->eyeangles;
			pCPlayer->PersistentData.LastResolveModeUsed = BackTrackUp;
		}
		else if (backtracktick == pCPlayer->LatestBloodTick)
		{
			TargetEyeAngles = backtracktick->eyeangles;
			pCPlayer->PersistentData.LastResolveModeUsed = BackTrackHit;
		}
		else
		{
			//Lower Body Yaw Tick
			pCPlayer->PersistentData.LastResolveModeUsed = BackTrackLby;
			TargetEyeAngles = QAngle(backtracktick->eyeangles.x, backtracktick->lowerbodyyaw, backtracktick->eyeangles.z);
		}

		//Enemy player eye angles should be in 0-360 format
		if (TargetEyeAngles.y < 0.0f)
			TargetEyeAngles.y += 360.0f;

		GetIdealPitch(TargetEyeAngles.x, pCPlayer->PersistentData.ShotsMissed, pCPlayer, Entity, LocalPlayer.Entity);

		FSN_RestoreNetvars(backtracktick, Entity, TargetEyeAngles, backtracktick->lowerbodyyaw);

		backtracktick->idealeyeangles = TargetEyeAngles;

		//Update animations and pose parameters only when update is received, since we are backtracking to this tick always
		//FIXME TODO: Update them anyway and then store in a separate buffer for use later but then use old record
		if (backtracktick->TickReceivedNetUpdate == Interfaces::Globals->tickcount)
		{
			//Update the origin if necessary
			StoredNetvars *previousvars = pCPlayer->GetPreviousRecord();
			if (!previousvars)
			{
				Entity->SetAbsOrigin(backtracktick->networkorigin);
			}
			else
			{
				if (Entity->GetOrigin() != backtracktick->networkorigin)
				{
					Entity->SetAbsOrigin(backtracktick->networkorigin);
				}
			}

			clientanimating_t *animating = nullptr;
			int animflags;

			//Make sure game is allowed to client side animate. Probably unnecessary
			for (unsigned int i = 0; i < g_ClientSideAnimationList->count; i++)
			{
				clientanimating_t *tanimating = (clientanimating_t*)g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
				CBaseEntity *pAnimEntity = (CBaseEntity*)tanimating->pAnimating;
				if (pAnimEntity == Entity)
				{
					animating = tanimating;
					animflags = tanimating->flags;
					tanimating->flags |= FCLIENTANIM_SEQUENCE_CYCLE;
					break;
				}
			}
			*Entity->EyeAngles() = TargetEyeAngles;

			//Update animations/poses
			Entity->UpdateClientSideAnimation();

			//UpdateClientSideAnimation updates the abs angles, so update our record immediately
			backtracktick->absangles = Entity->GetAbsAngles();

			//Restore anim flags
			if (animating)
				animating->flags = animflags;

			//Store the new animations
			Entity->CopyPoseParameters(backtracktick->flPoseParameter);
			Entity->CopyAnimLayers(backtracktick->AnimLayer);
		}
#if 0
		else if (Entity->GetVelocity().Length2D() <= 0.1f)
		{
			Entity->UpdateClientSideAnimation(); //this is shit, but the body_yaw pose parameter's fucked when backtracking so fuck it
			Entity->CopyPoseParameters(backtracktick->flPoseParameter);
			Entity->CopyAnimLayers(backtracktick->AnimLayer);
		}
#endif
	}
	else
	{
		pCPlayer->PersistentData.LastResolveModeUsed = NotBackTracked;

		//Player is not able to be backtracked
		//Use current netvars and try to extrapolate their real angles
		const auto &records = pCPlayer->PersistentData.m_PlayerRecords;
		StoredNetvars* currentvars = records.front();
		TargetEyeAngles = currentvars->eyeangles;

		auto stat1 = START_PROFILING("GetIdealPitch");
		GetIdealPitch(TargetEyeAngles.x, pCPlayer->PersistentData.JitterCounter, pCPlayer, Entity, LocalPlayer.Entity);
		END_PROFILING(stat1);

		auto stat = START_PROFILING("GetIdealYaw");
		GetIdealYaw(TargetEyeAngles.y, pCPlayer->PersistentData.correctedindex, pCPlayer, Entity, LocalPlayer.Entity);
		END_PROFILING(stat);

		if (pCPlayer->PersistentData.isCorrected && currentvars->velocity.Length() <= 0.1f && !pCPlayer->bWasHistoryBacktrackedWhenWeShotAtThisPlayer && !pCPlayer->bWasMovingWhenWeLastShotAtThem && Interfaces::Globals->curtime - pCPlayer->flLastTimeWeShotAtThisPlayer < 3.0f)
		{
			currentvars->idealeyeangles = pCPlayer->angIdealEyeAnglesWhenWeShotAtThisPlayer;
			TargetEyeAngles = pCPlayer->angIdealEyeAnglesWhenWeShotAtThisPlayer;
		}
		else
		{
			currentvars->idealeyeangles = TargetEyeAngles;
		}

		bool bForceAnimationUpdate = Entity->EyeAngles()->x != TargetEyeAngles.x || Entity->EyeAngles()->y != TargetEyeAngles.y;
		bool bReceivedUpdateThisTick = currentvars->TickReceivedNetUpdate == Interfaces::Globals->tickcount;
		bool bShouldResolve = pCPlayer->PersistentData.ShouldResolve();

		FSN_RestoreNetvars(currentvars, Entity, TargetEyeAngles, currentvars->lowerbodyyaw);

#if 0
		if (ShouldResolve && pCPlayer->TicksChoked > 0 && Entity->GetVelocity().Length() == 0.0f)
		{
			if (pCPlayer->Personalize.EstimateYawMode == ResolveYawModes::LinearFake)
			{
				//Correct the feet yaw for backtracking purposes
				CorrectFeetYaw(Entity, ClampYr(TargetEyeAngles.y), 50.0f);
			}
		}
#endif

		if (bShouldResolve && bReceivedUpdateThisTick)
		{
			//Force origin to fix phantom players
			StoredNetvars *previousvars = pCPlayer->GetPreviousRecord();
			if (!previousvars)
			{
				Entity->SetAbsOrigin(currentvars->networkorigin);
			}
			else
			{
				if (currentvars->networkorigin != previousvars->networkorigin || Entity->GetOrigin() != currentvars->networkorigin)
				{
					Entity->SetAbsOrigin(currentvars->networkorigin);
				}
			}
		}

		if (bShouldResolve && (bReceivedUpdateThisTick || bForceAnimationUpdate))
		{
			//Update animations and pose parameters
			clientanimating_t *animating = nullptr;
			int animflags;

			//Make sure game is allowed to client side animate. Probably unnecessary
			for (unsigned int i = 0; i < g_ClientSideAnimationList->count; i++)
			{
				clientanimating_t *tanimating = (clientanimating_t*)g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
				CBaseEntity *pAnimEntity = (CBaseEntity*)tanimating->pAnimating;
				if (pAnimEntity == Entity)
				{
					animating = tanimating;
					animflags = tanimating->flags;
					tanimating->flags |= FCLIENTANIM_SEQUENCE_CYCLE;
					break;
				}
			}

			//Make sure the game animates even if it's already animated this frame
			C_CSGOPlayerAnimState* animstate = Entity->GetPlayerAnimState();
			if (animstate)
			{
				if (animstate->m_flLastClientSideAnimationUpdateTime == Interfaces::Globals->curtime)
					animstate->m_flLastClientSideAnimationUpdateTime -= Interfaces::Globals->interval_per_tick;
				if (animstate->m_iLastClientSideAnimationUpdateFramecount == Interfaces::Globals->framecount)
					animstate->m_iLastClientSideAnimationUpdateFramecount--;
			}

			//Update animations/poses and set absangles
			Entity->UpdateClientSideAnimation();

			//Store the new absangles
			currentvars->absangles = Entity->GetAbsAngles();

			//Restore anim flags
			if (animating)
				animating->flags = animflags;
		}

		//Store the new animations
		Entity->CopyPoseParameters(currentvars->flPoseParameter);
		Entity->CopyAnimLayers(currentvars->AnimLayer);
	}
}

//Called from CreateMove to backtrack a player to the best possible tick, or compensates for fake lag
bool LagCompensation::CM_BacktrackPlayer(CustomPlayer *const pCPlayer, CBaseEntity*const pLocalEntity, CBaseEntity*const Entity, StoredNetvars *HistoryTick)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("CM_BacktrackPlayer\n");
#endif

	//Check to see if we have a valid tick to backtrack to
	StoredNetvars *BestBacktrackTick = HistoryTick ? HistoryTick : pCPlayer->GetBestBacktrackTick();

	if (BestBacktrackTick && BestBacktrackTick->IsValidForBacktracking && !pCPlayer->IsBreakingLagCompensation)
	{
		//Restore Animations and Pose Parameters
		CM_RestoreAnimations(BestBacktrackTick, Entity);

		//Restore the feet yaws
		Entity->SetGoalFeetYaw(BestBacktrackTick->GoalFeetYaw);
		Entity->SetCurrentFeetYaw(BestBacktrackTick->CurrentFeetYaw);

		//Restore Netvars
		CM_RestoreNetvars(BestBacktrackTick, Entity, &BestBacktrackTick->networkorigin, &BestBacktrackTick->absangles);

#if 0
		if (!BestBacktrackTick->ResolvedFromBloodSpawning && !BestBacktrackTick->IsRealTick && BestBacktrackTick->velocity.Length() == 0.0f && pCPlayer->Personalize.ShouldResolve())
		{
			int body_yaw_index;
			if (pCPlayer->bIsHistoryBacktracked)
			{
				body_yaw_index = BestBacktrackTick->corrected_body_yaw_index;
				pCPlayer->Personalize.correctedbodyyawindex = BestBacktrackTick->corrected_body_yaw_index;

			}
			else
			{
				body_yaw_index = pCPlayer->Personalize.correctedbodyyawindex;
			}

			switch (body_yaw_index)
			{
				//case 0 uses current pose parameter for the record, so is already set
			case 1:
				Entity->SetPoseParameterScaled(11, 0.0f);
				break;
			case 2:
				Entity->SetPoseParameterScaled(11, -0.5f);
				break;
			case 3:
				Entity->SetPoseParameterScaled(11, 0.5f);
			}
		}
#endif

		pCPlayer->iBacktrackTickCount = BestBacktrackTick->tickcount;
		pCPlayer->bBacktracked = true;
		pCPlayer->bNetvarsModified = true;
		//Let the rest of the cheat know which tick to get bones from
		pCPlayer->pBacktrackedTick = BestBacktrackTick;
		return true;
	}

	//Couldn't backtrack the player, so try to extrapolate positions
	pCPlayer->iBacktrackTickCount = 0;
	pCPlayer->bBacktracked = false;
	//Let the rest of the cheat know which tick to get bones from
	pCPlayer->pBacktrackedTick = pCPlayer->GetCurrentRecord();

	if (pCPlayer->PersistentData.ShouldResolve())
	{
		BacktrackToCurrentTickOrLagCompensate(pCPlayer);
	}
	return false;
}

//Backtrack to current tick and compensate for lag if necessary
void LagCompensation::BacktrackToCurrentTickOrLagCompensate(CustomPlayer*const pCPlayer)
{
	//Dylan's lag compensation attempt
	VMP_BEGINMUTILATION("CMPLAG")

	auto Records = &pCPlayer->PersistentData.m_PlayerRecords;
	int RecordCount = Records->size();
	CBaseEntity *Entity = pCPlayer->BaseEntity;
	if (RecordCount <= 1 || Entity->GetVelocity().Length() == 0.0f)
	{
		if (RecordCount)
		{
			auto CurrentTick = Records->front();

			CM_RestoreAnimations(CurrentTick, Entity);
			Entity->SetGoalFeetYaw(CurrentTick->GoalFeetYaw);
			Entity->SetCurrentFeetYaw(CurrentTick->CurrentFeetYaw);
			CM_RestoreNetvars(CurrentTick, Entity, &CurrentTick->networkorigin, &CurrentTick->absangles);

			pCPlayer->iBacktrackTickCount = CurrentTick->tickcount;

			pCPlayer->bNetvarsModified = true;
#if 0
		pCPlayer->bNetvarsModified = false;
		ResolveYawModes mode = pCPlayer->Personalize.EstimateYawMode;
		if (mode != ResolveYawModes::CloseFake && mode != AtTarget && mode != InverseAtTarget)
		{
			switch (pCPlayer->Personalize.correctedbodyyawindex)
			{
				//case 0 uses current pose parameter for the record, so is already set
			case 1:
				Entity->SetPoseParameterScaled(11, 0.0f);
				pCPlayer->bNetvarsModified = true;
				break;
			case 2:
				Entity->SetPoseParameterScaled(11, -1.0f);
				pCPlayer->bNetvarsModified = true;
				break;
			case 3:
				Entity->SetPoseParameterScaled(11, 1.0f);
				pCPlayer->bNetvarsModified = true;
				break;
			}
		}
#endif
		}
		else
		{
			pCPlayer->bNetvarsModified = false;
		}
		return;
	}

	auto CurrentTick = Records->front();
	auto PreviousTick = Records->at(1);

	CM_RestoreAnimations(CurrentTick, Entity);
	Entity->SetGoalFeetYaw(CurrentTick->GoalFeetYaw);
	Entity->SetCurrentFeetYaw(CurrentTick->CurrentFeetYaw);
	CM_RestoreNetvars(CurrentTick, Entity, &CurrentTick->networkorigin, &CurrentTick->absangles);

	pCPlayer->iBacktrackTickCount = CurrentTick->tickcount;

	pCPlayer->bNetvarsModified = true;

	if ((CurrentTick->networkorigin - PreviousTick->networkorigin).LengthSqr() > (64.0f * 64.0f)
		&& !PreviousTick->dormant)
	{
		PredictPlayerPosition(pCPlayer, CurrentTick, PreviousTick);
	}
	VMP_END
}

//Called from CreateMove to set up backtracking for all clients. If couldn't backtrack, then compensate for lag
void LagCompensation::CM_BacktrackPlayers()
{
	VMP_BEGINMUTILATION("CMBTP")
#ifdef PRINT_FUNCTION_NAMES
		LogMessage("CM_BacktrackPlayers\n");
#endif
	int MaxEntities = 64;// Interfaces::ClientEntList->GetHighestEntityIndex();
	int NumPlayers = GetClientCount();
	int CountedPlayers = 0;

	if (NumPlayers)
	{
		for (int i = 0; i <= MaxEntities; i++)
		{
			CBaseEntity* pPlayer = Interfaces::ClientEntList->GetClientEntity(i);

			if (pPlayer && pPlayer->IsPlayer())
			{
				CustomPlayer* pCPlayer = &AllPlayers[pPlayer->index];

				pCPlayer->bHasCachedBones = false;
				pCPlayer->bAlreadyRanHistoryScanThisTick = false; //Prevents infinite loops on history backtracking
				pCPlayer->bIsHistoryBacktracked = false;
				pCPlayer->bAlreadyCachedBonesThisTick = false;

#ifdef _DEBUG
				//THIS SHOULD NEVER HAPPEN!
				if (pCPlayer->bUsedCachedBonesThisTick)
					DebugBreak();
#endif

				pCPlayer->bUsedCachedBonesThisTick = false;

				if (pPlayer != LocalPlayer.Entity && pCPlayer->BaseEntity && !pPlayer->GetDormant() && pPlayer->GetAlive() && !pCPlayer->IsSpectating)
				{
					//Always store current netvars because animations and interp could have changed their properties
					//pCPlayer->LagCompensatedThisTick = false;
					pCPlayer->TempNetVars = StoredNetvars(pPlayer, pCPlayer, nullptr);
					StoredNetvars *BestBacktrackTick = pCPlayer->GetBestBacktrackTick();
					if (BestBacktrackTick && BestBacktrackTick == pCPlayer->LatestBloodTick)
						pCPlayer->TempNetVars.ResolvedFromBloodSpawning = true;
					CM_BacktrackPlayer(pCPlayer, LocalPlayer.Entity, pPlayer, nullptr);
				}
				else
				{
					pCPlayer->bNetvarsModified = false;
					pCPlayer->bBacktracked = false;
					pCPlayer->iBacktrackTickCount = 0;
				}

				if (++CountedPlayers == NumPlayers)
					break;
			}
		}
	}
	VMP_END
}

//Called from CreateMove to restore backtracked players back to current values
void LagCompensation::CM_RestorePlayers()
{
	VMP_BEGINMUTILATION("CMRESP")
#ifdef PRINT_FUNCTION_NAMES
		LogMessage("CM_RestorePlayers\n");
#endif

	int MaxEntities = 64;// Interfaces::ClientEntList->GetHighestEntityIndex();
	int NumPlayers = GetClientCount();
	int CountedPlayers = 0;

	if (NumPlayers)
	{
		for (int i = 0; i <= MaxEntities; i++)
		{
			CBaseEntity* pPlayer = Interfaces::ClientEntList->GetClientEntity(i);

			if (pPlayer && pPlayer->IsPlayer())
			{
				CustomPlayer* pCPlayer = &AllPlayers[pPlayer->index];
				pCPlayer->bAlreadyCachedBonesThisTick = false;
#if 0
				if (pCPlayer->bUsedCachedBonesThisTick)
				{
					CThreadFastMutex* pBoneSetupLock = pPlayer->GetBoneSetupLock();
					if (!pBoneSetupLock->TryLock())
					{
						pBoneSetupLock->Lock();
					}
					CBoneAccessor *accessor = pPlayer->GetBoneAccessor();
					CUtlVectorSimple *CachedBones = pPlayer->GetCachedBoneData();

					//Restore the original bone data and array that were set when using the cached bones
					//accessor->SetReadableBones(pCPlayer->iBackupGameReadableBones);
					//accessor->SetWritableBones(pCPlayer->iBackupGameWritableBones);
					CachedBones->count = pCPlayer->iBackupGameCachedBonesCount;
					memcpy(CachedBones->Base(), pCPlayer->BackupGameCachedBoneMatrixes, sizeof(matrix3x4_t) * MAXSTUDIOBONES);
					pCPlayer->bUsedCachedBonesThisTick = false;
					pBoneSetupLock->Unlock();
				}
#endif

				if (pPlayer != LocalPlayer.Entity)
				{
					if (pCPlayer->BaseEntity && pCPlayer->bNetvarsModified)
					{
						CM_RestoreAnimations(&pCPlayer->TempNetVars, pPlayer);
						pPlayer->SetGoalFeetYaw(pCPlayer->TempNetVars.GoalFeetYaw);
						pPlayer->SetCurrentFeetYaw(pCPlayer->TempNetVars.CurrentFeetYaw);
						CM_RestoreNetvars(&pCPlayer->TempNetVars, pPlayer, &pCPlayer->TempNetVars.origin, &pCPlayer->TempNetVars.absangles);
						//pPlayer->InvalidateBoneCache();
						//pPlayer->SetupBonesRebuiltSimple(nullptr, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, Interfaces::Globals->curtime);
						pCPlayer->iBacktrackTickCount = 0;
						pCPlayer->bNetvarsModified = false;
						//pCPlayer->LagCompensatedThisTick = false;
					}
				}

				if (++CountedPlayers == NumPlayers)
					break;
			}
		}
	}
	VMP_END
}