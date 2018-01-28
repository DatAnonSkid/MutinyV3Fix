#include "player_lagcompensation.h"
#include "Logging.h"

//Called from FrameStageNotify to set a player's netvars
void LagCompensation::FSN_RestoreNetvars(StoredNetvars *src, CBaseEntity *const Entity, QAngle EyeAngles, float LowerBodyYaw)
{
	Entity->SetMaxs(src->maxs);
	Entity->SetMins(src->mins);
	Entity->SetDuckAmount(src->duckamount);
	Entity->SetDuckSpeed(src->duckspeed);
	Entity->SetEyeAngles(EyeAngles);
	*Entity->EyeAngles() = EyeAngles;
	Entity->SetLowerBodyYaw(LowerBodyYaw);
}

//Restores various netvars and animations for a specific entity
void LagCompensation::CM_RestoreNetvars(StoredNetvars *const src, CBaseEntity *const Entity, Vector *const Origin, QAngle *const absangles)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("CM_RestoreNetvars\n");
#endif
	//Restore netvars
#if 0
	if (src->ResolvedFromBloodSpawning)
	{
		Entity->SetPoseParameterScaled(11, src->flPoseParameter[11]);
		Entity->SetAbsAngles(src->absangles);
	}
#endif
	//Entity->SetAngleRotation(src->angles); //FIXME: get this in render start?
	//Entity->SetViewOffset(src->viewoffset);
	//Entity->SetVelocity(src->velocity);
	//Entity->SetFallVelocity(src->fallvelocity);
	Entity->SetFlags(src->flags);
	//Entity->SetSimulationTime(src->simulationtime);
	//Entity->SetBaseVelocity(src->basevelocity);
	Entity->SetMaxs(src->maxs);
	Entity->SetMins(src->mins);
	Entity->SetLowerBodyYaw(src->lowerbodyyaw); //fixme?
	Entity->SetDuckAmount(src->duckamount);
	Entity->SetDuckSpeed(src->duckspeed);

	//Entity->SetVelocityModifier(src->velocitymodifier);
	//Entity->SetCycle(src->cycle);
	//Entity->SetSequence(src->sequence);
	//Entity->SetPlaybackRate(src->playbackrate);
	//Entity->SetPunch(src->aimpunch);
	//Entity->SetAngleRotation(src->angles);
	//Entity->SetGroundEntity(src->groundentity); //could possibly crash if entity is gone
	//Entity->SetEyeAngles(EyeAngles);
	//Entity->SetVecLadderNormal(src->laddernormal);
	//Entity->SetDucked(src->isducked);
	//Entity->SetDucking(src->isducking);
	//Entity->SetInDuckJump(src->isinduckjump);
	//Entity->SetDuckTimeMsecs(src->ducktimemsecs);
	//Entity->SetJumpTimeMsecs(src->jumptimemsecs);
	//Entity->UpdateClientSideAnimation();
	//Entity->SetLaggedMovement(src->laggedmovement);
	//Entity->SetAnimTime(src->animtime);
	//Entity->SetNetworkOrigin(src->networkorigin);//(src->networkorigin);
	//Entity->SetOrigin(src->origin);// src->networkorigin);// src->origin);
	//Entity->SetAbsOrigin(src->origin);
	//if (Absangles.y < 0)
	//	Absangles.y += 360.0f;
	//Entity->SetRenderAngles(EyeAngles);
	//Entity->SetAbsAngles(Absangles);
	//Entity->SetNetworkOrigin(src->networkorigin);

#if 1
	//Avoid using absorigin, it causes massive issues with bones
	//	Entity->SetOrigin(*Origin);
	Entity->SetAbsOrigin(*Origin);
	//	Entity->InvalidatePhysicsRecursive(POSITION_CHANGED);
#else
	Vector NewOrigin = *Origin;
	if (Entity->GetOrigin() != NewOrigin && NewOrigin != vecZero)
	{
		Entity->SetOrigin(NewOrigin);
		Entity->InvalidatePhysicsRecursive(POSITION_CHANGED);
	}
#endif
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("FinishRestoreNetvars\n");
#endif
}

void LagCompensation::CM_RestoreAnimations(StoredNetvars *const src, CBaseEntity *const Entity)
{
	//Restore pose parameters
	//if (!src->ResolvedFromBloodSpawning)
	{
		float* flPose = (float*)((uintptr_t)Entity + m_flPoseParameter);
		memcpy((void*)flPose, &src->flPoseParameter[0], (sizeof(float) * 24));
		//flPose[11] = body_yaw;
		//flPose[12] = body_pitch;
	}

	//Restore animations
	int animcount = Entity->GetNumAnimOverlays();
	for (int i = 0; i < animcount; i++)
	{
		C_AnimationLayer* pLayer = Entity->GetAnimOverlay(i);
#if 1
		* pLayer = src->AnimLayer[i];
#else
		C_AnimationLayer* pStoredLayer = &src->AnimLayer[i];
		if (pLayer->_m_flCycle != pStoredLayer->_m_flCycle)
			pLayer->SetCycle(pStoredLayer->_m_flCycle);
		if (pLayer->m_flWeight != pStoredLayer->m_flWeight)
			pLayer->SetWeight(pStoredLayer->m_flWeight);
		if (pLayer->m_nOrder != pStoredLayer->m_nOrder)
			pLayer->SetOrder(pStoredLayer->m_nOrder);
		if (pLayer->_m_nSequence != pStoredLayer->_m_nSequence)
			pLayer->SetSequence(pStoredLayer->_m_nSequence);
		pLayer->m_nInvalidatePhysicsBits = pStoredLayer->m_nInvalidatePhysicsBits;
		pLayer->m_flLayerAnimtime = pStoredLayer->m_flLayerAnimtime;
		pLayer->m_flPrevCycle = pStoredLayer->m_flPrevCycle;
		pLayer->m_flWeightDeltaRate = pStoredLayer->m_flWeightDeltaRate;
		pLayer->m_flBlendIn = pStoredLayer->m_flBlendIn;
		pLayer->m_flBlendOut = pStoredLayer->m_flBlendOut;
		pLayer->m_flLayerFadeOuttime = pStoredLayer->m_flLayerFadeOuttime;
		pLayer->m_pOwner = pStoredLayer->m_pOwner;
		pLayer->_m_flPlaybackRate = pStoredLayer->_m_flPlaybackRate;
#endif
	}
}