#ifndef GAMEMEMORY_H
#define GAMEMEMORY_H
#pragma once

#include "Offsets.h"
//#include "cx_strenc.h"
#include "misc.h"
#include "CSGO_HX.h"
#include "PlayerList.h"
#include "BaseEntity.h"
#include "EncryptString.h"
#include "Animation.h"
#include <map>
#include "Math.h"

extern int ActualNumPlayers;

#define gamecurtime ReadFloat((uintptr_t)&Interfaces::Globals->curtime)

#define TICK_INTERVAL			(ReadFloat((uintptr_t)&Interfaces::Globals->interval_per_tick))


#define TIME_TO_TICKS( dt )		( (int)( 0.5 + (double)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#define LIFE_ALIVE 0

enum TEAMS : DWORD {
	TEAM_NONE = 0,
	TEAM_GOTV,
	TEAM_T,
	TEAM_CT
};

#define INVALID_PLAYER -1

struct LocalPlayerStoredNetvars
{
	Vector origin;
	Vector networkorigin;
	Vector viewoffset;
	Vector LocalEyePos;
};

struct SavedHitboxPos
{
	bool bsaved;
	Vector origin;
	float damage;
	int corrected_body_yaw_index;
};

int GetGlobalTickCount();
float GetGlobalCurTime();
int GetExactTick(float SimulationTime);
int GetTicksChoked(CustomPlayer* pCPlayer);

struct StoredNetvars
{
	StoredNetvars() {}
	~StoredNetvars() {}
	StoredNetvars(CBaseEntity* Entity, CustomPlayer* pCPlayer, StoredNetvars *ForcedVars)
	{
		//Remember the current engine tick for storing pose parameters and animations
		TickReceivedNetUpdate = GetGlobalTickCount();
		ReceivedPostNetUpdateVars = false;
		ResolvedFromBloodSpawning = false;
		bCachedBones = false;
		iTickCachedBones = 0;
		corrected_body_yaw_index = -1;
		bHeadIsVisible = false;
		bBodyIsVisible = false;
		memset(&BestHitboxPos, 0, sizeof(SavedHitboxPos) * HITBOX_MAX);

		dormant = false;

		//Store all the netvars
		if (ForcedVars)
		{
			eyeangles = ForcedVars->eyeangles;
			eyeyaw = eyeangles.y;
			absangles = Entity->GetAbsAngles(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			angles = Entity->GetAngleRotation(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			aimpunch = ForcedVars->aimpunch;
			networkorigin = Entity->GetNetworkOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			origin = Entity->GetOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			//absorigin = *Entity->GetAbsOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			velocity = Entity->GetVelocity(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			basevelocity = Entity->GetBaseVelocity();
			viewoffset = ForcedVars->viewoffset;
			mins = ForcedVars->mins;
			maxs = ForcedVars->maxs;
			lowerbodyyaw = ForcedVars->lowerbodyyaw;
			fallvelocity = ForcedVars->fallvelocity;
			velocitymodifier = ForcedVars->velocitymodifier;
			simulationtime = ForcedVars->simulationtime;
			duckspeed = ForcedVars->duckspeed;
			duckamount = ForcedVars->duckamount;
			flags = ForcedVars->flags;
			groundentity = ForcedVars->groundentity;
		}
		else
		{
			eyeangles = Entity->GetEyeAngles();
			eyeyaw = eyeangles.y;
			absangles = Entity->GetAbsAngles(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			angles = Entity->GetAngleRotation(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			aimpunch = Entity->GetPunch();
			networkorigin = Entity->GetNetworkOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			origin = Entity->GetOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			//absorigin = *Entity->GetAbsOrigin(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			velocity = Entity->GetVelocity(); //Note: not valid from FRAME_NET_POSTDATAUPDATE_START
			basevelocity = Entity->GetBaseVelocity();
			viewoffset = Entity->GetViewOffset();
			mins = Entity->GetMins();
			maxs = Entity->GetMaxs();
			lowerbodyyaw = Entity->GetLowerBodyYaw();
			fallvelocity = Entity->GetFallVelocity();
			velocitymodifier = Entity->GetVelocityModifier();
			simulationtime = Entity->GetSimulationTime();
			duckspeed = Entity->GetDuckSpeed();
			duckamount = Entity->GetDuckAmount();
			flags = Entity->GetFlags();
			groundentity = Entity->GetGroundEntity();
		}
		tickcount = GetExactTick(simulationtime);
		tickschoked = pCPlayer ? GetTicksChoked(pCPlayer) : 0;
		IsValidForBacktracking = true;
		FiredBullet = false;

		//Note: the pose parameters and animations are not correct unless retreieved after FRAME_RENDER_START
		float* pose = (float*)((uintptr_t)Entity + m_flPoseParameter);
		memcpy(&flPoseParameter, pose, sizeof(float) * 24);

		int cnt = Entity->GetNumAnimOverlays();
		for (int i = 0; i < cnt; i++)
		{
			AnimLayer[i] = *Entity->GetAnimOverlay(i);
		}
		GoalFeetYaw = Entity->GetGoalFeetYaw();
		CurrentFeetYaw = Entity->GetCurrentFeetYaw();
		GetLocalPlayerVars();
	}

	bool operator==(const StoredNetvars& others) {
		return (simulationtime == others.simulationtime);
	}

	void GetLocalPlayerVars();

	float simulationtime;
	bool dormant;
	QAngle eyeangles;
	QAngle idealeyeangles;
	float eyeyaw; //Unmodified yaw
	QAngle absangles;
	QAngle angles;
	QAngle aimpunch;
	Vector absorigin;
	Vector origin;
	Vector networkorigin;
	//Vector headorigin;
	Vector velocity;
	Vector basevelocity;
	Vector viewoffset;
	Vector mins;
	Vector maxs;
	//Vector laddernormal;
	float lowerbodyyaw;
	float fallvelocity;
	float velocitymodifier;
	//float cycle;
	//float playbackrate;
	//int sequence;
	int flags;
	int tickcount;
	int tickschoked;
	int TickReceivedNetUpdate;
	bool IsValidForBacktracking;
	bool PitchingUp;
	bool LowerBodyUpdated;
	bool bSpinbotting;
	bool FiredBullet;
	bool IsRealTick;
	bool ReceivedPostNetUpdateVars;
	bool ResolvedFromBloodSpawning;
	CBaseEntity *groundentity; //could possibly crash if entity is gone
	//bool isducked;
	//bool isducking;
	//bool isinduckjump;
	//int ducktimemsecs;
	float duckamount;
	float duckspeed;
	//float laggedmovement;
	//float animtime;
	float flPoseParameter[24];
	float GoalFeetYaw;
	float CurrentFeetYaw;
	//std::array<float, 24>flPosePoseParameter;
	C_AnimationLayer AnimLayer[MAX_OVERLAYS];
	LocalPlayerStoredNetvars LocalVars;
	bool FireAtPelvis;
	bool bCachedBones;
	int iTickCachedBones;
	int iReadableBones;
	int iWritableBones;
	int iCachedBonesCount;
	matrix3x4_t CachedBoneMatrixes[MAXSTUDIOBONES];
	bool autoBonned;
	bool foundBone;
	Vector foundPos;
	float foundDamage;
	bool foundHead;
	float pelvisDamage;
	Vector bestPelvisPos;
	float headDamage;
	Vector bestHeadPos;

	//Used by aimbot backtracking
	int corrected_body_yaw_index;
	bool bRanHitScan;
	bool bHeadIsVisible;
	bool bBodyIsVisible;
	SavedHitboxPos BestHitboxPos[HITBOX_MAX];
};

struct StoredLowerBodyYaw
{
	StoredLowerBodyYaw() {}
	~StoredLowerBodyYaw() {}
	StoredLowerBodyYaw(CBaseEntity* Entity, StoredNetvars* CurrentNetvars, StoredLowerBodyYaw* PreviousLowerBodyRecord)
	{
		//TODO: Add IsMoving for detecting spinning lower body yaws
		networkorigin = Entity->GetNetworkOrigin();
		eyeangles = CurrentNetvars->eyeangles;
		lowerbodyyaw = CurrentNetvars->lowerbodyyaw;
		lowerbodyeyedelta = ClampYr(lowerbodyyaw - ClampYr(eyeangles.y));
	
		if (PreviousLowerBodyRecord)
		{
			lowerbodydelta = ClampYr(lowerbodyyaw - PreviousLowerBodyRecord->lowerbodyyaw);
			deltabetweenlastlowerbodyeyedelta = ClampYr(lowerbodyeyedelta - PreviousLowerBodyRecord->lowerbodyeyedelta);
			HasPreviousRecord = TRUE;
		}
		else
		{
			lowerbodyeyedelta = 0.0f;
			lowerbodydelta = 0.0f;
			deltabetweenlastlowerbodyeyedelta = 0.0f;
			HasPreviousRecord = FALSE;
		}

		tickcount = GetExactTick(CurrentNetvars->simulationtime);
		TickReceivedNetUpdate = GetGlobalTickCount();
	}
	Vector networkorigin;
	QAngle eyeangles;
	float lowerbodyyaw;
	float lowerbodyeyedelta;
	float lowerbodydelta;
	float deltabetweenlastlowerbodyeyedelta;
	BOOL HasPreviousRecord;
	int tickcount;
	int TickReceivedNetUpdate;
};

struct CustomPlayer
{
	BOOLEAN Connected;
	bool IsLocalPlayer;
	unsigned char Dormant; //Uncomment when needed, set in GetStreamedPlayers_And_RunPerPlayerHacks
	int Index;
	CBaseEntity* BaseEntity;
	//DWORD ObserverTarget; //Spectating target
	//wchar_t Name[32];
	int LifeState;
	//int Health;
	//TEAMS Team;
	bool IsTarget;
	bool IsSpectating;
	BOOLEAN Active;
	char steamid[33];
	bool bBacktracked;
	bool bNetvarsModified; //Have we modified the netvars and need to restore when finished?
	//bool DataUpdated;
	int TickReceivedLowerBodyUpdate;

	float flLastTimeRanAutoBone;
	float flTotalTimeTriedAimingAtHead; //The total time we have tried to aim at the head but couldn't hit it due to hitchance
	float flTotalTimeBodyWasVisible; //The total time the body was visible
	//float flTimeBodyWasFirstVisible;
	//float flTimeHeadWasVisibleButNotHittable;

	int iOriginalInterpolationEntries;

	Vector vecLastFootStepOrigin;
	int iLastFootStepTick;

	bool bReceivedMuzzleFlash;
	float flSimulationTimeReceivedMuzzleFlash;
	QAngle angEyeAnglesMuzzleFlash;

	bool bAlreadyRanHistoryScanThisTick; //Did we already run history hitscan on this player this tick? Prevents infinite loops in case the hitscan couldn't trace
	bool bIsHistoryBacktracked;

	StoredNetvars CurrentNetvarsFromProxy;
	StoredNetvars TempNetVars;
	StoredNetvars *LatestLowerBodyUpdate;
	StoredNetvars *LatestFiringTick;
	StoredNetvars *LatestRealTick;
	StoredNetvars *LatestPitchUpTick;
	StoredNetvars *LatestReloadingTick;
	StoredNetvars *LatestBloodTick;
	StoredNetvars *pBacktrackedTick;
	StoredNetvars *pLastBacktrackedTick;
	StoredNetvars *GetBestBacktrackTick()
	{
		if (!PersistentData.ShouldResolve())
			return nullptr;
		if (LatestLowerBodyUpdate)
			return LatestLowerBodyUpdate;
		if (LatestRealTick)
			return LatestRealTick;
		if (LatestReloadingTick)
			return LatestReloadingTick;
		if (LatestPitchUpTick && ValveResolverChk.Checked)
			return LatestPitchUpTick;
		if (FireWhenEnemiesShootChk.Checked)
		{
			if (LatestFiringTick)
			{
				//if (LatestFiringTick->tickschoked == 0)
				return LatestFiringTick;
			}
		}
		return LatestBloodTick;
	}
	StoredNetvars *GetBestHistoryTick(); //Returns the last known tick in history that was hittable for this player
	bool CloseFakeAngle;
	//bool EyeAnglesUpdated;
	bool LowerBodyCanBeDeltaed;
	bool LowerBodyUpdated;
	float TimeLowerBodyUpdated;
	float flLastLowerBodyYawBreakerDetectTime;
	//float m_flOldAnimTime;
	//float m_flOldSimulationTime;
	//float flAnimTime;
	float flLastLowerBodyYaw;
	bool bWasHistoryBacktrackedWhenWeShotAtThisPlayer;
	bool bWasMovingWhenWeLastShotAtThem;
	float flLastTimeWeShotAtThisPlayer;
	QAngle angIdealEyeAnglesWhenWeShotAtThisPlayer;
	bool ResolverFiredAtShootingAngles;
	bool FireAtPelvis;
	bool IsMoving;
	bool bCouldPredictLowerBodyYaw;
	float LowerBodyDelta; //delta of current lower body yaw and last lower body yaw
	float LowerBodyEyeDelta; //delta of current lower body yaw and eye angles
	float PredictedLinearFakeAngle;
	float PredictedRandomFakeAngle;
	float PredictedStaticFakeAngle;
	float PredictedAverageFakeAngle;
	float LastBodyRealDelta;
	float LastEyeRealDelta;
	float LastRealEyeDelta;
	float LastRealYaw;
	float SimulationTimeReceivedNewEyeAngleX;
	float SimulationTimeReceivedNewEyeAngleY;
	int BulletsLeft;
	int iBacktrackTickCount;
	int TicksChoked;
	//bool LagCompensatedThisTick; //Unused currently
	bool IsBreakingLagCompensation;
	QAngle DirectionWhenFirstStartedMoving;
	float flTimeWhenFirstStartedMoving;
	CBaseCombatWeapon *CurrentWeapon;
	Personalize PersistentData;
	bool bHookedBaseEntity;
	HookedEntity* HookedBaseEntity;
	bool bAlreadyCachedBonesThisTick;
	bool bUsedCachedBonesThisTick;
	bool bHasCachedBones;
	int iBackupGameCachedBonesCount;
	int iBackupGameReadableBones;
	int iBackupGameWritableBones;
	matrix3x4a_t* pGameCachedBoneMatrix;
	__declspec (align(16)) matrix3x4_t BackupGameCachedBoneMatrixes[MAXSTUDIOBONES]; //A backup of the game's cached bone matrixes. Do not use for reading. Instead use GetBoneAccessor()
	__declspec (align(16)) matrix3x4_t CachedBoneMatrixes[MAXSTUDIOBONES]; //Bone matrixes used for rendering things since SetupBones crashes CSGO inside endscene and we can't use current record since the game's multithreaded

	int ClientSideAnimationFlags;
	bool HadClientAnimSequenceCycle;

	//Predicted vars, temp
	Vector pred_LastPos;
	Vector pred_HowClose;
	Vector pred_LastPredictedVelocity;
	Vector pred_LastConfirmedValue;

	//Inlines
	inline StoredNetvars* GetCurrentRecord()
	{
		if (PersistentData.m_PlayerRecords.size() != 0)
		{
			return PersistentData.m_PlayerRecords.front();
		}
		return nullptr;
	}
	inline StoredNetvars* GetPreviousRecord()
	{
		if (PersistentData.m_PlayerRecords.size() > 1)
		{
			return PersistentData.m_PlayerRecords.at(1);
		}
		return nullptr;
	}
	inline StoredNetvars* GetLastRecord()
	{
		if (PersistentData.m_PlayerRecords.size() != 0)
		{
			return PersistentData.m_PlayerRecords.back();
		}
		return nullptr;
	}
	inline StoredLowerBodyYaw* GetCurrentLowerBodyRecord()
	{
		if (PersistentData.m_PlayerLowerBodyRecords.size() != 0)
		{
			return PersistentData.m_PlayerLowerBodyRecords.front();
		}
		return nullptr;
	}
	inline StoredLowerBodyYaw* GetPreviousLowerBodyRecord()
	{
		if (PersistentData.m_PlayerLowerBodyRecords.size() > 1)
		{
			return PersistentData.m_PlayerLowerBodyRecords.at(1);
		}
		return nullptr;
	}
	inline StoredLowerBodyYaw* GetLastLowerBodyRecord()
	{
		if (PersistentData.m_PlayerLowerBodyRecords.size() != 0)
		{
			return PersistentData.m_PlayerLowerBodyRecords.back();
		}
		return nullptr;
	}
	inline void SetSimulationTimePlayerWasShotOnClient()
	{
		if (pBacktrackedTick)
			PersistentData.flSimulationTimePlayerWasShotOnClient = pBacktrackedTick->simulationtime;
		else
			PersistentData.flSimulationTimePlayerWasShotOnClient = BaseEntity->GetSimulationTime();
#if 0
		StoredNetvars *backtracktick = GetBestBacktrackTick();
		if (backtracktick && !IsBreakingLagCompensation)
			PersistentData.flSimulationTimePlayerWasShotOnClient = backtracktick->simulationtime;
		else
			PersistentData.flSimulationTimePlayerWasShotOnClient = BaseEntity->GetSimulationTime();
#endif
	}
	inline void SetCurrentTimeAndAnglesPlayerWasShotOnClient()
	{
		flLastTimeWeShotAtThisPlayer = GetGlobalCurTime();
		if (pBacktrackedTick)
		{
			bWasHistoryBacktrackedWhenWeShotAtThisPlayer = bIsHistoryBacktracked;
			angIdealEyeAnglesWhenWeShotAtThisPlayer = pBacktrackedTick->idealeyeangles;
			bWasMovingWhenWeLastShotAtThem = pBacktrackedTick->velocity.Length() > 0.1f;
		}
		else
		{
			bWasHistoryBacktrackedWhenWeShotAtThisPlayer = false;
			angIdealEyeAnglesWhenWeShotAtThisPlayer = BaseEntity->GetEyeAngles();
			bWasMovingWhenWeLastShotAtThem = BaseEntity->GetVelocity().Length() > 0.1f;
		}
	}
};

inline void SaveBestHitboxPos(const CustomPlayer* pCPlayer, StoredNetvars* record, int hitboxid, float fldamage, const Vector& origin)
{
	SavedHitboxPos *dest = &record->BestHitboxPos[hitboxid];
	dest->bsaved = true;
	dest->origin = origin;
	dest->damage = fldamage;
	dest->corrected_body_yaw_index = pCPlayer->PersistentData.correctedbodyyawindex;
}

struct BloodRecord
{
	BloodRecord() {}
	~BloodRecord() {}
	BloodRecord(CBaseEntity* Entity, StoredNetvars *ForcedVars, float RealYaw)
	{
		if (ForcedVars)
		{
			simulationtime = ForcedVars->simulationtime;
			lowerbodyyaw = ForcedVars->lowerbodyyaw;
			eyeangles = ForcedVars->eyeangles;
			networkorigin = ForcedVars->networkorigin;
		}
		else
		{
			simulationtime = Entity->GetSimulationTime();
			lowerbodyyaw = Entity->GetLowerBodyYaw();
			eyeangles = Entity->GetEyeAngles();
			networkorigin = Entity->GetNetworkOrigin();
		}
		realeyeyaw = RealYaw;
		tickcount = GetExactTick(simulationtime);
		body_yaw = Entity->GetPoseParameter(11);
		HasPreviousRecord = 0; //FIXME
	}
	int tickcount;
	float simulationtime;
	float lowerbodyyaw;
	float realeyeyaw;
	float body_yaw;
	QAngle eyeangles;
	Vector networkorigin;
	BOOL HasPreviousRecord;
};

void ClearAllPlayers();

#define MAX_PLAYER_NAME_LENGTH	32
#define MAX_TRAIL_LENGTH	30
typedef struct MapPlayer_s {
	int		index;		// player's index
	int		userid;		// user ID on server
	int		icon;		// players texture icon ID
	Color   color;		// players team color
	wchar_t	name[MAX_PLAYER_NAME_LENGTH];
	int		team;		// N,T,CT
	int		health;		// 0..100, 7 bit
	Vector	position;	// current x,y pos
	QAngle	angle;		// view origin 0..360
	Vector2D trail[MAX_TRAIL_LENGTH];	// save 1 footstep each second for 1 minute
} MapPlayer_t;

using ServerRankRevealAllFn = bool(__cdecl*)(float*);
extern ServerRankRevealAllFn ServerRankRevealAllEx;

void ServerRankRevealAll();

extern unsigned char(__cdecl *ReadByte) (uintptr_t);
extern int(__cdecl *ReadInt) (uintptr_t);
extern float(__cdecl *ReadFloat) (uintptr_t);
extern double(__cdecl *ReadDouble) (uintptr_t);
extern short(__cdecl *ReadShort) (uintptr_t);
extern void(__cdecl *WriteByte) (uintptr_t, unsigned char);
extern void(__cdecl *WriteInt) (uintptr_t, int);
extern void(__cdecl *WriteFloat) (uintptr_t, float);
extern void(__cdecl *WriteDouble) (uintptr_t, double);
extern void(__cdecl *WriteShort) (uintptr_t, short);

inline void ReadByteArray(uintptr_t adr, char* dest, int size)
{
	for (int i = 0; i < size; i++)
	{
		dest[i] = ReadByte(adr + i);
	}
}

inline void WriteByteArray(char* dest, char* source, int sourcelength)
{
	for (int i = 0; i < sourcelength; i++)
	{
		WriteByte((uintptr_t)(dest + i), source[i]);
	}
}

//WARNING: No checks for evenly dividable sizes!
inline void ReadIntArray(uintptr_t adr, char* dest, int size)
{
	for (int offset = 0; offset < size; offset += 4)
	{
		//Get the rest of the bytes
		*(int*)(dest + offset) = ReadInt(adr + offset);
	}

}

/*
__forceinline void Read2DVector(uintptr_t adr, char* dest)
{
	*(char*)dest = ReadDouble(adr);
}

__forceinline void Read2DVector(uintptr_t adr, Vector2D& dest)
{
	*(char*)&dest = ReadDouble(adr);
}
__forceinline void Read2DVector(uintptr_t adr, float* dest)
{
	*dest = ReadDouble(adr);
}
*/

uintptr_t FindMemoryPattern(HANDLE ModuleHandle, char* strpattern, int length);

inline void ReadVector(uintptr_t adr, Vector& dest)
{
	dest.x = ReadFloat(adr);
	dest.y = ReadFloat(adr + sizeof(float));
	dest.z = ReadFloat(adr + (sizeof(float) * 2));
}

inline void ReadAngle(uintptr_t adr, QAngle& dest)
{
	dest.x = ReadFloat(adr);
	dest.y = ReadFloat(adr + sizeof(float));
	dest.z = ReadFloat(adr + (sizeof(float) * 2));
}

inline void WriteAngle(uintptr_t adr, QAngle ang)
{
	WriteFloat(adr, ang.x);
	WriteFloat(adr + sizeof(float), ang.y);
	WriteFloat(adr + (sizeof(float) * 2), ang.z);
}

inline void WriteVector(uintptr_t adr, Vector ang)
{
	WriteFloat(adr, ang.x);
	WriteFloat(adr + sizeof(float), ang.y);
	WriteFloat(adr + (sizeof(float) * 2), ang.z);
}

extern HANDLE EngineHandle;
extern HANDLE ClientHandle;
extern HMODULE ThisDLLHandle;
extern HANDLE MatchmakingHandle;
extern HANDLE VPhysicsHandle;
extern HANDLE VSTDLIBHandle;
extern HANDLE SHADERAPIDX9Handle;
extern HANDLE DatacacheHandle;
extern HANDLE Tier0Handle;

class CBaseEntity;

inline BOOLEAN IsInSimulation(DWORD ClientState)
{
	return (BOOLEAN)ReadByte(ClientState + m_bInSimulation);
}

inline BOOLEAN IsHLTV(DWORD ClientState)
{
	return (BOOLEAN)ReadByte(ClientState + isHLTV);
}

#if 0
inline CBaseEntity* GetLocalPlayer()
{
	return (CBaseEntity*)ReadInt((DWORD)ClientHandle + m_dwLocalPlayer);
}
#endif

inline DWORD GetLocalPlayerIndex()
{
	return ReadInt((DWORD)ClientHandle + m_dwLocalPlayerIndex);
}

inline unsigned char GetDormant(CBaseEntity* Entity)
{
	return ReadByte((DWORD)Entity + m_bDormant);
}

inline int GetZoomLevel(CBaseEntity* Entity) {
	return ReadInt((DWORD)Entity + m_bIsScoped);
}

inline int GetFOV(CBaseEntity* Entity)
{
	return ReadInt((DWORD)Entity + m_iFOV);
}

inline int GetPlayerLifeState(CBaseEntity* Entity)
{
	return ReadInt((DWORD)Entity + m_lifeState);
}

inline TEAMS GetPlayerTeamDylan(CBaseEntity* Entity)
{
	return (TEAMS)ReadInt((DWORD)Entity + m_iTeamNum);
}

inline int GetPlayerHealth(CBaseEntity* BasePlayer)
{
	return ReadInt((DWORD)BasePlayer + m_iHealth);
}

#if 0
inline CBaseEntity* GetBasePlayer(DWORD index)
{
	return (CBaseEntity*)ReadInt((DWORD)ClientHandle + m_dwEntityList + (index * 16));
}
#endif

inline void GetPlayerOrigin(CBaseEntity* Entity, Vector& dest)
{
	ReadVector((DWORD)Entity + m_vecOrigin, dest);
}

inline DWORD GetBoneMatrix(CBaseEntity* Entity)
{
	return ReadInt((DWORD)Entity + m_dwBoneMatrix);
}

#if 0
struct Bones {
	matrix3x4_t bones[128];
};

Vector getBonePosition(DWORD p, int boneId) {
	Bones bones = mem.Read<Bones>(p + 0x2698);

	Vector pos;
	pos.x = bones.bones[boneId][0][3];
	pos.y = bones.bones[boneId][1][3];
	pos.z = bones.bones[boneId][2][3];
	return pos;
}
#endif

inline void GetBonePos(DWORD BoneMatrix, int BoneID, Vector& BonePos)
{
	BonePos.x = ReadFloat((BoneMatrix + (0x30 * BoneID) + 0xC));//x
	BonePos.y = ReadFloat((BoneMatrix + (0x30 * BoneID) + 0x1C));//y
	BonePos.z = ReadFloat((BoneMatrix + (0x30 * BoneID) + 0x2C));//z
}

#if 0
inline DWORD GetCSPlayerResource()
{
	return ReadInt((DWORD)ClientHandle + DT_CSPlayerResource);
}
#endif

#if 0
inline DWORD GetClientState()
{
	return ReadInt((DWORD)EngineHandle + m_dwClientState);
}
#endif

inline DWORD GetRadar()
{
	//return ReadInt(ReadInt((DWORD)ClientHandle + m_dwRadarBase) + m_dwRadarBasePointer);
	return ReadInt(*(DWORD*)RadarBaseAdr + m_dwRadarBasePointer);
}

inline CUserCmd * GetCurrentCommand(CBaseEntity* BasePlayer)
{
	return (CUserCmd*)(ReadInt((uintptr_t)BasePlayer + m_pCurrentCommand));
}

inline void SetCurrentCommand(CBaseEntity* BasePlayer, CUserCmd* currentcommand)
{
	WriteInt((uintptr_t)BasePlayer + m_pCurrentCommand, (int)currentcommand);
}

//BROKEN
inline void GetMapPlayer_T(DWORD RadarAdr, int index, MapPlayer_t *dest)
{
	ReadByteArray(RadarAdr + sizeof(MapPlayer_t) * index, (char*)dest, sizeof(MapPlayer_t));
}

wchar_t* GetPlayerNameAddress(DWORD RadarAdr, int index);

void GetPlayerName(DWORD RadarAdr, int index, wchar_t* dest);

//TODO: BROKEN
inline int GetMaxPlayers(DWORD ClientState)
{
	return (int)ReadByte(ClientState + m_dwMaxPlayer);
}

#if 0
inline CBaseEntity* GetBaseEntity(int index)
{
	return (CBaseEntity*)ReadInt((DWORD)ClientHandle + m_dwEntityList + (index * 16));
}
#endif

inline DWORD GetGlowObjectManager()
{
	return GlowObjectManagerAdr;
	//return ((DWORD)ClientHandle + m_dwGlowObject);
}

inline DWORD GetGlowObject()
{
	return ReadInt(GetGlowObjectManager());
}

/*
//This doesn't get encrypted for some reason

static std::string Ranks[] =
{
	strenc("-"),
	strenc("Silver I"),
	strenc("Silver II"),
	strenc("Silver III"),
	strenc("Silver IV"),
	strenc("Silver Elite"),
	strenc("Silver Elite Master"),

	strenc("Gold Nova I"),
	strenc("Gold Nova II"),
	strenc("Gold Nova III"),
	strenc("Gold Nova Master"),
	strenc("Master Guardian I"),
	strenc("Master Guardian II"),

	strenc("Master Guardian Elite"),
	strenc("Distinguished Master Guardian"),
	strenc("Legendary Eagle"),
	strenc("Legendary Eagle Master"),
	strenc("Supreme Master First Class"),
	strenc("The Global Elite")
};
*/

inline int GetCompetitiveRank(DWORD CSPlayerResource, DWORD index)
{
	return ReadInt(CSPlayerResource + m_iCompetitiveRanking + index * 4);
}

extern char *silver1str;
extern char *silver2str;
extern char *silver3str;
extern char *silver4str;
extern char *silver5str;
extern char *silver6str;
extern char *nova1str;
extern char *nova2str;
extern char *nova3str;
extern char *nova4str;
extern char *mg1str;
extern char *mg2str;
extern char *mg3str;
extern char *mg4str;
extern char *lestr;
extern char *lemstr;
extern char *supremestr;
extern char *globalstr;

inline void GetCompetitiveRankString(char* dest, int destlength, int rank)
{
#if 0
	//strcpy(dest, Ranks[rank].c_str());
	switch (rank) {
		case 0:
			strcpy(dest, "-");
			break;
		case 1:
			DecStr(silver1str);
			strcpy(dest, silver1str);
			EncStr(silver1str);
			break;
		case 2:
			DecStr(silver2str);
			strcpy(dest, silver2str);
			EncStr(silver2str);
			break;
		case 3:
			DecStr(silver3str);
			strcpy(dest, silver3str);
			EncStr(silver3str);
			break;
		case 4:
			DecStr(silver4str);
			strcpy(dest, silver4str);
			EncStr(silver4str);
			break;
		case 5:
			DecStr(silver5str);
			strcpy(dest, silver5str);
			EncStr(silver5str);
			break;
		case 6:
			DecStr(silver6str);
			strcpy(dest, silver6str);
			EncStr(silver6str);
			break;
		case 7:
			DecStr(nova1str);
			strcpy(dest, nova1str);
			EncStr(nova1str);
			break;
		case 8:
			DecStr(nova2str);
			strcpy(dest, nova2str);
			EncStr(nova2str);
			break;
		case 9:
			DecStr(nova3str);
			strcpy(dest, nova3str);
			EncStr(nova3str);
			break;
		case 10:
			DecStr(nova4str);
			strcpy(dest, nova4str);
			EncStr(nova4str);
			break;
		case 11:
			DecStr(mg1str);
			strcpy(dest, mg1str);
			EncStr(mg1str);
			break;
		case 12:
			DecStr(mg2str);
			strcpy(dest, mg2str);
			EncStr(mg2str);
			break;
		case 13:
			DecStr(mg3str);
			strcpy(dest, mg3str);
			EncStr(mg3str);
			break;
		case 14:
			DecStr(mg4str);
			strcpy(dest, mg4str);
			EncStr(mg4str);
			break;
		case 15:
			DecStr(lestr);
			strcpy(dest, lestr);
			EncStr(lestr);
			break;
		case 16:
			DecStr(lemstr);
			strcpy(dest, lemstr);
			EncStr(lemstr);
			break;
		case 17:
			DecStr(supremestr);
			strcpy(dest, supremestr);
			EncStr(supremestr);
			break;
		case 18:
			DecStr(globalstr);
			strcpy(dest, globalstr);
			EncStr(globalstr);
			break;
	}
#endif
}

inline int GetCompetitiveWins(DWORD CSPlayerResource, DWORD index)
{
	return ReadInt(CSPlayerResource + m_iCompetitiveWins + index * 4);
}

inline void SpotPlayer(CBaseEntity* Entity)
{
	WriteByte((DWORD)Entity + m_bSpotted, 1);
}

extern RECT rc; //screen rectangle
extern HWND tWnd; //target game window
extern HWND hWnd; //hack window

inline bool PointIsInsideBox(Vector2D point, Vector2D topleft, int width, int height)
{
	Vector2D bottomright = { topleft.x + width, topleft.y + height };
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector topleft, int width, int height)
{
	Vector2D bottomright = { topleft.x + width, topleft.y + height };
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector2D topleft, int width, int height)
{
	Vector2D bottomright = { topleft.x + width, topleft.y + height };
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector2D point, Vector2D topleft, Vector2D bottomright)
{
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector topleft, Vector bottomright)
{
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

inline bool PointIsInsideBox(Vector point, Vector2D topleft, Vector2D bottomright)
{
	if ((point.x >= topleft.x && point.x <= bottomright.x)
		&& (point.y >= topleft.y && point.y <= bottomright.y))
		return true;
	return false;
}

//Dylan added below
inline float DistanceBetweenPoints(Vector2D points1, Vector2D points2)
{
	float dx = points1.x - points2.x;
	float dy = points1.y - points2.y;
	float sqin = dx*dx + dy*dy;
	float sqout;
	SSESqrt(&sqout, &sqin);
	return sqout;//fabs(sqrtf(dx*dx + dy*dy));
}

inline float DistanceBetweenPoints(Vector points1, Vector points2)
{
	float dx = points1.x - points2.x;
	float dy = points1.y - points2.y;
	float sqin = dx*dx + dy*dy;
	float sqout;
	SSESqrt(&sqout, &sqin);
	return sqout; //fabs(sqrtf(dx*dx + dy*dy));
}

typedef struct
{
	float flMatrix[4][4];
}WorldToScreenMatrix_t;

BOOL WorldToScreenCapped(Vector& from, Vector& to);
BOOL WorldToScreen(Vector& from, Vector& to);

struct rgba_t
{
	float r;
	float g;
	float b;
	float a;

	bool operator==(const rgba_t& other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool operator!=(const rgba_t& other) const
	{
		return !((*this) == other);
	}
};

struct GlowObject_t
{
	uint32_t pEntity;
	rgba_t rgba;
	uint8_t unk1[16];
	bool m_bRenderWhenOccluded;
	bool m_bRenderWhenUnoccluded;
	bool m_bFullBloom;
	uint8_t unk2[0xE];
};


#endif