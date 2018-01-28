#include "player_lagcompensation.h"
#include "LocalPlayer.h"
#include "Intersection.h"
#include "INetchannelInfo.h"
#include "Netchan.h"
#include "VMProtectDefs.h"
#include "Logging.h"

//Gets the ideal angles for backtracking to a tick a player shot
QAngle LagCompensation::GetFiringTickAngles(CustomPlayer *const pCPlayer, CBaseEntity*const Entity, StoredNetvars*const  vars)
{
	QAngle TargetEyeAngles = vars->eyeangles;

	if (pCPlayer->PersistentData.ShouldResolve() && FiringCorrectionsChk.Checked)
	{
		if (vars->tickschoked != 0)
		{
			StoredNetvars *TickBeforeFiring = nullptr;
			bool FoundFiringTick = false;
			for (auto tick : pCPlayer->PersistentData.m_PlayerRecords)
			{
				if (FoundFiringTick)
				{
					TickBeforeFiring = tick;
					break;
				}
				else if (tick == vars)
				{
					FoundFiringTick = true;
				}
			}

			if (TickBeforeFiring)
			{
				if (pCPlayer->PersistentData.correctedresolvewhenshootingindex % 2 == 1)
				{
					TargetEyeAngles.x = TickBeforeFiring->eyeangles.x;
					//if (!vars->LowerBodyUpdated)
					//	TargetEyeAngles.y = TickBeforeFiring->eyeangles.y;
					/*else
					{
					//TargetEyeAngles.y = vars->lowerbodyyaw;
					}
					*/
				}
				else
				{
					TargetEyeAngles.x = vars->eyeangles.x;
					//if (!vars->LowerBodyUpdated)
					//	TargetEyeAngles.y = Entity->GetGoalFeetYaw();// TickBeforeFiring->lowerbodyyaw;
					/*else
					{
					TargetEyeAngles.y = vars->lowerbodyyaw;
					}
					*/
				}
			}
#if 0
			if (TargetEyeAngles.x > 80.0f)
			{
				QAngle angletous = CalcAngle(Entity->GetEyePosition(), LocalPlayer.Entity->GetEyePosition());
				TargetEyeAngles.x = angletous.x;
				//TargetEyeAngles.y = angletous.y - 45.0f;
			}
#endif

#if 0
			if (pCPlayer->LatestFiringTick->tickschoked > 0)
			{
				//Extrapolate yaw
				//FIXME: THIS IS FUCKED, WE NEED TO STORE PREDICTED ANGLES FOR EACH TICK RECORDS AND USE THOSE, NOT THE CURRENT TICK'S PREDICTED ONES!!
				GetIdealYaw(TargetEyeAngles.y, pCPlayer->Personalize.correctedindex, pCPlayer, Entity, LocalPlayer.Entity);
			}
#endif
		}

		if (TargetEyeAngles.y < 0.0f)
			TargetEyeAngles.y += 360.0f;
	}

	return TargetEyeAngles;

#if 0
	if (pCPlayer->Personalize.ShouldResolve() && FiringCorrectionsChk.Checked && pCPlayer->Personalize.correctedresolvewhenshootingindex > 0)
	{
		auto Records = &pCPlayer->Personalize.m_PlayerRecords;
		const int NumRecords = Records->size();
		if (NumRecords < 2)
		{
			return TargetEyeAngles;
		}

		int index = 0;
		for (auto& Record : *Records)
		{
			if (Record == pCPlayer->LatestFiringTick)
				break;
			index++;
		}

		if (++index <= (NumRecords - 1))
		{
			//Use Previous tick's pitch
			auto& TickBeforeFiring = Records->at(index);
			TargetEyeAngles.x = TickBeforeFiring->eyeangles.x;
		}

		//Extrapolate yaw
		GetIdealYaw(TargetEyeAngles.y, pCPlayer->Personalize.correctedindex, pCPlayer, Entity, LocalPlayer.Entity);

		if (TargetEyeAngles.y < 0.0f)
			TargetEyeAngles.y += 360.0f;
	}

	return TargetEyeAngles;
#endif
}


//Sets the ideal yaw for this player
//FIXME: WE NEED TO STORE PREDICTED ANGLES FOR EACH TICK RECORD AND USE THOSE, NOT THE CURRENT TICK'S PREDICTED ONES!!!
//THE CURRENT SETUP WON'T WORK FOR GETTING PREDICTED ANGLES FOR OLD TICKS DURING BACKTRACKING
void LagCompensation::GetIdealYaw(float& yaw, int correctedindex, CustomPlayer* pCPlayer, CBaseEntity* Entity, CBaseEntity* pLocalEntity)
{
	VMP_BEGINMUTILATION("GIY")
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("GetIdealYaw\n");
#endif
#ifdef NO_IDEAL_YAW
	pCPlayer->Personalize.LastResolveModeUsed = ResolveYawModes::NoYaw;
	return;
#endif

	if (!pLocalEntity)
	{
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::NoYaw;
		return;
	}

	Personalize *pPersistentData = &pCPlayer->PersistentData;

	int NumberOfRecords = pPersistentData->m_PlayerRecords.size();

	if (NumberOfRecords == 0 || pPersistentData->DoNotResolve)
	{
		pPersistentData->LastResolveModeUsed = ResolveYawModes::NoYaw;
		return;
	}

#ifdef logicResolver
	if (WhaleDongTxt.iValue >= 1 && WhaleDongTxt.iValue != 0)
	{
		pCPlayer->Personalize.LastResolveModeUsed = WD;
		QAngle yawang = QAngle(0, yaw, 0);
		LogicResolver2_Predict(yawang, false, pCPlayer, pLocalEntity, 0);
		yaw = yawang.y;
		ClampY(yaw);
		if (yaw < 0.0f)
			yaw += 360.0f;

		return;
	}
#endif

	//Try to resolve those legit antiaimers that you don't know about
	if (!pPersistentData->ShouldResolve())
	{
		if (NumberOfRecords >= 2)
		{
			auto Records = &pPersistentData->m_PlayerRecords;
			//Check to see if last two ticks had choked packets
			if (Records->at(0)->tickschoked && Records->at(1)->tickschoked)
			{
				if (pCPlayer->IsMoving)
				{
					yaw = ResolveStaticFake(pCPlayer);
				}
				else
				{
					float TimeSinceLBYUpdate = (Interfaces::Globals->curtime - pCPlayer->TimeLowerBodyUpdated) - GetNetworkLatency(FLOW_OUTGOING);
					StoredNetvars *CurrentVars = pPersistentData->m_PlayerRecords.front();
					float delta = ClampYr(ClampYr(CurrentVars->eyeangles.y) - CurrentVars->lowerbodyyaw);
					if (fabsf(delta) > 35.0f && TimeSinceLBYUpdate > 0.4f)
						yaw = ResolveStaticFake(pCPlayer);
				}
			}
		}
		return;
	}

	ResolveYawModes mode = pPersistentData->ResolveYawMode;

	switch (mode)
	{
		//Manual selected resolve modes
	case ResolveYawModes::NoYaw:
		return;
	case ResolveYawModes::FakeSpins:
		yaw = ResolveFakeSpins(pCPlayer);
		return;
	case ResolveYawModes::LinearFake:
		yaw = ResolveLinearFake(pCPlayer);
		return;
	case ResolveYawModes::RandomFake:
		yaw = ResolveRandomFake(pCPlayer);
		return;
	case ResolveYawModes::CloseFake:
		pCPlayer->CloseFakeAngle = true;
		yaw = ResolveCloseFakeAngles(pCPlayer);
		return;
	case ResolveYawModes::BloodBodyRealDelta:
		yaw = ResolveBloodBodyRealDelta(pCPlayer);
		return;
	case ResolveYawModes::BloodEyeRealDelta:
		yaw = ResolveBloodEyeRealDelta(pCPlayer);
		return;
	case ResolveYawModes::BloodReal:
		yaw = ResolveBloodReal(pCPlayer);
		return;
	case ResolveYawModes::AtTarget:
		yaw = ResolveAtTargetFake(pCPlayer);
		return;
	case ResolveYawModes::InverseAtTarget:
		yaw = ResolveInverseAtTargetFake(pCPlayer);
		return;
	case ResolveYawModes::AverageLBYDelta:
		yaw = ResolveAverageLBYDelta(pCPlayer);
		return;
	case ResolveYawModes::StaticFakeDynamic:
		yaw = ResolveStaticFakeDynamic(pCPlayer);
		return;
	case ResolveYawModes::FakeWalk:
		yaw = ResolveFakeWalk(pCPlayer);
		return;
	case ResolveYawModes::StaticFake:
		yaw = ResolveStaticFake(pCPlayer);
		return;
	case ResolveYawModes::Inverse:
		yaw += 180.0f;
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::Inverse;
		break;
	case ResolveYawModes::Back:
		yaw = CalcAngle(Entity->GetEyePosition(), pLocalEntity->GetEyePosition()).y - 180.0f;
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::Back;
		break;
	case ResolveYawModes::Left:
		yaw += 90.0f;
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::Left;
		break;
	case ResolveYawModes::Right:
		yaw -= 90.0f;
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::Right;
		break;
	case ResolveYawModes::CustomStaticYaw:
		yaw = ResolveYawTxt.flValue;
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::CustomStaticYaw;
		break;
	case ResolveYawModes::CustomAdditiveYaw:
		yaw += ResolveYawTxt.flValue;
		pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::CustomAdditiveYaw;
		break;
		//Automatic resolve
	case ResolveYawModes::AutomaticYaw:
	{
		if (pCPlayer->TicksChoked == 0 && !pCPlayer->ResolverFiredAtShootingAngles)
		{
			pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::NoYaw;
			return;
		}

		StoredNetvars* currentrecord = pCPlayer->GetCurrentRecord();
		StoredLowerBodyYaw* currentlbyrecord = pCPlayer->GetCurrentLowerBodyRecord();;
		StoredLowerBodyYaw* previouslbyrecord = pCPlayer->GetPreviousLowerBodyRecord();

		//If the lower body yaw record had eyeangles matching the lower body yaw then assume they are not faking eye angles
		if (pCPlayer->PersistentData.correctednotfakeindex < 2 && pCPlayer->TicksChoked != 0 && currentlbyrecord && previouslbyrecord)
		{
			if (fabsf(ClampYr(currentlbyrecord->lowerbodyyaw - currentlbyrecord->eyeangles.y)) <= 5.0f
				&& fabsf(ClampYr(previouslbyrecord->lowerbodyyaw - previouslbyrecord->eyeangles.y)) <= 5.0f
				&& fabsf(ClampYr(currentrecord->eyeangles.y - currentrecord->lowerbodyyaw)) <= 35.0f)
			{
				pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::NotFaked;
				return;
			}
		}

		const Vector velocity = Entity->GetVelocity();
		float speed = velocity.Length2D();
		bool Moving = speed > 0.1f;
		int OnGround = Entity->GetFlags() & FL_ONGROUND;
		float TimeSinceLBYUpdate = (Interfaces::Globals->curtime - pCPlayer->TimeLowerBodyUpdated) - GetNetworkLatency(FLOW_OUTGOING);

		if (!pCPlayer->IsBreakingLagCompensation)
		{
			if (Moving && OnGround)
			{
				//fake walk
				if (speed > 1.0f && speed < 20.0f)
					//&& (Interfaces::Globals->curtime - pCPlayer->flTimeWhenFirstStartedMoving > 0.125f) 
					//&& (TimeSinceLBYUpdate > (TICKS_TO_TIME(pCPlayer->TicksChoked) + 0.22f)))
					//if (speed > 12.0f && speed < 18.0f)
				{
					yaw = ResolveFakeWalk(pCPlayer);
				}
				else
				{
					yaw = ResolveStaticFake(pCPlayer);
				}
				return;
			}
			else if (pCPlayer->CurrentWeapon && pCPlayer->CurrentWeapon->IsGun() && pCPlayer->BulletsLeft == 0)
			{
				yaw = ResolveStaticFake(pCPlayer);
				return;
			}
		}

		if (PredictFakeAnglesChk.Checked)
		{
			//Automatic Resolver

			//bool poseclose = fabsf(Entity->GetPoseParameterUnscaled(11)) < 0.1f;
			//pCPlayer->CloseFakeAngle = Entity->GetVelocity().Length() == 0.0f && ((fabsf(Entity->GetPoseParameterUnscaled(11)) < 0.5f /*|| fabsf(pCPlayer->CurrentNetVars.eyeangles.y - pCPlayer->CurrentNetVars.lowerbodyyaw) < 3.0f*/) || (ReadFloat((uintptr_t)&Interfaces::Globals->curtime) - pCPlayer->TimeLowerBodyUpdated) > 1.55f);

			//float EyeBodyDelta = fabsf(pCPlayer->CurrentNetvarsFromProxy.eyeangles.y - pCPlayer->CurrentNetvarsFromProxy.lowerbodyyaw);
			//if (EyeBodyDelta <= 15.0f)
			//	return; //todo: miss counter for this!!

#if 1

			if (pCPlayer->IsBreakingLagCompensation || pCPlayer->IsMoving)
				pCPlayer->CloseFakeAngle = false;
			else
			{
				//Check to see if the enemy's lower body yaw is looking 180 degrees at us or 180 degrees away from us
				//If they are, then assume they are using either a static yaw or an Anti-LBY Antiaim
				//TODO: detect to see if lower body yaw has a history of spinning to rule out spinbots

				//TODO: FIX SHOTS FIRED DETECTION FOR THIS!
#ifdef FIXED
				float YawFromUsToEnemy = CalcAngle(LocalPlayer.Entity->GetEyePosition(), Entity->GetEyePosition()).y;
				float DeltaYaw = fabsf(ClampYr(YawFromUsToEnemy - pCPlayer->CurrentNetvarsFromProxy.lowerbodyyaw));
				pCPlayer->CloseFakeAngle = false;
				//FIXME BUGBUG: WasInverseFakingWhileMoving is not the same kind of inverse check! Check PredictLowerBodyYaw to see what I mean
				if (DeltaYaw < 5.0f /*|| (pPersistentData->WasInverseFakingWhileMoving && pPersistentData->ShotsMissed < 2)*/)
				{
					pPersistentData->EstimateYawMode = ResolveYawModes::InverseAtTarget;
				}
				else if (DeltaYaw > 175.0f)
				{
					pPersistentData->EstimateYawMode = ResolveYawModes::AtTarget;
				}
				else
#endif
				{
					//If player's lower body yaw has not updated in > 1.1 seconds then assume they are using an Anti-LBY Antiaim
					float bodydelta = fabsf(ClampYr(pCPlayer->CurrentNetvarsFromProxy.lowerbodyyaw - pCPlayer->CurrentNetvarsFromProxy.eyeangles.y));
					pCPlayer->CloseFakeAngle = TimeSinceLBYUpdate >= 1.25f && fabsf(ClampYr(pCPlayer->CurrentNetvarsFromProxy.lowerbodyyaw - pCPlayer->CurrentNetvarsFromProxy.eyeangles.y)) <= 35.0f;
				}
			}
#endif

			//Get predicted yaw mode
			ResolveYawModes mode = pPersistentData->EstimateYawMode;

			if (pCPlayer->PersistentData.ShotsMissed >= 1 && mode >= ResolveYawModes::BloodBodyRealDelta && mode <= ResolveYawModes::BloodReal)
			{
				pPersistentData->EstimateYawMode = pPersistentData->LastEstimateYawMode;
				mode = pPersistentData->LastEstimateYawMode;
			}

			if (Moving)
			{
				if (OnGround)
				{
					mode = ResolveYawModes::StaticFake;
				}
				else
				{
					if (velocity.z < 0.0f)
						mode = pPersistentData->EstimateYawModeInAir;
					else
						mode = ResolveYawModes::StaticFake;
				}
			}

			bool bUseCloseFakeAngle = pCPlayer->CloseFakeAngle;

			if (bUseCloseFakeAngle)
			{
				if (mode >= ResolveYawModes::BloodBodyRealDelta && mode <= ResolveYawModes::BloodReal)
				{
					switch (mode)
					{
						float diff;
					case ResolveYawModes::BloodBodyRealDelta:
						diff = ClampYr(currentrecord->lowerbodyyaw - ClampYr(currentrecord->lowerbodyyaw + pCPlayer->LastBodyRealDelta));
						if (fabsf(diff) < 40.0f)
							bUseCloseFakeAngle = false;
						break;
					case ResolveYawModes::BloodEyeRealDelta:
						diff = ClampYr(currentrecord->lowerbodyyaw - ClampYr(currentrecord->eyeangles.y + pCPlayer->LastEyeRealDelta));
						if (fabsf(diff) < 40.0f)
							bUseCloseFakeAngle = false;
						break;
					case ResolveYawModes::BloodReal:
						diff = ClampYr(currentrecord->lowerbodyyaw - pCPlayer->LastRealYaw);
						if (fabsf(diff < 40.0f))
							bUseCloseFakeAngle = false;
						break;
					}
				}
			}

			if (bUseCloseFakeAngle)
			{
				//Resolve for 'Anti-Resolve Yaw'
				yaw = ResolveCloseFakeAngles(pCPlayer);
				return;
			}
			else
			{
				switch (mode)
				{
				default:
				case ResolveYawModes::NoYaw:
					return;
				case ResolveYawModes::FakeSpins:
					yaw = ResolveFakeSpins(pCPlayer);
					return;
				case ResolveYawModes::LinearFake:
					yaw = ResolveLinearFake(pCPlayer);
					return;
				case ResolveYawModes::RandomFake:
					yaw = ResolveRandomFake(pCPlayer);
					return;
				case ResolveYawModes::AtTarget:
					yaw = ResolveAtTargetFake(pCPlayer);
					return;
				case ResolveYawModes::InverseAtTarget:
					yaw = ResolveInverseAtTargetFake(pCPlayer);
					return;
				case ResolveYawModes::AverageLBYDelta:
					yaw = ResolveAverageLBYDelta(pCPlayer);
					return;
				case ResolveYawModes::StaticFakeDynamic:
					yaw = ResolveStaticFakeDynamic(pCPlayer);
					return;
				case ResolveYawModes::FakeWalk:
					yaw = ResolveFakeWalk(pCPlayer);
					return;
				case ResolveYawModes::StaticFake:
					yaw = ResolveStaticFake(pCPlayer);
					return;
				case ResolveYawModes::BloodReal:
					//FIXME: check time since it was set!
					yaw = ResolveBloodReal(pCPlayer);
					return;
				case ResolveYawModes::BloodBodyRealDelta:
					yaw = ResolveBloodBodyRealDelta(pCPlayer);
					return;
				case ResolveYawModes::BloodEyeRealDelta:
					yaw = ResolveBloodEyeRealDelta(pCPlayer);
					return;
				}
			}
		}
		else
		{
			//Shots missed based resolver.
			//TODO: Could use cleanup
			Vector screen;
#ifdef _DEBUG
			bool Draw = false;// EXTERNAL_WINDOW && /*DrawEyeAnglesChk.Checked &&*/ WorldToScreenCapped(Entity->GetBonePosition(HITBOX_CHEST, Entity->GetSimulationTime()), screen);
#endif
							  //MAX_RESOLVER_MISSES is the max
			switch (correctedindex)
			{
			case 0:
				//Corrects non-fake angles
#ifdef _DEBUG
				if (Draw)
					DrawStringUnencrypted("Estimate Mode: No Yaw", Vector2D(screen.x - 200, screen.y + 25), ColorRGBA(255, 0, 0, 255), pFont);
#endif
				pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::NoYaw;
				break;
			case 1:
			case 2:
				if (pPersistentData->isCorrected)
					pPersistentData->correctedindex = 1;
			case 9:
			case 10:
				if (pPersistentData->isCorrected && correctedindex == 10)
					pPersistentData->correctedindex = 9;
				//Corrects slow spin with fakeangles
				yaw = ResolveFakeSpins(pCPlayer);
#ifdef _DEBUG
				if (Draw)
					DrawStringUnencrypted("Estimate Mode: Spin Fake", Vector2D(screen.x - 200, screen.y + 25), ColorRGBA(255, 0, 0, 255), pFont);
#endif
				return;
			case 3:
			case 4:
				if (pPersistentData->isCorrected)
					pPersistentData->correctedindex = 3;
			case 11:
			case 12:
				if (pPersistentData->isCorrected && correctedindex == 12)
					pPersistentData->correctedindex = 11;
				//Corrects linear fakeangles
				yaw = ResolveLinearFake(pCPlayer);
#ifdef _DEBUG
				if (Draw)
					DrawStringUnencrypted("Estimate mode: Linear Fake", Vector2D(screen.x - 200, screen.y + 25), ColorRGBA(255, 0, 0, 255), pFont);
#endif
				return;
			case 5:
			case 6:
				if (pPersistentData->isCorrected)
					pPersistentData->correctedindex = 5;
			case 13:
			case 14:
			case 15:
			case 16:
				if (pPersistentData->isCorrected && correctedindex == 16)
					pPersistentData->correctedindex = 15;
				//Corrects random fake angles (ballerina) most of the time
				yaw = ResolveRandomFake(pCPlayer);
#ifdef _DEBUG
				if (Draw)
					DrawStringUnencrypted("Estimate mode: Random Fake", Vector2D(screen.x - 200, screen.y + 25), ColorRGBA(255, 0, 0, 255), pFont);
#endif
				return;
			case 7:
			case 8:
				if (pPersistentData->isCorrected && correctedindex == 8)
					pPersistentData->correctedindex = 7;
				//Corrects static fakeangles
				yaw = ResolveStaticFake(pCPlayer);
#ifdef _DEBUG
				if (Draw)
					DrawStringUnencrypted("Estimate mode: Static Fake", Vector2D(screen.x - 200, screen.y + 25), ColorRGBA(255, 0, 0, 255), pFont);
#endif
				return;
			case 17:
			case 18:
				if (pPersistentData->isCorrected && correctedindex == 18)
					pPersistentData->correctedindex = 17;
				//Corrects static dynamic fakeangles
				yaw = ResolveStaticFakeDynamic(pCPlayer);
#ifdef _DEBUG
				if (Draw)
					DrawStringUnencrypted("Estimate mode: Static Fake Dynamic", Vector2D(screen.x - 200, screen.y + 25), ColorRGBA(255, 0, 0, 255), pFont);
#endif
				return;
			}

		}
		break;
	}
	}
	ClampY(yaw);
	if (yaw < 0.0f)
		yaw += 360.0f;
	VMP_END
}

//Sets the ideal pitch for this player
void LagCompensation::GetIdealPitch(float& pitch, int correctedindex, CustomPlayer* pCPlayer, CBaseEntity* Entity, CBaseEntity* pLocalEntity)
{
	Personalize *pPersistentData = &pCPlayer->PersistentData;

	if (!pPersistentData->ShouldResolve())
		return;

	ResolvePitchModes mode = pPersistentData->ResolvePitchMode;

	switch (mode)
	{
	case ResolvePitchModes::NoPitch:
		return;
	case ResolvePitchModes::AutomaticPitch:

		if (!ValveResolverChk.Checked)
		{
			if (pitch < -179.f) pitch += 360.f;
			else if (pitch > 90.0 || pitch < -90.0) pitch = 89.f;
			else if (pitch > 89.0 && pitch < 91.0) pitch -= 90.f;
			else if (pitch > 179.0 && pitch < 181.0) pitch -= 180;
			else if (pitch > -179.0 && pitch < -181.0) pitch += 180;
			else if (fabs(pitch) == 0) pitch = std::copysign(89.0f, pitch);

			switch (correctedindex % 12)
			{
			case 10:
				pitch = -89.0f;
				break;
			case 11:
				pitch = 89.0f;
				break;

			}
		}
		break;
	case ResolvePitchModes::Up:
		pitch = -89.0f;
		break;
	case ResolvePitchModes::Down:
		pitch = 89.0f;
		break;
	case ResolvePitchModes::CustomStaticPitch:
		pitch = ResolvePitchTxt.flValue;
		break;
	case ResolvePitchModes::CustomAdditivePitch:
		pitch += ResolvePitchTxt.flValue;
		break;
	}
	ClampX(pitch);
	if (pitch < 0.0f)
		pitch += 360.0f;
}

//Attempts to predict what the player's lower body yaw will be using algorithms
bool LagCompensation::PredictLowerBodyYaw(CustomPlayer *const pCPlayer, CBaseEntity*const Entity, float LowerBodyDelta)
{
	VMP_BEGINMUTILATION("PLBY")
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("PredictLowerBodyYaw\n");
#endif

	auto stat = START_PROFILING("PredictLowerBodyYaw");
	StoredNetvars *CurrentNetVars = pCPlayer->GetCurrentRecord();
	auto LowerBodyRecords = &pCPlayer->PersistentData.m_PlayerLowerBodyRecords;
	bool FoundPattern = false;

	float flCurrentBodyYaw = CurrentNetVars->lowerbodyyaw;

	bool bPlayerIsMoving = Entity->GetVelocity().Length() > 0.1f;

	if (bPlayerIsMoving)
	{
		QAngle VelocityAngles;
		VectorAngles(Entity->GetVelocity(), VelocityAngles);

		pCPlayer->PersistentData.WasInverseFakingWhileMoving = fabsf(ClampYr(VelocityAngles.y - flCurrentBodyYaw)) >= 170.0f;

		if (!LowerBodyRecords->empty())
		{
			//Detects inverse very well
			StoredLowerBodyYaw *NewestLowerBodyRecord = LowerBodyRecords->front();
			if (NewestLowerBodyRecord->HasPreviousRecord && fabsf(NewestLowerBodyRecord->deltabetweenlastlowerbodyeyedelta) < 10.0f)
			{
				pCPlayer->PersistentData.EstimateYawModeInAir = ResolveYawModes::StaticFakeDynamic;
			}
			else
			{
				pCPlayer->PersistentData.EstimateYawModeInAir = pCPlayer->PersistentData.EstimateYawMode;
			}
		}
	}
	else
	{
		if (pCPlayer->LowerBodyEyeDelta != -999.9f)
		{
			//Not a very reliable resolve method at all
			if (pCPlayer->PredictedRandomFakeAngle != -999.9f)
			{
				if (fabsf(ClampYr(flCurrentBodyYaw - pCPlayer->PredictedRandomFakeAngle)) < 10.0f)
				{
					//random fake
					pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::RandomFake;
					FoundPattern = true;
				}
			}
			pCPlayer->PredictedRandomFakeAngle = ClampYr(CurrentNetVars->eyeangles.y + (pCPlayer->LowerBodyEyeDelta * 0.5f));
		}

		bool FoundToBeFakeSpinning = false; //ugh shit code

		if (pCPlayer->LowerBodyDelta != -999.9f)
		{
			if (CurrentNetVars->bSpinbotting && fabsf(ClampYr(LowerBodyDelta - pCPlayer->LowerBodyDelta)) < 10.0f)
			{
				//fake spin
				pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::FakeSpins;
				FoundPattern = true;
				FoundToBeFakeSpinning = true;
			}
			else
			{
				if (pCPlayer->PredictedLinearFakeAngle != -999.9f)
				{
					if (fabsf(ClampYr(flCurrentBodyYaw - pCPlayer->PredictedLinearFakeAngle)) < 15.0f)
					{
						//linear fake
						pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::LinearFake;
						FoundPattern = true;
					}
				}

				if (pCPlayer->PredictedStaticFakeAngle != -999.9f)
				{
					if (fabsf(ClampYr(flCurrentBodyYaw - pCPlayer->PredictedStaticFakeAngle)) < 10.0f)
					{
						//static fake
						pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::FakeSpins;
						FoundPattern = true;
					}
				}
			}
			pCPlayer->PredictedLinearFakeAngle = ClampYr(flCurrentBodyYaw + LowerBodyDelta);
			pCPlayer->PredictedStaticFakeAngle = ClampYr(ClampYr(CurrentNetVars->eyeangles.y) - LowerBodyDelta);

			if (!FoundToBeFakeSpinning && !LowerBodyRecords->empty())
			{
				//Detects inverse very well
				StoredLowerBodyYaw *NewestLowerBodyRecord = LowerBodyRecords->front();
				if (NewestLowerBodyRecord->HasPreviousRecord && fabsf(NewestLowerBodyRecord->deltabetweenlastlowerbodyeyedelta) < 10.0f)
				{
					pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::StaticFakeDynamic;
					FoundPattern = true;
				}
			}

			//Detect average lower body changes
			int LowerBodyRecordCount = LowerBodyRecords->size();
			if (LowerBodyRecordCount > 1)
			{
				float TotalYaws = 0.0f;
				int maxindex = min(LowerBodyRecordCount - 1, 4);
				int samples = 0;
				for (samples; samples < maxindex; samples++)
				{
					StoredLowerBodyYaw *record = LowerBodyRecords->at(samples);
					StoredLowerBodyYaw *previousrecord = LowerBodyRecords->at(samples + 1);
					if (record->networkorigin != previousrecord->networkorigin)
					{
						//If player moved, don't count it
						break;
					}
					TotalYaws += ClampYr(record->lowerbodyyaw - previousrecord->lowerbodyyaw);
				}

				if (samples)
				{
					float AverageYaw = ClampYr(TotalYaws / samples);
					if (!FoundPattern)
					{
						float Delta = ClampYr(flCurrentBodyYaw - pCPlayer->PredictedAverageFakeAngle);
						if (fabsf(Delta) <= 25.0f)
						{
							pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::AverageLBYDelta;
							FoundPattern = true;
						}
					}

					pCPlayer->PredictedAverageFakeAngle = ClampYr(flCurrentBodyYaw + AverageYaw);
				}
			}

			if (pCPlayer->PersistentData.EstimateYawMode == ResolveYawModes::AverageLBYDelta && pCPlayer->PredictedAverageFakeAngle == -999.9f)
			{
				//Don't allow invalidated angles
				pCPlayer->PersistentData.EstimateYawMode = ResolveYawModes::LinearFake;
			}
		}

		pCPlayer->LowerBodyDelta = LowerBodyDelta;

		float lbed = ClampYr(flCurrentBodyYaw - CurrentNetVars->eyeangles.y);
		pCPlayer->LowerBodyEyeDelta = lbed;
	}

	//if (pCPlayer->PersistentData.EstimateYawModeInAir == ResolveYawModes::NoYaw)
	{
		pCPlayer->PersistentData.EstimateYawModeInAir = pCPlayer->PersistentData.EstimateYawMode;
	}

	//Save in case we miss something like blood yaw modes
	pCPlayer->PersistentData.LastEstimateYawMode = pCPlayer->PersistentData.EstimateYawMode;

	END_PROFILING(stat);
	return FoundPattern;
	VMP_END
}

//Attempts to predict real player's yaw from blood received
bool LagCompensation::PredictRealYawFromBlood(CustomPlayer *const pCPlayer, CBaseEntity*const Entity, float yaw)
{
	VMP_BEGINMUTILATION("PRYFB")
		auto stat = START_PROFILING("PredictRealYawFromBlood");
	//FIXME: CHECK FOR INVALID DELTAS!!
	bool bCorrectPrediction = false;
	Personalize *pPersistentData = &pCPlayer->PersistentData;
	StoredNetvars *CurrentVars = &pCPlayer->CurrentNetvarsFromProxy;
	//auto BloodRecords = &pPersistentData->m_BloodRecords;

	float BodyRealDelta = ClampYr(CurrentVars->lowerbodyyaw - yaw);
	float EyeRealDelta = ClampYr(CurrentVars->eyeangles.y - yaw);
	float RealDelta = ClampYr(yaw - pCPlayer->LastRealYaw);
	if (fabsf(RealDelta < 10.0f))
	{
		pPersistentData->EstimateYawMode = ResolveYawModes::BloodReal;
		bCorrectPrediction = true;
	}
	if (fabsf(BodyRealDelta - pCPlayer->LastBodyRealDelta) < 10.0f)
	{
		pPersistentData->EstimateYawMode = ResolveYawModes::BloodBodyRealDelta;
		bCorrectPrediction = true;
	}
	if (fabsf(EyeRealDelta - pCPlayer->LastEyeRealDelta) < 10.0f)
	{
		pPersistentData->EstimateYawMode = ResolveYawModes::BloodEyeRealDelta;
		bCorrectPrediction = true;
	}
	pCPlayer->LastBodyRealDelta = BodyRealDelta;
	pCPlayer->LastEyeRealDelta = EyeRealDelta;
	pCPlayer->LastRealYaw = yaw;

	END_PROFILING(stat);
	return bCorrectPrediction;
	VMP_END
}

char *baimbloodresolveheadposstr = new char[26]{ 56, 21, 30, 3, 90, 59, 19, 23, 90, 40, 31, 9, 21, 22, 12, 31, 30, 90, 50, 31, 27, 30, 42, 21, 9, 0 }; /*Body Aim Resolved HeadPos*/
char *bodyhiterrorstr = new char[37]{ 63, 40, 40, 53, 40, 64, 90, 52, 53, 90, 40, 63, 57, 53, 40, 62, 90, 60, 53, 47, 52, 62, 90, 60, 53, 40, 90, 56, 53, 62, 35, 90, 50, 51, 46, 91, 0 }; /*ERROR: NO RECORD FOUND FOR BODY HIT!*/

void LagCompensation::OnPlayerBloodSpawned(CBaseEntity* const Entity, Vector* const origin, Vector* const vecDir)
{
	if (BloodResolverChk.Checked)
	{
		CustomPlayer* pCPlayer = &AllPlayers[Entity->index];
		Personalize *pPersistentData = &pCPlayer->PersistentData;
		if (pPersistentData->bAwaitingBloodResolve)
		{
			if (pCPlayer->BaseEntity && !pCPlayer->IsLocalPlayer && pPersistentData->ShouldResolve())
			{
				if (pPersistentData->m_PlayerRecords.size() == 0)
					return;

				//OnPlayerBloodSpawned seems to happen after the game events always, so no need for absolute value
				float flSimulationTime = pCPlayer->CurrentNetvarsFromProxy.simulationtime;
				if (flSimulationTime - pPersistentData->flSimulationTimePlayerWasShotOnClient < 0.5f)
				{
					StoredNetvars *ResolvedTick = nullptr;

					for (auto &tick : pPersistentData->m_PlayerRecords)
					{
						if (tick->simulationtime != pPersistentData->flSimulationTimePlayerWasShotOnClient)
							continue;
						ResolvedTick = tick;
						break;
					}

					if (!ResolvedTick)
					{
						Vector HeadPos = Entity->GetBonePosition(HITBOX_HEAD, Interfaces::Globals->curtime, false, false, nullptr);
						DecStr(bodyhiterrorstr, 36);
						Interfaces::DebugOverlay->AddTextOverlay(Vector(HeadPos.x, HeadPos.y, HeadPos.z + 1.0f), 1.0f, bodyhiterrorstr);
						EncStr(bodyhiterrorstr, 36);
						return;
					}

					pPersistentData->flSimulationTimeBloodWasSpawned = flSimulationTime;
					StoredNetvars tmpvars = StoredNetvars(Entity, pCPlayer, nullptr);
					tmpvars.absorigin = *Entity->GetAbsOrigin();
					tmpvars.absangles = Entity->GetAbsAngles();
					int IdealHitGroup = pPersistentData->iHitGroupPlayerWasShotOnServer;
					QAngle oRenderAngles = Entity->GetRenderAngles();
					QAngle originalanimeyeangles = *Entity->EyeAngles();

					Vector tracestart = *origin + (*vecDir * 64.0f);
					Vector traceexclude = *origin + (*vecDir * 1.0f); //don't count as valid if trace still hits slightly outside of the origin to get a more accurate angle
					CTraceFilterTargetSpecificEntity filter;
					filter.m_icollisionGroup = COLLISION_GROUP_NONE;
					filter.pTarget = Entity;

					Entity->SetLowerBodyYaw(ResolvedTick->lowerbodyyaw);
					Entity->SetDuckAmount(ResolvedTick->duckamount);
					CM_RestoreAnimations(ResolvedTick, Entity);

					if (tmpvars.absorigin != ResolvedTick->networkorigin)
					{
						Entity->SetAbsOrigin(ResolvedTick->networkorigin);
					}

					float flOriginalLastClientSideAnimationUpdateTime = Entity->GetLastClientSideAnimationUpdateTime();
					int iOriginalLastClientSideAnimationUpdateGlobalsFrameCount = Entity->GetLastClientSideAnimationUpdateGlobalsFrameCount();

					QAngle angNewEyeAngles = ResolvedTick->eyeangles;

					QAngle angFirstAbsAnglesFound;
					float flFirstEyeYawFound;
					float flFirstPoseParametersFound[24];
					C_AnimationLayer FirstAnimLayerFound[MAX_OVERLAYS];

					bool FoundRealHead = false; //Real head should be the exact spot the head is
					bool FoundGeneralHead = false; //General head is an area where the head will most likely generally be. It's not an exact spot
												   //Entity->DrawHitboxes(ColorRGBA(255, 255, 255, 255), 0.6f);
					matrix3x4_t tmpmatrix[MAXSTUDIOBONES];


					//for (float anginc = 0.0f; anginc <= 1080.0f; anginc += 4.0f)

					//do two passes of rotation, one going clockwise, the other counter clockwise. this is so that the pose parameters get tested in both directions

					bool firstrun = true;
					float flIncrementAmount = 5.0f;
					for (float anginc = 0.0f; firstrun ? anginc < 360.0f : anginc > 0.0f; firstrun ? anginc += flIncrementAmount : anginc -= flIncrementAmount)
					{
						//float body_yaw = anginc <= 360.0f ? 0.0f : anginc <= 720.0f ? -1.0f : 1.0f;
						//Entity->SetPoseParameterScaled(11, body_yaw);

						Entity->InvalidateBoneCache();
						LocalPlayer.Entity->SetLastOcclusionCheckFlags(0);
						LocalPlayer.Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);

						if (anginc != 0.0f)
						{

							Entity->SetLastClientSideAnimationUpdateTime(flOriginalLastClientSideAnimationUpdateTime);
							Entity->SetLastClientSideAnimationUpdateGlobalsFrameCount(iOriginalLastClientSideAnimationUpdateGlobalsFrameCount);
							angNewEyeAngles = QAngle(ResolvedTick->eyeangles.x, ClampYr(anginc), ResolvedTick->eyeangles.z);
							*Entity->EyeAngles() = angNewEyeAngles;
							Entity->UpdateClientSideAnimation();
						}
						else
						{
							float body_yaw = ResolvedTick->flPoseParameter[11] * (60.0f - -60.0f) + -60.0f;
							float body_pitch = ResolvedTick->flPoseParameter[12] * (90.0f - -90.0f) + -90.0f;
							float backtracked_eye_yaw = ClampYr(ClampYr(ResolvedTick->absangles.y) + body_yaw);
							angNewEyeAngles = QAngle(body_pitch, backtracked_eye_yaw, 0.0f);
							Entity->SetAbsAngles(QAngle(ResolvedTick->absangles.x, ResolvedTick->absangles.y, ResolvedTick->absangles.z));
						}

						Entity->SetupBones(tmpmatrix, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, /*Entity->GetSimulationTime()*/ Interfaces::Globals->curtime);

#ifndef USE_TRACERAY
						mstudiohitboxset_t *set = Interfaces::ModelInfoClient->GetStudioModel(Entity->GetModel())->pHitboxSet(Entity->GetHitboxSet());
						std::vector<CSphere>m_cSpheres;
						std::vector<COBB>m_cOBBs;

						for (int i = 0; i < set->numhitboxes; i++)
						{
							mstudiobbox_t *pbox = set->pHitbox(i);
							if (pbox->group == IdealHitGroup)
							{
								if (pbox->radius != -1.0f)
								{
									Vector vMin, vMax;
									VectorTransformZ(pbox->bbmin, tmpmatrix[pbox->bone], vMin);
									VectorTransformZ(pbox->bbmax, tmpmatrix[pbox->bone], vMax);
									SetupCapsule(vMin, vMax, pbox->radius, i, pbox->group, m_cSpheres);
								}
								else
								{
									m_cOBBs.push_back(COBB(pbox->bbmin, pbox->bbmax, &tmpmatrix[pbox->bone], i, pbox->group));
								}
							}
						}
#endif

						//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 0.6f, tmpmatrix);

						trace_t tr;
						Ray_t ray;
						ray.Init(tracestart, *origin);

#ifndef USE_TRACERAY
						TRACE_HITBOX(Entity, ray, tr, m_cSpheres, m_cOBBs);
#else
						Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);
#endif
						if (tr.m_pEnt == Entity && tr.hitgroup == IdealHitGroup)
						{
							//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 1.6f, tmpmatrix);
							if (!FoundGeneralHead)
							{
								flFirstEyeYawFound = ClampYr(anginc);
								Entity->CopyAnimLayers(FirstAnimLayerFound);
								Entity->CopyPoseParameters(flFirstPoseParametersFound);
								angFirstAbsAnglesFound = Entity->GetAbsAngles();
								FoundGeneralHead = true;
							}
							//Make sure if we trace slightly out of the impact origin, we don't still hit the same thing, otherwise the angle isn't correct
							ray.Init(tracestart, traceexclude);
#ifndef USE_TRACERAY
							TRACE_HITBOX(Entity, ray, tr, m_cSpheres, m_cOBBs);
#else
							Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);
#endif
							if (!tr.m_pEnt || tr.hitgroup != IdealHitGroup)
							{
								FoundRealHead = true;
								break;
							}
						}

						if (firstrun && (anginc + flIncrementAmount >= 360.0f))
							firstrun = false;
					}

					//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 0.6f, tmpmatrix);

					if (FoundRealHead || FoundGeneralHead)
					{
						if (!FoundRealHead)
						{
							//Set anims and poses and angles back to the values when we found the head

							Entity->InvalidateBoneCache();
							LocalPlayer.Entity->SetLastOcclusionCheckFlags(0);
							LocalPlayer.Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);

							float* flPose = (float*)((uintptr_t)Entity + m_flPoseParameter);
							memcpy((void*)flPose, &flFirstPoseParametersFound[0], (sizeof(float) * 24));

							int animcount = Entity->GetNumAnimOverlays();
							for (int i = 0; i < animcount; i++)
							{
								C_AnimationLayer* pLayer = Entity->GetAnimOverlay(i);
								*pLayer = FirstAnimLayerFound[i];
							}

							Entity->SetAbsAngles(QAngle(angFirstAbsAnglesFound.x, angFirstAbsAnglesFound.y, angFirstAbsAnglesFound.z));
							Entity->SetupBones(tmpmatrix, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, /*Entity->GetSimulationTime()*/ Interfaces::Globals->curtime);
						}

						if (DrawBloodResolveChk.Checked)
						{
							Vector HeadPos = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, tmpmatrix);
							DecStr(baimbloodresolveheadposstr, 25);
							Interfaces::DebugOverlay->AddTextOverlay(Vector(HeadPos.x, HeadPos.y, HeadPos.z + 1.0f), 1.5f, baimbloodresolveheadposstr);
							EncStr(baimbloodresolveheadposstr, 25);
							Interfaces::DebugOverlay->AddBoxOverlay(HeadPos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 0, 255, 0, 255, 1.5f);
						}

						//INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
						//float Latency = nci ? (nci->GetAvgLatency(FLOW_OUTGOING) + nci->GetAvgLatency(FLOW_INCOMING)) : 0.0f;

						if (BacktrackToBloodChk.Checked)
						{
							float cursimulation = flSimulationTime + GetLerpTime();
							StoredNetvars *NewTick = new StoredNetvars(Entity, pCPlayer, false);
							NewTick->absorigin = *Entity->GetAbsOrigin();
							//NewTick->tickcount = TIME_TO_TICKS(cursimulation - Latency); //ResolvedTick->tickcount;
							//if (!bTickIsValid(NewTick->tickcount))
							NewTick->tickcount = TIME_TO_TICKS(cursimulation);
							NewTick->ResolvedFromBloodSpawning = true;
							NewTick->FireAtPelvis = false;
							pCPlayer->LatestBloodTick = NewTick; //FIXME: MEMORY LEAK?
						}

						pPersistentData->angEyeAnglesResolvedFromBlood = angNewEyeAngles;

						//Store new blood record

						//FIXME: THIS LOOKS COMPLETELY FUCKED. FIX WHEN I'M NOT SO EXHAUSTED
						BloodRecord *nBloodRecord = new BloodRecord(Entity, &pCPlayer->CurrentNetvarsFromProxy, angNewEyeAngles.y);
						auto BloodRecords = &pPersistentData->m_BloodRecords;
						BloodRecords->push_front(nBloodRecord);

						//Don't store too many blood records
						if (BloodRecords->size() > 8)
						{
							delete BloodRecords->back(); //Free memory used by the record
							BloodRecords->pop_back(); //Delete the record
						}

						if (PredictFakeAnglesChk.Checked)
						{
							PredictRealYawFromBlood(pCPlayer, Entity, angNewEyeAngles.y);
						}
					}

					//Restore pre-modified values
					Entity->InvalidateBoneCache();
					Entity->SetLastClientSideAnimationUpdateTime(flOriginalLastClientSideAnimationUpdateTime);
					Entity->SetLastClientSideAnimationUpdateGlobalsFrameCount(iOriginalLastClientSideAnimationUpdateGlobalsFrameCount);
					//Entity->SetRenderAngles(oRenderAngles);
					Entity->SetGoalFeetYaw(tmpvars.GoalFeetYaw);
					Entity->SetCurrentFeetYaw(tmpvars.CurrentFeetYaw);
					Entity->SetEyeAngles(tmpvars.eyeangles);
					Entity->SetLowerBodyYaw(tmpvars.lowerbodyyaw);
					Entity->SetDuckAmount(tmpvars.duckamount);
					*Entity->EyeAngles() = originalanimeyeangles;
					CM_RestoreAnimations(&tmpvars, Entity);
					CM_RestoreNetvars(&tmpvars, Entity, &tmpvars.absorigin, &tmpvars.absangles);
					LocalPlayer.Entity->SetupBones(tmpmatrix, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, Interfaces::Globals->curtime);
				}
			}
			pPersistentData->bAwaitingBloodResolve = false;
		}
	}
}

void LagCompensation::OnPlayerImpactSpawned(CBaseEntity* const Entity, C_TEEffectDispatch* impact)
{
	if (BloodResolverChk.Checked)
	{
		CustomPlayer* pCPlayer = &AllPlayers[Entity->index];
		Personalize *pPersistentData = &pCPlayer->PersistentData;
		if (pPersistentData->bAwaitingBloodResolve)
		{
			if (pCPlayer->BaseEntity && !pCPlayer->IsLocalPlayer && Entity->GetAlive() && pPersistentData->ShouldResolve())
			{
				if (pPersistentData->m_PlayerRecords.size() == 0)
					return;

				//OnPlayerBloodSpawned seems to happen after the game events always, so no need for absolute value
				float flSimulationTime = pCPlayer->CurrentNetvarsFromProxy.simulationtime;
				if (flSimulationTime - pPersistentData->flSimulationTimePlayerWasShotOnClient < 0.5f || flSimulationTime - pPersistentData->flSimulationTimePlayerWasShot < 0.5f)
				{
					StoredNetvars *ResolvedTick = nullptr;

					for (auto &tick : pPersistentData->m_PlayerRecords)
					{
						if (tick->simulationtime < pPersistentData->flSimulationTimePlayerWasShotOnClient)
							break;
						if (tick->simulationtime != pPersistentData->flSimulationTimePlayerWasShotOnClient)
							continue;
						ResolvedTick = tick;
						break;
					}

					if (!ResolvedTick)
					{
						for (auto &tick : pPersistentData->m_PlayerRecords)
						{
							if (tick->simulationtime < pPersistentData->flSimulationTimePlayerWasShot)
								break;
							if (tick->simulationtime != pPersistentData->flSimulationTimePlayerWasShot)
								continue;
							ResolvedTick = tick;
							break;
						}
					}

					if (!ResolvedTick)
					{
						Vector HeadPos = Entity->GetBonePosition(HITBOX_HEAD, Interfaces::Globals->curtime, false, false, nullptr);
						DecStr(bodyhiterrorstr, 36);
						Interfaces::DebugOverlay->AddTextOverlay(Vector(HeadPos.x, HeadPos.y, HeadPos.z + 1.0f), 1.0f, bodyhiterrorstr);
						EncStr(bodyhiterrorstr, 36);
						return;
					}

					pPersistentData->flSimulationTimeBloodWasSpawned = flSimulationTime;
					int IdealHitGroup = pPersistentData->iHitGroupPlayerWasShotOnServer;
					int IdealHitBox = impact->m_EffectData.m_nHitBox;
					mstudiohitboxset_t *set = Interfaces::ModelInfoClient->GetStudioModel(Entity->GetModel())->pHitboxSet(Entity->GetHitboxSet());
					if (IdealHitBox >= set->numhitboxes)
						return;

					StoredNetvars tmpvars = StoredNetvars(Entity, pCPlayer, nullptr);
					tmpvars.absorigin = *Entity->GetAbsOrigin();
					tmpvars.absangles = Entity->GetAbsAngles();

					QAngle oRenderAngles = Entity->GetRenderAngles();
					QAngle originalanimeyeangles = *Entity->EyeAngles();

					Vector impactorigin = ResolvedTick->networkorigin + impact->m_EffectData.m_vOrigin;
					Vector shotorigin = ResolvedTick->networkorigin + impact->m_EffectData.m_vStart;
					Vector vecDirOutwardFromImpactTowardsShotOrigin = shotorigin - impactorigin;
					VectorNormalizeFast(vecDirOutwardFromImpactTowardsShotOrigin);

					Vector tracestart = impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * 1.25f);
					Vector traceend = impactorigin + ((vecDirOutwardFromImpactTowardsShotOrigin * -1.0f) * 1.25f);
					Vector traceexclude = impactorigin + (vecDirOutwardFromImpactTowardsShotOrigin * 128.0f);

					Entity->SetLowerBodyYaw(ResolvedTick->lowerbodyyaw);
					Entity->SetDuckAmount(ResolvedTick->duckamount);
					CM_RestoreAnimations(ResolvedTick, Entity);

					if (tmpvars.absorigin != ResolvedTick->networkorigin)
					{
						Entity->SetAbsOrigin(ResolvedTick->networkorigin);
					}

					float flOriginalLastClientSideAnimationUpdateTime = Entity->GetLastClientSideAnimationUpdateTime();
					int iOriginalLastClientSideAnimationUpdateGlobalsFrameCount = Entity->GetLastClientSideAnimationUpdateGlobalsFrameCount();

					QAngle angNewEyeAngles = ResolvedTick->eyeangles;

					QAngle angFirstAbsAnglesFound;
					float flFirstEyeYawFound;
					float flFirstPoseParametersFound[24];
					C_AnimationLayer FirstAnimLayerFound[MAX_OVERLAYS];

					bool FoundRealHeadPosition = false; //Real head should be the exact spot the head is
					bool FoundGeneralAreaOfHead = false; //General head is an area where the head will most likely generally be. It's not an exact spot

														 //Entity->DrawHitboxes(ColorRGBA(255, 255, 255, 255), 0.6f);
					matrix3x4_t tmpmatrix[MAXSTUDIOBONES];

					//First, restore to the exact original tick's angles and test against it
					Entity->InvalidateBoneCache();
					float body_yaw = ResolvedTick->flPoseParameter[11] * (60.0f - -60.0f) + -60.0f;
					float body_pitch = ResolvedTick->flPoseParameter[12] * (90.0f - -90.0f) + -90.0f;
					float backtracked_eye_yaw = ClampYr(ClampYr(ResolvedTick->absangles.y) + body_yaw);
					angNewEyeAngles = QAngle(body_pitch, backtracked_eye_yaw, 0.0f);
					Entity->SetAbsAngles(QAngle(ResolvedTick->absangles.x, ResolvedTick->absangles.y, ResolvedTick->absangles.z));

					//do two passes of rotation, one going clockwise, the other counter clockwise. this is so that the pose parameters get tested in both directions
					bool firstrun = true;
					float flIncrementAmount = 2.0f;
					for (float anginc = 0.0f; firstrun ? anginc < 360.0f : anginc > 0.0f; firstrun ? anginc += flIncrementAmount : anginc -= flIncrementAmount)
					{
						LocalPlayer.Entity->SetLastOcclusionCheckFlags(0);
						LocalPlayer.Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
						if (anginc != 0.0f)
						{
							LocalPlayer.Entity->SetLastOcclusionCheckFlags(0);
							LocalPlayer.Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
							Entity->InvalidateBoneCache();
							Entity->SetLastClientSideAnimationUpdateTime(flOriginalLastClientSideAnimationUpdateTime - Interfaces::Globals->interval_per_tick);
							Entity->SetLastClientSideAnimationUpdateGlobalsFrameCount(iOriginalLastClientSideAnimationUpdateGlobalsFrameCount - 1);
							angNewEyeAngles = QAngle(ResolvedTick->eyeangles.x, ClampYr(anginc), ResolvedTick->eyeangles.z);
							*Entity->EyeAngles() = angNewEyeAngles;
							Entity->UpdateClientSideAnimation();
						}

						Entity->SetupBones(tmpmatrix, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, /*Entity->GetSimulationTime()*/ Interfaces::Globals->curtime);

						std::vector<CSphere>m_cSpheres;
						std::vector<COBB>m_cOBBs;

						mstudiobbox_t *pbox = set->pHitbox(IdealHitBox);
						if (pbox->radius != -1.0f)
						{
							Vector vMin, vMax;
							VectorTransformZ(pbox->bbmin, tmpmatrix[pbox->bone], vMin);
							VectorTransformZ(pbox->bbmax, tmpmatrix[pbox->bone], vMax);
							SetupCapsule(vMin, vMax, pbox->radius, IdealHitBox, pbox->group, m_cSpheres);
						}
						else
						{
							m_cOBBs.push_back(COBB(pbox->bbmin, pbox->bbmax, &tmpmatrix[pbox->bone], IdealHitBox, pbox->group));
						}

						//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 0.6f, tmpmatrix);

						trace_t tr;
						Ray_t ray;
						ray.Init(tracestart, traceend);

						TRACE_HITBOX(Entity, ray, tr, m_cSpheres, m_cOBBs);

						if (tr.m_pEnt)
						{
							//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 1.6f, tmpmatrix);
							if (!FoundGeneralAreaOfHead)
							{
								flFirstEyeYawFound = ClampYr(anginc);
								Entity->CopyAnimLayers(FirstAnimLayerFound);
								Entity->CopyPoseParameters(flFirstPoseParametersFound);
								angFirstAbsAnglesFound = Entity->GetAbsAngles();
								FoundGeneralAreaOfHead = true;
							}
							//Make sure if we trace slightly out of the impact origin, we don't still hit the same thing, otherwise the angle isn't correct
							ray.Init(tracestart, traceexclude);

							TRACE_HITBOX(Entity, ray, tr, m_cSpheres, m_cOBBs);

							if (!tr.m_pEnt)
							{
								FoundRealHeadPosition = true;
								break;
							}
						}

						if (firstrun && (anginc + flIncrementAmount >= 360.0f))
							firstrun = false;
					}

					//Entity->DrawHitboxesFromCache(ColorRGBA(255, 0, 0, 255), 0.6f, tmpmatrix);

					if (FoundRealHeadPosition || FoundGeneralAreaOfHead)
					{
						if (!FoundRealHeadPosition)
						{
							//Set anims and poses and angles back to the values when we found the general area of the head

							Entity->InvalidateBoneCache();
							LocalPlayer.Entity->SetLastOcclusionCheckFlags(0);
							LocalPlayer.Entity->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);

							float* flPose = (float*)((uintptr_t)Entity + m_flPoseParameter);
							memcpy((void*)flPose, &flFirstPoseParametersFound[0], (sizeof(float) * 24));

							int animcount = Entity->GetNumAnimOverlays();
							for (int i = 0; i < animcount; i++)
							{
								C_AnimationLayer* pLayer = Entity->GetAnimOverlay(i);
								*pLayer = FirstAnimLayerFound[i];
							}

							Entity->SetAbsAngles(QAngle(angFirstAbsAnglesFound.x, angFirstAbsAnglesFound.y, angFirstAbsAnglesFound.z));
							Entity->SetupBones(tmpmatrix, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, /*Entity->GetSimulationTime()*/ Interfaces::Globals->curtime);
						}

						if (DrawBloodResolveChk.Checked)
						{
							//Draw head position for us to see
							Vector HeadPos = Entity->GetBonePositionCachedOnly(HITBOX_HEAD, tmpmatrix);
							DecStr(baimbloodresolveheadposstr, 25);
							Interfaces::DebugOverlay->AddTextOverlay(Vector(HeadPos.x, HeadPos.y, HeadPos.z + 1.0f), 1.5f, baimbloodresolveheadposstr);
							EncStr(baimbloodresolveheadposstr, 25);
							Interfaces::DebugOverlay->AddBoxOverlay(HeadPos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 0, 255, 0, 255, 1.5f);
						}

						//INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
						//float Latency = nci ? (nci->GetAvgLatency(FLOW_OUTGOING) + nci->GetAvgLatency(FLOW_INCOMING)) : 0.0f;

						if (BacktrackToBloodChk.Checked)
						{
							StoredNetvars *NewTick = new StoredNetvars(Entity, pCPlayer, false);
							if (bTickIsValid(ResolvedTick->tickcount))
							{
								NewTick->networkorigin = ResolvedTick->networkorigin;
								NewTick->absorigin = *Entity->GetAbsOrigin();
								NewTick->tickcount = ResolvedTick->tickcount;
								NewTick->eyeangles = angNewEyeAngles;
							}
							else
							{
								StoredNetvars *CurTick = pCPlayer->GetCurrentRecord();
								NewTick->tickcount = TIME_TO_TICKS(CurTick->simulationtime + GetLerpTime());
								NewTick->networkorigin = CurTick->networkorigin;
								NewTick->absorigin = CurTick->absorigin;
								NewTick->eyeangles = angNewEyeAngles;
							}
							NewTick->ResolvedFromBloodSpawning = true;
							NewTick->FireAtPelvis = false;
							pCPlayer->LatestBloodTick = NewTick;
						}

						pPersistentData->angEyeAnglesResolvedFromBlood = angNewEyeAngles;

						//Store new blood record

						BloodRecord *nBloodRecord = new BloodRecord(Entity, &pCPlayer->CurrentNetvarsFromProxy, angNewEyeAngles.y);
						auto BloodRecords = &pPersistentData->m_BloodRecords;
						BloodRecords->push_front(nBloodRecord);

						//Don't store too many blood records
						if (BloodRecords->size() > 8)
						{
							delete BloodRecords->back(); //Free memory used by the record
							BloodRecords->pop_back(); //Delete the record
						}

						if (PredictFakeAnglesChk.Checked)
						{
							PredictRealYawFromBlood(pCPlayer, Entity, angNewEyeAngles.y);
						}
					}

					//Restore pre-modified values
					Entity->InvalidateBoneCache();
					Entity->SetLastClientSideAnimationUpdateTime(flOriginalLastClientSideAnimationUpdateTime);
					Entity->SetLastClientSideAnimationUpdateGlobalsFrameCount(iOriginalLastClientSideAnimationUpdateGlobalsFrameCount);
					//Entity->SetRenderAngles(oRenderAngles);
					Entity->SetGoalFeetYaw(tmpvars.GoalFeetYaw);
					Entity->SetCurrentFeetYaw(tmpvars.CurrentFeetYaw);
					Entity->SetEyeAngles(tmpvars.eyeangles);
					Entity->SetLowerBodyYaw(tmpvars.lowerbodyyaw);
					Entity->SetDuckAmount(tmpvars.duckamount);
					CM_RestoreAnimations(&tmpvars, Entity);
					*Entity->EyeAngles() = originalanimeyeangles;
					CM_RestoreNetvars(&tmpvars, Entity, &tmpvars.absorigin, &tmpvars.absangles);
					LocalPlayer.Entity->SetupBones(nullptr, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, Interfaces::Globals->curtime);
				}
			}
			pPersistentData->bAwaitingBloodResolve = false;
		}
	}
}

void LagCompensation::CorrectFeetYaw(CBaseEntity* const Entity, float flClampedYaw, float flCorrect)
{
	VMP_BEGINMUTILATION("CFY")
		float newyaw = flClampedYaw;

	if (newyaw < 0.0f)
	{
		newyaw += flCorrect;
	}
	else if (newyaw > 0.0f)
	{
		newyaw -= flCorrect;
	}
#ifdef _DEBUG
	else
	{
		//FIXME
		//DebugBreak();
	}
#endif

	while (newyaw > 360.0f)
		newyaw -= 360.0f;

	while (newyaw < -360.0f)
		newyaw += 360.0f;

	//Feet yaw must always be 0-360 range
	if (newyaw < 0.0f)
		newyaw += 360.0f;

	Entity->SetCurrentFeetYaw(newyaw);
	Entity->SetGoalFeetYaw(newyaw);
	VMP_END
}