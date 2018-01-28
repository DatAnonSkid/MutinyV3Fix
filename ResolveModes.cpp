#include "player_lagcompensation.h"
#include "LocalPlayer.h"
#include "VMProtectDefs.h"

float LagCompensation::ResolveCloseFakeAngles(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RCFA")
		float yaw;
	StoredNetvars *CurrentNetVars = &pCPlayer->CurrentNetvarsFromProxy;

	//MAX_NEAR_BODY_YAW_RESOLVER_MISSES is the max
	switch (pCPlayer->PersistentData.correctedclosefakeindex)
	{
	default:
	case 0:
		yaw = CurrentNetVars->lowerbodyyaw;
		break;
	case 1:
		yaw = CurrentNetVars->lowerbodyyaw + 17.5f;
		break;
	case 2:
		yaw = CurrentNetVars->lowerbodyyaw - 17.5f;
		break;
	case 3:
		yaw = CurrentNetVars->lowerbodyyaw + 35.0f;
		break;
	case 4:
		yaw = CurrentNetVars->lowerbodyyaw - 35.0f;
		break;
	case 5:
		return ResolveLinearFake(pCPlayer);
	case 6:
		return ResolveStaticFakeDynamic(pCPlayer);
	}

	ClampY(yaw);

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::CloseFake;

	return yaw;
	VMP_END
}

//Also works very well for static fake angles
float LagCompensation::ResolveFakeSpins(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RFS")
		float yaw;
	StoredNetvars *CurrentNetVars = &pCPlayer->CurrentNetvarsFromProxy;

	yaw = ClampYr(ClampYr(CurrentNetVars->eyeangles.y) - pCPlayer->LowerBodyDelta);

	if (BruteForceAnglesAfterXMissesTxt.iValue > 0 && pCPlayer->PersistentData.correctedfakespinindex > (BruteForceAnglesAfterXMissesTxt.iValue - 1))
	{
		if (pCPlayer->PersistentData.correctedfakespinindex % 2 == 0)
			yaw += RandomFloat(-105.0f, 105.0f);
		else
			yaw += RandomFloat(165.0f, 205.0f);

		NormalizeFloat(yaw);
	}

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::FakeSpins;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveLinearFake(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RLF")
		float yaw;
	StoredNetvars *CurrentNetVars = &pCPlayer->CurrentNetvarsFromProxy; //pCPlayer->Personalize.m_PlayerRecords.front();

	yaw = ClampYr(CurrentNetVars->lowerbodyyaw + pCPlayer->LowerBodyDelta);// +(pCPlayer->LowerBodyDelta * 0.5f);

	if (BruteForceAnglesAfterXMissesTxt.iValue > 0 && pCPlayer->PersistentData.correctedlinearindex > (BruteForceAnglesAfterXMissesTxt.iValue - 1))
	{
		if (pCPlayer->PersistentData.correctedlinearindex % 2 == 0)
			yaw += RandomFloat(155.0f, 205.0f);
		else
			yaw += RandomFloat(155.0f, 180.0f);

		NormalizeFloat(yaw);
	}

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::LinearFake;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveRandomFake(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RRF")
		float yaw;
	StoredNetvars *CurrentNetVars = &pCPlayer->CurrentNetvarsFromProxy;

	yaw = ClampYr(CurrentNetVars->eyeangles.y + (pCPlayer->LowerBodyEyeDelta * 0.5f)); //taps ballerina most of the time

	if (pCPlayer->PersistentData.correctedrandomindex > 0)
	{
		if (pCPlayer->PersistentData.correctedrandomindex % 2 == 0)
			yaw += RandomFloat(155.0f, 205.0f);
		else
			yaw += RandomFloat(155.0f, 205.0f);

		NormalizeFloat(yaw);
	}

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::RandomFake;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveAtTargetFake(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RATF")
		float yaw;
	float dt;
	StoredNetvars *CurrentNetVars = pCPlayer->GetCurrentRecord();

	QAngle AngleToUs = CalcAngle(CurrentNetVars->networkorigin, LocalPlayer.Entity->GetNetworkOrigin());

	if (pCPlayer->PersistentData.ShotsMissed < 2)
	{
		yaw = AngleToUs.y;
	}
	else if (pCPlayer->PersistentData.ShotsMissed % 2 == 0)
	{
		yaw = ClampYr(AngleToUs.y + 20.0f);

		//FIXME: this still isn't right
		dt = ClampYr(yaw - CurrentNetVars->lowerbodyyaw);
		if (fabsf(dt) > 35.0f)
		{
			yaw = ClampYr(CurrentNetVars->lowerbodyyaw + 20.0f);
		}
	}
	else
	{
		yaw = ClampYr(AngleToUs.y - 20.0f);

		//FIXME: this still isn't right
		dt = ClampYr(yaw - CurrentNetVars->lowerbodyyaw);
		if (fabsf(dt) > 35.0f)
		{
			yaw = ClampYr(CurrentNetVars->lowerbodyyaw - 20.0f);
		}
	}
	if (yaw < 0.0f)
		yaw += 360.0f;
	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::AtTarget;
	return yaw;
	VMP_END
}

float LagCompensation::ResolveInverseAtTargetFake(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RIATF")
		float yaw;
	float dt;
	StoredNetvars *CurrentNetVars = pCPlayer->GetCurrentRecord();

	QAngle AngleFromUsToTarget = CalcAngle(LocalPlayer.Entity->GetOrigin(), CurrentNetVars->origin);

	if (pCPlayer->PersistentData.ShotsMissed < 1)
	{
		yaw = AngleFromUsToTarget.y;
	}
	else if (pCPlayer->PersistentData.ShotsMissed % 2 == 0)
	{
		yaw = ClampYr(AngleFromUsToTarget.y + 20.0f);

		//FIXME: this still isn't right
		dt = ClampYr(yaw - CurrentNetVars->lowerbodyyaw);
		if (fabsf(dt) > 35.0f)
		{
			yaw = ClampYr(CurrentNetVars->lowerbodyyaw + 20.0f);
		}
	}
	else
	{
		yaw = ClampYr(AngleFromUsToTarget.y - 20.0f);

		//FIXME: this still isn't right
		dt = ClampYr(yaw - CurrentNetVars->lowerbodyyaw);
		if (fabsf(dt) > 35.0f)
		{
			yaw = ClampYr(CurrentNetVars->lowerbodyyaw - 20.0f);
		}
	}

	if (yaw < 0.0f)
		yaw += 360.0f;
	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::InverseAtTarget;
	return yaw;
	VMP_END
}

float LagCompensation::ResolveStaticFake(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RSF")
		float yaw;
	StoredNetvars *CurrentNetVars = &pCPlayer->CurrentNetvarsFromProxy;

	yaw = CurrentNetVars->lowerbodyyaw;

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::StaticFake;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveStaticFakeDynamic(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RSFD")
		float yaw;
	StoredNetvars *CurrentNetVars = pCPlayer->PersistentData.m_PlayerRecords.front();
	//StoredLowerBodyYaw *CurrentLowerBodyVars = pCPlayer->Personalize.m_PlayerLowerBodyRecords.front();

	yaw = ClampYr(CurrentNetVars->eyeangles.y + pCPlayer->LowerBodyEyeDelta);

	if (BruteForceAnglesAfterXMissesTxt.iValue > 0 && pCPlayer->PersistentData.correctedfakedynamicindex > (BruteForceAnglesAfterXMissesTxt.iValue - 1))
	{
		if (pCPlayer->PersistentData.correctedfakedynamicindex % 2 == 0)
			yaw += RandomFloat(155.0f, 205.0f);
		else
			yaw += RandomFloat(155.0f, 205.0f);

		NormalizeFloat(yaw);
	}

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::StaticFakeDynamic;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveAverageLBYDelta(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RALBYD")
		float yaw;
	StoredNetvars *CurrentNetVars = pCPlayer->PersistentData.m_PlayerRecords.front();
	//StoredLowerBodyYaw *CurrentLowerBodyVars = pCPlayer->Personalize.m_PlayerLowerBodyRecords.front();

	yaw = pCPlayer->PredictedAverageFakeAngle + (45.0f * pCPlayer->PersistentData.ShotsMissed);

	if (BruteForceAnglesAfterXMissesTxt.iValue > 0 && pCPlayer->PersistentData.correctedavglbyindex > (BruteForceAnglesAfterXMissesTxt.iValue - 1))
	{
		if (pCPlayer->PersistentData.correctedavglbyindex % 2 == 0)
			yaw += RandomFloat(155.0f, 205.0f);
		else
			yaw += RandomFloat(155.0f, 205.0f);

		NormalizeFloat(yaw);
	}

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::AverageLBYDelta;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveBloodReal(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RBR")
		float yaw;

	yaw = pCPlayer->LastRealYaw;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::BloodReal;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveBloodBodyRealDelta(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RBBRD")
		float yaw;
	StoredNetvars *CurrentNetVars = pCPlayer->PersistentData.m_PlayerRecords.front();

	yaw = ClampYr(CurrentNetVars->lowerbodyyaw + pCPlayer->LastBodyRealDelta);

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::BloodBodyRealDelta;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveBloodEyeRealDelta(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RBERD")
		float yaw;
	StoredNetvars *CurrentNetVars = pCPlayer->PersistentData.m_PlayerRecords.front();

	yaw = ClampYr(CurrentNetVars->eyeangles.y + pCPlayer->LastEyeRealDelta);

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::BloodEyeRealDelta;

	return yaw;
	VMP_END
}

float LagCompensation::ResolveFakeWalk(CustomPlayer* pCPlayer)
{
	VMP_BEGINMUTILATION("RBERD")
		float yaw;
	int correctedinex = pCPlayer->PersistentData.correctedfakewalkindex;

	if (correctedinex < 2)
	{
		yaw = ClampYr(pCPlayer->DirectionWhenFirstStartedMoving.y + 180.0f);
	}
	else if (correctedinex < 4)
	{
		QAngle dir;
		VectorAngles(pCPlayer->CurrentNetvarsFromProxy.velocity, dir);
		yaw = ClampYr(dir.y + 180.0f);
	}
	else if (correctedinex < 6)
	{
		yaw = pCPlayer->DirectionWhenFirstStartedMoving.y;
	}
	else
	{
		QAngle dir;
		VectorAngles(pCPlayer->CurrentNetvarsFromProxy.velocity, dir);
		yaw = dir.y;
	}

	if (yaw < 0.0f)
		yaw += 360.0f;

	pCPlayer->PersistentData.LastResolveModeUsed = ResolveYawModes::FakeWalk;
	return yaw;
	VMP_END
}

