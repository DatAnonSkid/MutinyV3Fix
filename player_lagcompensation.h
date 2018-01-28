#pragma once
#include "ConVar.h"
#include "utlrbtree.h"
#include "utllinkedlist.h"
#include "CreateMove.h"
#include "utlmap.h"
#include "CBaseHandle.h"
extern float FAKE_LATENCY_AMOUNT;
extern float EST_FAKE_LATENCY_ON_SERVER;
#define LAG_COMPENSATION_EPS_SQR ( 0.1f * 0.1f )
// Allow 4 units of error ( about 1 / 8 bbox width )
#define LAG_COMPENSATION_ERROR_EPS_SQR ( 4.0f * 4.0f )
extern bool BackTracked;
extern ConVar *sv_unlag;
extern ConVar *sv_maxunlag;
extern ConVar *sv_lagflushbonecache;
extern ConVar *sv_lagpushticks;
extern ConVar *sv_client_min_interp_ratio;
extern ConVar *sv_client_max_interp_ratio;
extern ConVar *sv_maxupdaterate;
extern ConVar *sv_minupdaterate;
extern ConVar *cl_updaterate;
extern ConVar *cl_interp_ratio;
typedef CHandle2<CBaseEntity> EHANDLE;
class INetChannel;

struct CIncomingSequence
{
	CIncomingSequence::CIncomingSequence(int instate, int outstate, int seqnr, float time)
	{
		inreliablestate = instate;
		outreliablestate = outstate;
		sequencenr = seqnr;
		curtime = time;
	}
	int inreliablestate;
	int outreliablestate;
	int sequencenr;
	float curtime;
};

struct CSimulationData {
	CSimulationData() {}
	CSimulationData(CBaseEntity* ent)
	{
		m_pEntity = ent;
		velocity = ent->GetVelocity();
		networkorigin = ent->GetNetworkOrigin();
		m_nFlags = ent->GetFlags();
	}

	CBaseEntity* m_pEntity;
	Vector velocity;
	Vector networkorigin;
	int m_nFlags;
};

class LagCompensation
{
public:
	bool GetLagCompensationConVars();
	int GetEstimatedServerTickCount(float latency);
	float GetEstimatedServerTime();
	float GetNetworkLatency(int DIRECTION);
	float GetTrueNetworkLatency(int DIRECTION);
	void AdjustTickCountForCmd(CBaseEntity *const pPlayerFiredAt, bool bLocalPlayerIsFiring);
	float GetLerpTime() { return cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat(); }
	bool bTickIsValid(int tick);
	float GetServerDeltaTimeForTick(int tick, bool bTakeFakeLatencyIntoAccount);
	void ClearLagCompensationRecords(CustomPlayer*const pCPlayer);
	void MarkLagCompensationRecordsDormant(CustomPlayer*const pCPlayer);
	void ClearBloodRecords(CustomPlayer*const pCPlayer);
	void NetUpdatePlayer(CustomPlayer *const pCPlayer, CBaseEntity*const  Entity);
	void PostNetUpdatePlayer(CustomPlayer *const pCPlayer, CBaseEntity*const  Entity);
	void PreRenderStart();
	void PostRenderStart();
	bool CM_BacktrackPlayer(CustomPlayer *const pCPlayer, CBaseEntity*const pLocalEntity, CBaseEntity*const Entity, StoredNetvars *HistoryTick);
	void CM_BacktrackPlayers();
	void CM_RestorePlayers();
	void CorrectFeetYaw(CBaseEntity* const Entity, float flClampedYaw, float flCorrect);
	bool PredictRealYawFromBlood(CustomPlayer *const pCPlayer, CBaseEntity*const Entity, float yaw);
	void OnPlayerBloodSpawned(CBaseEntity* const Entity, Vector* const origin, Vector* const vecDir);
	void OnPlayerImpactSpawned(CBaseEntity* const Entity, C_TEEffectDispatch* impact);
	void UpdateIncomingSequences();
	void ClearIncomingSequences();
	void AddLatencyToNetchan(INetChannel* netchan, float Latency);
	void EnginePredictPlayer(CBaseEntity* Entity, int ticks, bool CalledFromCreateMove, bool WasJumping, bool Ducking, bool ForwardVelocity, CustomPlayer*const pCPlayer);
	bool CanFireAtLagCompensatedPlayer(CustomPlayer *const pCPlayer) 
	{
		return !pCPlayer->BaseEntity || !ResolverChk.Checked || !pCPlayer->PersistentData.ShouldResolve() || pCPlayer->bBacktracked || pCPlayer->TicksChoked == 0 || pCPlayer->FireAtPelvis || pCPlayer->PersistentData.ResolveYawMode > ResolveYawModes::FakeWalk /*|| !(pCPlayer->BaseEntity->GetFlags() & FL_ONGROUND)*/ || ((Interfaces::Globals->curtime - pCPlayer->TimeLowerBodyUpdated) >= LowerBodyWaitTimeBeforeFiringTxt.flValue);// || fabsf(pCPlayer->BaseEntity->GetPoseParameterUnscaled(11)) < 35.0f; //2.0
	}
#ifdef _DEBUG
	void OutputFeetYawInformation(CustomPlayer* const pCPlayer, CBaseEntity* const Entity);
	void OutputLowerBodyInformation(CustomPlayer* const pCPlayer, CBaseEntity* const Entity);
	void OutputServerYaws(float eyeyaw, float goalfeet, float curfeet);
#ifdef ANIMATIONS_DEBUG
	void OutputAnimations(CustomPlayer* const pCPlayer, CBaseEntity* const Entity);
#endif
#endif
private:
	void NewTickRecordReceived(CBaseEntity*const Entity, CustomPlayer*const pCPlayer, float flNewSimulationTime);
	void EvaluateNewTickRecord(CBaseEntity*const Entity, CustomPlayer*const pCPlayer, StoredNetvars*const record, bool PreviousFireAtPelvis);
	void FSN_BacktrackPlayer(CustomPlayer *const pCPlayer, CBaseEntity*const Entity);
	void FSN_RestoreNetvars(StoredNetvars *const src, CBaseEntity *const Entity, QAngle EyeAngles, float LowerBodyYaw);
	void CM_RestoreNetvars(StoredNetvars *const src, CBaseEntity *const Entity, Vector *const Origin, QAngle *const absangles);
	void CM_RestoreAnimations(StoredNetvars *const src, CBaseEntity *const Entity);
	void BacktrackToCurrentTickOrLagCompensate(CustomPlayer*const pCPlayer);
	bool PredictLowerBodyYaw(CustomPlayer *const pCPlayer, CBaseEntity*const Entity, float LowerBodyDelta);
	void InvalidateTicks(CustomPlayer* pCPlayer, int from, int to);
	void ValidateTickRecords(CustomPlayer *const pCPlayer, CBaseEntity*const Entity);
	void ClearImportantTicks(CustomPlayer *const pCPlayer, StoredNetvars *const record);
	void ClearPredictedAngles(CustomPlayer *const pCPlayer);
	void ValidateNetVarsWithProxy(CustomPlayer *const pCPlayer);
	QAngle GetFiringTickAngles(CustomPlayer *const pCPlayer, CBaseEntity*const Entity, StoredNetvars*const  vars);
	void JakesSimpleMathLagComp(CustomPlayer*const pCPlayer, int ticksChoked, StoredNetvars* CurrentTick, StoredNetvars *PreviousTick);
	void JakesSimpleMathLagCompZ(CustomPlayer*const pCPlayer, int ticksChoked, StoredNetvars* CurrentTick, StoredNetvars *PreviousTick);
	void PredictPlayerPosition(CustomPlayer*const pCPlayer, StoredNetvars* CurrentTick, StoredNetvars* PreviousTick);
	void SimulateMovement(CSimulationData& data, bool in_air);
	void GetIdealYaw(float& yaw, int correctedindex, CustomPlayer* pCPlayer, CBaseEntity* Entity, CBaseEntity* pLocalEntity);
	void GetIdealPitch(float& pitch, int correctedindex, CustomPlayer* pCPlayer, CBaseEntity* Entity, CBaseEntity* pLocalEntity);
	float ResolveCloseFakeAngles(CustomPlayer* pCPlayer);
	float ResolveFakeSpins(CustomPlayer* pCPlayer);
	float ResolveLinearFake(CustomPlayer* pCPlayer);
	float ResolveRandomFake(CustomPlayer* pCPlayer);
	float ResolveAtTargetFake(CustomPlayer* pCPlayer);
	float ResolveInverseAtTargetFake(CustomPlayer* pCPlayer);
	float ResolveStaticFake(CustomPlayer* pCPlayer);
	float ResolveStaticFakeDynamic(CustomPlayer* pCPlayer);
	float ResolveAverageLBYDelta(CustomPlayer* pCPlayer);
	float ResolveBloodReal(CustomPlayer* pCPlayer);
	float ResolveBloodBodyRealDelta(CustomPlayer* pCPlayer);
	float ResolveBloodEyeRealDelta(CustomPlayer* pCPlayer);
	float ResolveFakeWalk(CustomPlayer* pCPlayer);
	bool LagCompensationConvarsReceived = false;
};

extern LagCompensation gLagCompensation;