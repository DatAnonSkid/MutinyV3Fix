#include "player_lagcompensation.h"
#include "LocalPlayer.h"
#include "VMProtectDefs.h"
#include "ClientSideAnimationList.h"
#include "INetchannelInfo.h"
#include "Triggerbot.h"
#include "HitChance.h"
#include "ThirdPerson.h"
#include "Logging.h"

//Update was received from the server, so update netvars
void LagCompensation::NetUpdatePlayer(CustomPlayer *const pCPlayer, CBaseEntity*const Entity)
{
	VMP_BEGINMUTILATION("NUP")
#ifdef PRINT_FUNCTION_NAMES
		LogMessage("NetUpdatePlayer\n");
#endif
	const float flNewSimulationTime = pCPlayer->CurrentNetvarsFromProxy.simulationtime;

	//See if the simulation time is valid yet or if the entity is not valid
	if ((flNewSimulationTime == 0.0f || !Entity->IsPlayer() || Entity->GetDormant() || !Entity->GetAlive()) && Entity->index <= MAX_PLAYERS)
	{
		MarkLagCompensationRecordsDormant(pCPlayer);
		ClearPredictedAngles(pCPlayer);
		pCPlayer->PersistentData.correctedfakewalkindex = 0;
		return;
	}

	//Get address of player records
	auto Records = &pCPlayer->PersistentData.m_PlayerRecords;

	//Store new tick record if simulation time changed or there are no records
	if (Records->empty())
	{
		ClearPredictedAngles(pCPlayer);
		NewTickRecordReceived(Entity, pCPlayer, flNewSimulationTime);
	}
	else
	{
		StoredNetvars *CurrentRecords = Records->front();
		//Also check origin for sanity

		if (flNewSimulationTime > CurrentRecords->simulationtime || Entity->GetNetworkOrigin() != CurrentRecords->networkorigin)
		{
#if 0
#if defined(_DEBUG) || defined(PRINT_FUNCTION_NAMES)
			float flDeltaTime = flNewSimulationTime - CurrentRecords->simulationtime;
			if (!CurrentRecords->dormant && fabsf(flDeltaTime) >= 1.0f)
			{
				AllocateConsole();
				if (flDeltaTime > 0.0f)
					printf("WARNING: Record for player %i Delta Time == %f\n", pCPlayer->Index, flDeltaTime);
				else
				{
					//Records->pop_front();
					printf("WARNING: Record for player %i Delta Time == %f (Is Bot?)\n", pCPlayer->Index, flDeltaTime);
				}
			}
			//printf("Recorded Simulation Time: %f\n", flNewSimulationTime);
#endif
#endif
			NewTickRecordReceived(Entity, pCPlayer, flNewSimulationTime);
		}
	}

	//Validate all the tick records. Remove excess ticks, invalidate any that are too old or those in which the player teleported too far
	ValidateTickRecords(pCPlayer, Entity);

	//Restore player netvars to the best possible tick
	FSN_BacktrackPlayer(pCPlayer, Entity);
	VMP_END
}

//Called by NetUpdatePlayer
void LagCompensation::NewTickRecordReceived(CBaseEntity* Entity, CustomPlayer*const pCPlayer, float flNewSimulationTime)
{
	auto Records = &pCPlayer->PersistentData.m_PlayerRecords;

	//Check to see if the current previous record is still valid
	bool bPreviousRecordIsInvalid = Records->empty() || Records->front()->dormant;

	//Store the amount of ticks this player choked
	pCPlayer->TicksChoked = bPreviousRecordIsInvalid ? 0 : TIME_TO_TICKS(flNewSimulationTime - Records->front()->simulationtime) - 1;

	//Update velocity
	float flTimeDelta = flNewSimulationTime - Entity->GetOldSimulationTime();
	bool bForceEFNoInterp = Entity->IsParentChanging();

	if (flTimeDelta != 0.0f && !((Entity->GetEffects() & EF_NOINTERP) || bForceEFNoInterp))
	{
		Vector newVelo = (Entity->GetNetworkOrigin() - Entity->GetOldOrigin()) / flTimeDelta;
		// This code used to call SetAbsVelocity, which is a no-no since we are in networking and if
		//  in hieararchy, the parent velocity might not be set up yet.
		// On top of that GetNetworkOrigin and GetOldOrigin are local coordinates
		// So we'll just set the local vel and avoid an Assert here
		Entity->SetLocalVelocity(newVelo);
	}

	//Validate to make sure proxy is valid..
	ValidateNetVarsWithProxy(pCPlayer);

	//Allocate a new tick record
	StoredNetvars *newtick = new StoredNetvars(Entity, pCPlayer, &pCPlayer->CurrentNetvarsFromProxy);

	//Get the previous tick's fire at pelvis value so that we can still shoot the heads if we successfully resolved the player
	bool PreviousFireAtPelvis = bPreviousRecordIsInvalid ? false : Records->front()->FireAtPelvis;

	//Store the new tick record
	Records->push_front(newtick);

	//See if this tick is special (player fired, lower body updated, pitch is up, etc)
	EvaluateNewTickRecord(Entity, pCPlayer, newtick, PreviousFireAtPelvis);

#ifdef predictTicks
	//Lets log choking information for fake lag fix
	int RecordCount = Records->size();
	if (RecordCount >= 1)
	{
		StoredNetvars *CurrentNetvars = Records->front();
		recordChokedTicks(pCPlayer, CurrentNetvars->tickschoked);
	}
#endif
}

//Process the new incoming tick to see if it is special
void LagCompensation::EvaluateNewTickRecord(CBaseEntity*const Entity, CustomPlayer*const pCPlayer, StoredNetvars*const record, bool PreviousFireAtPelvis)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("EvaluateNewTickRecord\n");
#endif
	StoredNetvars *PreviousNetvars = pCPlayer->GetPreviousRecord();
	if (PreviousNetvars)
	{
		if (PreviousNetvars->velocity.Length2D() == 0.0f && record->velocity.Length2D() != 0.0f)
		{
			VectorAngles(record->velocity, pCPlayer->DirectionWhenFirstStartedMoving);
			pCPlayer->PersistentData.correctedfakewalkindex = 0;
			pCPlayer->flTimeWhenFirstStartedMoving = Interfaces::Globals->curtime;
		}
		pCPlayer->IsBreakingLagCompensation = !PreviousNetvars->dormant && (record->networkorigin - PreviousNetvars->networkorigin).LengthSqr() > (64.0f * 64.0f);
	}
	else
	{
		pCPlayer->IsBreakingLagCompensation = false;
		pCPlayer->PersistentData.correctedfakewalkindex = 0;
	}

	if (!pCPlayer->PersistentData.ShouldResolve())
	{
		pCPlayer->LatestRealTick = record;
		record->IsRealTick = true;
		record->FireAtPelvis = false;
		pCPlayer->PersistentData.ShotsMissed = 0;
		pCPlayer->PersistentData.correctednotfakeindex = 0;
		pCPlayer->PersistentData.correctedbodyyawindex = 0;
		return;
	}

	bool IsBacktrackable = false;

#ifdef ANIMATIONS_DEBUG
	OutputAnimations(pCPlayer, Entity);
#endif

	//Check to see if the player fired this tick
	CBaseCombatWeapon *pWeapon = Entity->GetWeapon();
	if (pWeapon)
	{
		//Only count a tick as a fire bullet tick if we have stored currentweapon and the weapon has not changed, and the bullet count has changed

		if (!pWeapon->IsGun())
		{
			pCPlayer->BulletsLeft = 0;
		}
		else
		{
			int BulletsLeft = pWeapon->GetClipOne();
			if (pCPlayer->CurrentWeapon == pWeapon && BulletsLeft != pCPlayer->BulletsLeft)
			//if (pCPlayer->bReceivedMuzzleFlash && pCPlayer->CurrentWeapon == pWeapon)
			{
				//Make this record known as a firing update
				record->FiredBullet = true;

				//StoredNetvars *varsbeforeshot = pCPlayer->GetPreviousRecord();

				//record->tickcount = TIME_TO_TICKS(pCPlayer->flSimulationTimeReceivedMuzzleFlash + GetLerpTime());

				//Don't fire at pelvis this tick
				record->FireAtPelvis = false;

				//Remember this tick as the newest firing update
				pCPlayer->LatestFiringTick = record;

				//if (varsbeforeshot)
				record->tickcount = TIME_TO_TICKS((record->simulationtime - Interfaces::Globals->interval_per_tick) + gLagCompensation.GetLerpTime()) + 1;

				if (FireWhenEnemiesShootChk.Checked)
				{
					IsBacktrackable = true;
				}
			}
			pCPlayer->BulletsLeft = BulletsLeft;
		}
	}
	pCPlayer->CurrentWeapon = pWeapon;
	pCPlayer->bReceivedMuzzleFlash = false;

	//The EyeAnglesUpdated proxy isn't really used anymore in here.
	//pCPlayer->EyeAnglesUpdated = false;

	//Store spinbotting value into the record
	record->bSpinbotting = pCPlayer->PersistentData.bSpinbotting;

	//Check to see if the player is using a pitch that is easily hittable
	//Todo, check smaller pitch but 
#if 0
	if (ClampXr(record->eyeangles.x) <= 0.0f && !record->FiredBullet && ValveResolverChk.Checked)
	{
		//Don't count holdaim packets as a pitching up tick
		//UPDATE: holdaim doesn't seem to affect it!
		//if (pCPlayer->Personalize.m_PlayerRecords.size() <= 1 || !pCPlayer->Personalize.m_PlayerRecords.at(1)->FiredBullet)
		{
			//if (!pCPlayer->LatestFiringTick || record->eyeangles != pCPlayer->LatestFiringTick->eyeangles)
			{
				//Remember this tick as the newest pitching up update
				pCPlayer->LatestPitchUpTick = record;
				//Make this record known as a pitching up tick
				record->PitchingUp = true;
				record->FireAtPelvis = false;
				IsBacktrackable = true;
			}
		}
	}
#endif

	//See if player is using a lower body yaw breaker
	C_AnimationLayer *layer = Entity->GetAnimOverlay(3);
	if (pCPlayer->TicksChoked > 0 && PreviousNetvars && !pCPlayer->LowerBodyUpdated && !pCPlayer->LatestLowerBodyUpdate && !pCPlayer->LatestRealTick && !pCPlayer->LatestBloodTick && layer)
	{
		if (layer->_m_flCycle == 0.0f && layer->m_flWeight == 0.0f && PreviousNetvars->AnimLayer[3]._m_flCycle != 0.0f && PreviousNetvars->AnimLayer[3].m_flWeight != 0.0f)
		{
#if 1
			float flLatency = GetNetworkLatency(FLOW_OUTGOING);
			float TimeSinceLBYUpdate = (Interfaces::Globals->curtime - pCPlayer->TimeLowerBodyUpdated) - flLatency;
			bool bMoving = Entity->GetVelocity().Length() > 0.1f;
			float flTimeSinceLastDetection = Interfaces::Globals->curtime - pCPlayer->flLastLowerBodyYawBreakerDetectTime;

			if (flTimeSinceLastDetection > 3.0f)
			{
				pCPlayer->PersistentData.correctedlbybreakerindex = 0;
			}

			if (pCPlayer->PersistentData.correctedlbybreakerindex < 2 && flTimeSinceLastDetection >(bMoving ? 0.22f : 1.0f) && TimeSinceLBYUpdate > (bMoving ? 0.22f : 1.25f))
			{
				float plbeyedelta = fabsf(ClampYr(PreviousNetvars->eyeangles.y - record->lowerbodyyaw));
				float lbyeyedelta = fabsf(ClampYr(record->eyeangles.y - record->lowerbodyyaw));

				if (lbyeyedelta > 35.0f || plbeyedelta > 35.0f)
					record->eyeangles.y = record->eyeangles.y;
				else
					record->eyeangles.y = record->lowerbodyyaw;

				//char tmp[128];
				//sprintf(tmp, "%f  %f", lbyeyedelta, plbeyedelta);
				//Interfaces::DebugOverlay->AddTextOverlay(Entity->GetOrigin(), 1.5f, tmp);


				pCPlayer->LatestRealTick = record;
				record->IsRealTick = true;
				record->FireAtPelvis = false;
				record->tickcount = PreviousNetvars->tickcount;
				//pCPlayer->PersistentData.ShotsMissed = 0;
				pCPlayer->PersistentData.correctedbodyyawindex = 0;
				pCPlayer->flLastLowerBodyYawBreakerDetectTime = Interfaces::Globals->curtime;
				IsBacktrackable = true;
			}
			//pCPlayer->LowerBodyUpdated = true;
#endif
		}
	}

	//See if lower body updated this tick
	if (pCPlayer->LowerBodyUpdated)
	{
		//Check to see if we already have a lower body update tick to delta with
		if (pCPlayer->LowerBodyCanBeDeltaed || record->FiredBullet)
		{
			//Get the delta
			float lbd = ClampYr(record->lowerbodyyaw - pCPlayer->flLastLowerBodyYaw);

			//Only consider this a lower body yaw update if the delta changed or the update is new enough.
			//Servers like to send duplicate lower body updates

			float flTimeSinceLowerBodyUpdate = Interfaces::Globals->curtime - pCPlayer->TimeLowerBodyUpdated;

			if (record->FiredBullet || lbd != 0.0f || Entity->GetVelocity().Length2D() > 0.1f || flTimeSinceLowerBodyUpdate >= (0.22f + (GetNetworkLatency(FLOW_INCOMING) + GetNetworkLatency(FLOW_OUTGOING))))
			{
				//Store new lower body record with delta if possible
				auto LowerBodyRecords = &pCPlayer->PersistentData.m_PlayerLowerBodyRecords;
				StoredLowerBodyYaw *PreviousLowerBodyRecord = LowerBodyRecords->size() != 0 ? LowerBodyRecords->front() : nullptr;
				StoredLowerBodyYaw *NewLowerBodyRecord = new StoredLowerBodyYaw(Entity, &pCPlayer->CurrentNetvarsFromProxy, PreviousLowerBodyRecord);
				pCPlayer->PersistentData.m_PlayerLowerBodyRecords.push_front(NewLowerBodyRecord);

				//Try to predict what the player's yaw will be
				//Exclude shots fired from prediction
				if (PredictFakeAnglesChk.Checked && !record->FiredBullet)
				{
					pCPlayer->bCouldPredictLowerBodyYaw = PredictLowerBodyYaw(pCPlayer, Entity, lbd);
				}

				IsBacktrackable = true;

				//Remember this tick as the newest lower body update
				if (!record->FiredBullet)
				{
					pCPlayer->LatestLowerBodyUpdate = record;
				}
				else
				{
					pCPlayer->PersistentData.correctedresolvewhenshootingindex = 0;
					record->eyeangles.y = record->lowerbodyyaw;
				}

				//Make this record known as a lower body update
				record->LowerBodyUpdated = true;

				//Reset missed shots if lower body yaw updated
				pCPlayer->PersistentData.correctedbodyyawindex = 0;
				pCPlayer->PersistentData.correctednotfakeindex = 0;
				pCPlayer->PersistentData.ShotsMissed = 0;

				//Set time stamp
				pCPlayer->TimeLowerBodyUpdated = Interfaces::Globals->curtime;
			}
		}
		else
		{
			//First lower body update
			//Remember this tick as the newest lower body update
			//pCPlayer->LatestLowerBodyUpdate = record;
			//Set time stamp
			//pCPlayer->TimeLowerBodyUpdated = Interfaces::Globals->curtime;
			IsBacktrackable = true;
			pCPlayer->LowerBodyCanBeDeltaed = true;
			pCPlayer->PersistentData.correctedbodyyawindex = 0;
			pCPlayer->PersistentData.correctednotfakeindex = 0;
			pCPlayer->PersistentData.ShotsMissed = 0;

			//Store first lower body record
			StoredLowerBodyYaw *NewLowerBodyRecord = new StoredLowerBodyYaw(Entity, &pCPlayer->CurrentNetvarsFromProxy, nullptr);
			pCPlayer->PersistentData.m_PlayerLowerBodyRecords.push_back(NewLowerBodyRecord);
		}

		//Remember current lower body yaw
		pCPlayer->flLastLowerBodyYaw = record->lowerbodyyaw;
		pCPlayer->LowerBodyUpdated = false;
	}
	/*
	else
	{
	//Check to see if player is moving, if they are then we can backtrack to LBY
	Vector Velocity = Entity->GetVelocity();
	//if (Velocity.Length2D() > 0.1f && Velocity.z >= 0)
	if (Velocity.Length2D() > 0.1f && (pCPlayer->BaseEntity->GetFlags() & FL_ONGROUND))
	{
	//Remember this tick as the newest lower body update
	pCPlayer->LatestLowerBodyUpdate = record;
	//Make this record known as a lower body update
	record->LowerBodyUpdated = true;
	//Set time stamp
	pCPlayer->TimeLowerBodyUpdated = Interfaces::Globals->curtime;

	pCPlayer->PredictedAverageFakeAngle = record->lowerbodyyaw;

	IsBacktrackableTick = true;
	}
	}
	*/

	//If player didn't choke ticks then backtrack to this tick always
	if (pCPlayer->TicksChoked == 0 && !record->FiredBullet && (pCPlayer->PersistentData.m_PlayerRecords.size() <= 1 || !pCPlayer->PersistentData.m_PlayerRecords.at(1)->FiredBullet))
	{
		pCPlayer->LatestRealTick = record;
		pCPlayer->PersistentData.correctedbodyyawindex = 0;
		pCPlayer->PersistentData.correctednotfakeindex = 0;
		//Make this record known as a real update
		record->IsRealTick = true;
		IsBacktrackable = true;
	}

	//Correct player's origin if it got phantomed
#if 0
	if (record->absorigin == vecZero || (record->absorigin - record->origin).Length() > 0.1f)
	{
		Entity->SetAbsOrigin(record->networkorigin);
	}
#endif

	if (IsBacktrackable)
	{
		//We found a new tick that is possible to backtrack to
		record->FireAtPelvis = false;
		pCPlayer->PersistentData.ShotsMissed = 0;
	}
	else if (!pCPlayer->GetBestBacktrackTick() && !pCPlayer->bCouldPredictLowerBodyYaw)
	{
		if (BAimIfCouldntPredictFakeAnglesChk.Checked && (AimbotAimTorsoChk.Checked || AimbotAimArmsHandsChk.Checked || AimbotAimLegsFeetChk.Checked))
			record->FireAtPelvis = true;
		else
			record->FireAtPelvis = false;
		//pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::AverageLBYDelta;
	}
	else
	{
		record->FireAtPelvis = false;
	}
}

//Finished FRAME_NET_UPDATE_START, and NET_UPDATE_END has been called
void LagCompensation::PostNetUpdatePlayer(CustomPlayer *const pCPlayer, CBaseEntity*const Entity)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("PostNetUpdatePlayer\n");
#endif
	if (DisableAllChk.Checked || !RemoveInterpChk.Checked)
		Entity->EnableInterpolation();
	else
		Entity->DisableInterpolation();

	//Get address of player records
	auto Records = &pCPlayer->PersistentData.m_PlayerRecords;
	int RecordCount = Records->size();

	if (RecordCount)
	{
		//Store netvars that are only updated during PostDataUpdate for this player
		StoredNetvars *CurrentNetvars = Records->front();
		if (!CurrentNetvars->ReceivedPostNetUpdateVars)
		{
			CurrentNetvars->absangles = Entity->GetAbsAngles();
			CurrentNetvars->angles = Entity->GetAngleRotation();
			CurrentNetvars->absorigin = *Entity->GetAbsOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			CurrentNetvars->origin = Entity->GetOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
														  //CurrentNetvars->networkorigin = Entity->GetNetworkOrigin();
			CurrentNetvars->velocity = Entity->GetVelocity(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			CurrentNetvars->basevelocity = Entity->GetBaseVelocity();

			if (CurrentNetvars->velocity.Length() > 0.1f)
			{
				//Clear predicted angles
				pCPlayer->PredictedLinearFakeAngle = -999.9f;
				pCPlayer->PredictedRandomFakeAngle = -999.9f;
				pCPlayer->PredictedStaticFakeAngle = -999.9f;
				pCPlayer->PredictedAverageFakeAngle = -999.9f;

				pCPlayer->IsMoving = true;
			}
			else
			{
				pCPlayer->IsMoving = false;
			}

			//IsBreakingLagCompensation is now handled in EvaluateNewTickRecord
			//StoredNetvars *PreviousNetvars = pCPlayer->GetPreviousRecord();
			//pCPlayer->IsBreakingLagCompensation = PreviousNetvars && !PreviousNetvars->dormant && (CurrentNetvars->networkorigin - PreviousNetvars->networkorigin).LengthSqr() > (64.0f * 64.0f);

			CurrentNetvars->ReceivedPostNetUpdateVars = true;
		}
	}
}

void LagCompensation::PreRenderStart()
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("PreRenderStart\n");
#endif
	if (LocalPlayer.Entity)
	{
		//Need to iterate the whole thing since rendering is on a different thread than networking..
		int MaxEntities = 64;// Interfaces::ClientEntList->GetHighestEntityIndex();
		int NumPlayers = GetClientCount();
		int CountedPlayers = 0;

		if (NumPlayers)
		{
			for (int i = 0; i <= MaxEntities; i++)
			{
				CBaseEntity* Entity = Interfaces::ClientEntList->GetClientEntity(i);

				if (Entity && Entity->IsPlayer())
				{
					if (Entity != LocalPlayer.Entity && !Entity->GetDormant())
					{
						if (!DisableAllChk.Checked && RemoveInterpChk.Checked)
							Entity->DisableInterpolation();
						else
							Entity->EnableInterpolation();

						//Entity->FrameAdvance(0.0f);
					}

					if (++CountedPlayers == NumPlayers)
						break;
				}
			}
		}

		if (!DisableAllChk.Checked)
		{
			SetThirdPersonAngles();

			//Stop the game from updating player animations and pose parameters
			for (unsigned int i = 0; i < g_ClientSideAnimationList->count; i++)
			{
				clientanimating_t *animating = (clientanimating_t*)g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
				CBaseEntity *Entity = (CBaseEntity*)animating->pAnimating;
				if (Entity->IsPlayer() && Entity != LocalPlayer.Entity && !Entity->GetDormant() && Entity->GetAlive())
				{
					CustomPlayer *pCPlayer = &AllPlayers[Entity->index];
					if (pCPlayer->PersistentData.ShouldResolve())
					{
						unsigned int flags = animating->flags;
						pCPlayer->ClientSideAnimationFlags = flags;
						pCPlayer->HadClientAnimSequenceCycle = (flags & FCLIENTANIM_SEQUENCE_CYCLE);
						if (pCPlayer->HadClientAnimSequenceCycle)
						{
#if 0
							StoredNetvars *CurrentVars = pCPlayer->GetCurrentRecord();
							if (CurrentVars && pCPlayer->IsBreakingLagCompensation && Interfaces::Globals->tickcount != CurrentVars->TickReceivedNetUpdate)
							{
								Entity->UpdateClientSideAnimation();
								CurrentVars->absangles = Entity->GetAbsAngles();
								//Store the new animations
								Entity->CopyPoseParameters(CurrentVars->flPoseParameter);
								Entity->CopyAnimLayers(CurrentVars->AnimLayer);
							}
#endif
							animating->flags &= ~FCLIENTANIM_SEQUENCE_CYCLE;
						}
					}
				}
			}
		}
	}
}

//Stores player pose parameters and animations for this tick since CreateMove happens before these are updated
void LagCompensation::PostRenderStart()
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("PostRenderStart\n");
#endif
	if (LocalPlayer.Entity && !DisableAllChk.Checked)
	{
		//No need to copy animations from here anymore as we handle them ourselves now
#if 0
		int MaxEntities = 64;// Interfaces::ClientEntList->GetHighestEntityIndex();
		int NumPlayers = GetClientCount();
		int CountedPlayers = 0;

		if (NumPlayers)
		{
			for (int i = 0; i <= MaxEntities; i++)
			{
				CBaseEntity* Entity = Interfaces::ClientEntList->GetClientEntity(i);

				if (Entity && Entity->IsPlayer())
				{
					CustomPlayer* pCPlayer = &AllPlayers[i];

					if (pCPlayer->Active && !pCPlayer->IsLocalPlayer && !Entity->GetDormant() && Entity->GetAlive() && pCPlayer->Personalize.m_PlayerRecords.size() != 0)
					{
						StoredNetvars *CurrentNetvars = pCPlayer->Personalize.m_PlayerRecords.front();


						if (!CurrentNetvars->ReceivedPoseParametersAndAnimations)
						{
							//Store poses for the current tick
							Entity->CopyPoseParameters(CurrentNetvars->flPoseParameter);

							//Store animations for the current tick
							Entity->CopyAnimLayers(CurrentNetvars->AnimLayer);

							CurrentNetvars->GoalFeetYaw = Entity->GetGoalFeetYaw();
							CurrentNetvars->CurrentFeetYaw = Entity->GetCurrentFeetYaw();
#ifdef _DEBUG
#ifdef SHOW_FEET_YAW
							OutputFeetYawInformation(pCPlayer, Entity);
#endif
#endif
							CurrentNetvars->ReceivedPoseParametersAndAnimations = true;
						}
					}

					if (++CountedPlayers == NumPlayers)
						break;
				}
			}
		}
#endif
		//Restore original animation flags after rendering
		for (unsigned int i = 0; i < g_ClientSideAnimationList->count; i++)
		{
			clientanimating_t *animating = (clientanimating_t*)g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
			CBaseEntity *Entity = (CBaseEntity*)animating->pAnimating;

			if (Entity->IsPlayer() && Entity != LocalPlayer.Entity && !Entity->GetDormant() && Entity->GetAlive())
			{
				CustomPlayer *pCPlayer = &AllPlayers[Entity->index];
				if (pCPlayer->HadClientAnimSequenceCycle && pCPlayer->PersistentData.ShouldResolve())
				{
					animating->flags |= FCLIENTANIM_SEQUENCE_CYCLE;
				}
			}
		}
	}
}

//Called from CreateMove to get ideal tick to set cmd->tickcount
void LagCompensation::AdjustTickCountForCmd(CBaseEntity *const pPlayerFiredAt, bool bLocalPlayerIsFiring)
{
	VMP_BEGINMUTILATION("ATCFC")
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("AdjustTickCountForCmd\n");
#endif
	//int lerpTicks = RemoveInterpChk.Checked ? TIME_TO_TICKS(cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat()) : 0;
	//pPlayerFiredAt is the player we are aimbotting or triggerbotting and we are definitely firing a bullet this tick
	if (pPlayerFiredAt)
	{
		CustomPlayer *pCPlayer = &AllPlayers[pPlayerFiredAt->index];

#ifdef ONLY_SHOOT_WHEN_BLOOD_SPAWNS
		//for debugging blood resolver
		if (!pCPlayer->GetBestBacktrackTick() || pCPlayer->GetBestBacktrackTick() != pCPlayer->LatestBloodTick)
		{
			if (!GetAsyncKeyState(VK_LBUTTON))
				CurrentUserCmd.cmd->buttons &= ~IN_ATTACK;
		}
#endif
		int backtracktick = pCPlayer->iBacktrackTickCount;
		//if (BacktrackExploitChk.Checked)
		//	backtracktick -= 1;

		if (bTickIsValid(backtracktick))
		{
			//	int tick = pCPlayer->BacktrackTick;
			//	int tickssinceupdate = Interfaces::Globals->tickcount - pCPlayer->TickReceivedLowerBodyUpdate;
			//	if (tickssinceupdate > 0 && pCPlayer->GetBestBacktrackTick() == pCPlayer->LatestLowerBodyUpdate && bTickIsValid(tick + tickssinceupdate))
			//		tick += tickssinceupdate;
			//	WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, tick);
			//int inc = CurrentUserCmd.bSendPacket ? 0 : 1;

			CurrentUserCmd.cmd->tick_count = backtracktick;
		}
		else
		{
			int targettick;
			if (RemoveInterpChk.Checked)
				targettick = TIME_TO_TICKS(pCPlayer->CurrentNetvarsFromProxy.simulationtime) + TIME_TO_TICKS(GetLerpTime());
			else
				targettick = CurrentUserCmd.cmd->tick_count;
			WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, targettick);
		}
		//Store hit percentage so resolver won't count really inaccurate shots as misses
		LastShotHitPercentage = CurrentHitPercentage;
	}
	else
	{
		if (bLocalPlayerIsFiring)
		{
			Vector vecDir;
			Vector LocalEyePos = LocalPlayer.Entity->GetEyePosition(); //FIXME: correct this eye position for simtime?
			QAngle EyeAngles = LocalPlayer.Entity->GetEyeAngles();
			AngleVectors(EyeAngles, &vecDir);
			VectorNormalizeFast(vecDir);
			Vector EndPos = LocalEyePos + (vecDir * 8192.0f);
			trace_t tr;
			UTIL_TraceLine(LocalEyePos, EndPos, MASK_SHOT, LocalPlayer.Entity, &tr);
			CTraceFilterPlayersOnlyNoWorld filter;
			filter.AllowTeammates = true;
			filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
			filter.m_icollisionGroup = COLLISION_GROUP_NONE;
			UTIL_ClipTraceToPlayers(LocalEyePos, EndPos + vecDir * 40.0f, 0x4600400B, (ITraceFilter*)&filter, &tr);
			if (MTargetting.IsPlayerAValidTarget(tr.m_pEnt))
			{
				CBaseCombatWeapon* weapon = LocalPlayer.Entity->GetWeapon();
				if (gTriggerbot.WeaponCanFire(weapon))
				{
					//Store hit percentage so resolver won't count really inaccurate shots as misses
					BulletWillHit(weapon, buttons, tr.m_pEnt, EyeAngles, LocalEyePos, &EndPos, MTargetting.CURRENT_HITCHANCE_FLAGS);
					LastShotHitPercentage = CurrentHitPercentage;
				}
				int index = ReadInt((uintptr_t)&tr.m_pEnt->index);
				CustomPlayer* pCPlayer = &AllPlayers[index];
				if (bTickIsValid(pCPlayer->iBacktrackTickCount))
				{
					WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, pCPlayer->iBacktrackTickCount);
				}
				else
				{
					int targettick;
					if (RemoveInterpChk.Checked)
						targettick = TIME_TO_TICKS(pCPlayer->CurrentNetvarsFromProxy.simulationtime) + TIME_TO_TICKS(GetLerpTime());
					else
						targettick = CurrentUserCmd.cmd->tick_count;
					WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, targettick);
				}
			}
			else
			{
				CBaseEntity *pPlayerClosestToCrosshair = MTargetting.GetPlayerClosestToCrosshair(20.0f);
				if (pPlayerClosestToCrosshair)
				{
					CustomPlayer* pCPlayer = &AllPlayers[pPlayerClosestToCrosshair->index];
					if (bTickIsValid(pCPlayer->iBacktrackTickCount))
					{
						WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, pCPlayer->iBacktrackTickCount);
					}
					else
					{
						int targettick;
						if (RemoveInterpChk.Checked)
							targettick = TIME_TO_TICKS(pCPlayer->CurrentNetvarsFromProxy.simulationtime) + TIME_TO_TICKS(GetLerpTime());
						else
							targettick = CurrentUserCmd.cmd->tick_count;
						WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, targettick);
					}
				}
				else
				{
					int targettick = CurrentUserCmd.cmd->tick_count;
					if (RemoveInterpChk.Checked)
					{
						targettick += TIME_TO_TICKS(GetLerpTime());
					}
					WriteInt((uintptr_t)&CurrentUserCmd.cmd->tick_count, targettick);
				}
			}
		}
	}
	VMP_END
}
