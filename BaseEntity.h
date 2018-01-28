#pragma once
#include "misc.h"
#include <string>
#include "NetVarManager.h"
#include "utlvectorsimple.h"
#include <vector>
#include "bone_accessor.h"
#include "Offsets.h"
#include "threadtools.h"

class INetChannelInfo;
class ICollideable;
class CBaseAnimating;
class CBaseCombatWeapon;
class C_AnimationLayer;
struct StoredNetvars;
struct C_CSGOPlayerAnimState;
struct studiohdr_t;
class CStudioHdr;

extern bool* s_bEnableInvalidateBoneCache;
extern bool* bAllowBoneAccessForViewModels;
extern bool* bAllowBoneAccessForNormalModels;
extern unsigned long* g_iModelBoneCounter;
extern int unknownsetupbonescall_offset;
extern DWORD UnknownSetupBonesVTable;
extern bool* g_bInThreadedBoneSetup;
extern bool(*ThreadInMainThread) (void);

// edict->solid values
// NOTE: Some movetypes will cause collisions independent of SOLID_NOT/SOLID_TRIGGER when the entity moves
// SOLID only effects OTHER entities colliding with this one when they move - UGH!

// Solid type basically describes how the bounding volume of the object is represented
// NOTE: SOLID_BBOX MUST BE 2, and SOLID_VPHYSICS MUST BE 6
// NOTE: These numerical values are used in the FGD by the prop code (see prop_dynamic)
enum SolidType_t
{
	SOLID_NONE = 0,	// no solid model
	SOLID_BSP = 1,	// a BSP tree
	SOLID_BBOX = 2,	// an AABB
	SOLID_OBB = 3,	// an OBB (not implemented yet)
	SOLID_OBB_YAW = 4,	// an OBB, constrained so that it can only yaw
	SOLID_CUSTOM = 5,	// Always call into the entity for tests
	SOLID_VPHYSICS = 6,	// solid vphysics object, get vcollide from the model and collide with that
	SOLID_LAST,
};

enum MoveType_t
{
	MOVETYPE_NONE = 0,
	MOVETYPE_ISOMETRIC,
	MOVETYPE_WALK,
	MOVETYPE_STEP,
	MOVETYPE_FLY,
	MOVETYPE_FLYGRAVITY,
	MOVETYPE_VPHYSICS,
	MOVETYPE_PUSH,
	MOVETYPE_NOCLIP,
	MOVETYPE_LADDER,
	MOVETYPE_OBSERVER,
	MOVETYPE_CUSTOM,
	MOVETYPE_LAST = MOVETYPE_CUSTOM,
	MOVETYPE_MAX_BITS = 4
};

struct CustomPlayer;

INetChannelInfo* GetPlayerNetInfoServer(int entindex);

class CBaseEntity
{
public:
	char				__pad[0x64];
	int					index;
	CustomPlayer* CBaseEntity::ToCustomPlayer();
	int					GetHealth();
	int					GetTeam();
	int					GetFlags();
	void SetFlags(int flags);
	int					GetTickBase();
	void SetTickBase(int base);
	int					GetShotsFired();
	void SetMoveType(int type);
	int					GetMoveType();
	int					GetModelIndex();
	void SetModelIndex(int index);
	int					GetHitboxSet();
	void SetHitboxSet(int set);
	int					GetUserID();
	int					GetArmor();
	int					GetCollisionGroup();
	int					PhysicsSolidMaskForEntity();
	CBaseEntity*		GetOwner();
	int					GetGlowIndex();
	BOOL				GetAlive();
	BOOLEAN				GetAliveServer();
	void CalcAbsolutePositionServer();
	BOOLEAN				GetDormant();
	BOOLEAN				GetImmune();
	bool				IsEnemy();
	bool				IsVisible(int bone);
	bool				m_visible;
	BOOLEAN				IsBroken();
	BOOLEAN				HasHelmet();
	BOOLEAN HasDefuseKit();
	BOOLEAN IsDefusing();
	BOOLEAN				IsFlashed();
	bool				IsTargetingLocal();
	float				GetFlashDuration();
	void SetFlashDuration(float dur);
	float				GetBombTimer();
	QAngle				GetViewPunch();
	void SetViewPunch(QAngle punch);
	QAngle				GetPunch();
	void SetPunch(QAngle punch);
	Vector              GetPunchVel();
	void SetPunchVel(Vector vel);
	QAngle				GetEyeAngles();
	QAngle				GetEyeAnglesServer();
	QAngle* EyeAngles();
	void SetEyeAngles(QAngle angles);
	QAngle GetRenderAngles();
	void SetRenderAngles(QAngle angles);
	Vector				GetOrigin();
	Vector GetLocalOrigin() { return GetOrigin();
	}
	Vector				GetNetworkOrigin();
	Vector GetOldOrigin();
	void SetOrigin(Vector origin);
	void SetNetworkOrigin(Vector origin);
	void SetAbsOrigin(const Vector &origin);
	QAngle GetAbsAngles();
	QAngle GetAbsAnglesServer();
	Vector WorldSpaceCenter();
	Vector				GetEyePosition();
	SolidType_t         GetSolid();
	BOOLEAN ShouldCollide(int collisionGroup, int contentsMask);
	BOOLEAN IsTransparent();
	Vector GetBonePosition(int HitboxID, float time, bool ForceSetup, bool StoreInCache, StoredNetvars *Record);
	Vector GetBonePositionTotal(int HitboxID, float time, Vector &BoneMax, Vector &BoneMin, bool ForceSetup, bool StoreInCache, StoredNetvars *Record);
	Vector GetBonePositionCachedOnly(int HitboxID, matrix3x4_t* matrixes);
	void GetBonePosition(int iBone, Vector &origin, QAngle &angles, float time, bool ForceSetup, bool StoreInCache, StoredNetvars *Record);
	bool CacheCurrentBones(float flTime, bool bForce);
	bool CacheBones(float flTime, bool bForce, StoredNetvars *Record);
	bool IsViewModel();
	inline int GetLastSetupBonesFrameCount()
	{
		return *(DWORD*)((DWORD)this + LastSetupBonesFrameCount);
	}
	inline void SetLastSetupBonesFrameCount(int framecount)
	{
		*(DWORD*)((DWORD)this + LastSetupBonesFrameCount) = framecount;
	}
	inline bool IsBoneAccessAllowed()
	{
		if (!ThreadInMainThread())
		{
			return true;
		}

		if (IsViewModel())
			return *bAllowBoneAccessForViewModels;
		else
			return *bAllowBoneAccessForNormalModels;
	}
	inline int GetPreviousBoneMask()
	{
		return *(DWORD*)((DWORD)this + m_iPrevBoneMask);
	}
	inline void SetPreviousBoneMask(int mask)
	{
		*(int*)((DWORD)this + m_iPrevBoneMask) = mask;
	}
	inline int GetAccumulatedBoneMask()
	{
		return *(int*)((DWORD)this + m_iAccumulatedBoneMask);
	}
	inline void SetAccumulatedBoneMask(int mask)
	{
		*(int*)((DWORD)this + m_iAccumulatedBoneMask) = mask;
	}
	float LastBoneChangedTime();
	inline float GetLastBoneSetupTime()
	{
		return *(float*)((DWORD)this + m_flLastBoneSetupTime);
	}
	inline void SetLastBoneSetupTime(float time)
	{
		*(float*)((DWORD)this + m_flLastBoneSetupTime) = time;
	}
	inline DWORD GetIK()
	{
		return *(DWORD*)((DWORD)this + m_pIK_offset);
	}
	inline void SetIK(DWORD ik)
	{
		*(DWORD*)((DWORD)this + m_pIK_offset) = ik;
	}
	inline unsigned char GetEntClientFlags()
	{
		return *(unsigned char*)((DWORD)this + m_EntClientFlags);
	}
	inline void SetEntClientFlags(unsigned char flags)
	{
		*(unsigned char*)((DWORD)this + m_EntClientFlags) = flags;
	}
	inline void AddEntClientFlag(unsigned char flag)
	{
		*(unsigned char*)((DWORD)this + m_EntClientFlags) |= flag;
	}
	inline bool IsToolRecording()
	{
		return *(bool*)((DWORD)this + m_bIsToolRecording);
	}
	inline bool GetPredictable()
	{
		return *(bool*)((DWORD)this + m_dwGetPredictable);
	}
	inline CThreadFastMutex* GetBoneSetupLock()
	{
		return (CThreadFastMutex*)((DWORD)this + m_BoneSetupLock);
	}
	bool SetupBonesRebuiltNoCache(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	bool SetupBonesRebuiltSimple(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	bool SetupBonesRebuilt(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	bool SetupBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime);
	Vector GetVelocity();
	void SetVelocity(Vector velocity);
	void SetAbsVelocity(Vector velocity);
	Vector GetBaseVelocity();
	void SetBaseVelocity(Vector velocity);
	Vector				GetPredicted(Vector p0);
	ICollideable*		GetCollideable();
	void		GetPlayerInfo(player_info_t *dest);
	model_t*			GetModel();
	CStudioHdr* GetModelPtr();
	studiohdr_t* GetStudioHDR();
	void SetModel(model_t* mod);
	std::string			GetName();
	void			GetSteamID(char* dest);
	std::string			GetLastPlace();
	CBaseCombatWeapon*	GetWeapon();
	ClientClass*		GetClientClass();
	CBaseAnimating* GetBaseAnimating();
	void InvalidateBoneCache();
	HANDLE GetObserverTarget();
	BOOLEAN IsPlayer();
	CUserCmd* GetLastUserCommand();
	BOOLEAN IsConnected();
	BOOLEAN IsSpawned();
	BOOLEAN IsActive();
	BOOLEAN IsFakeClient();
	bool IsTargetingLocalPunch();
	QAngle GetAngleRotation();
	void SetAngleRotation(QAngle rot);
	QAngle GetLocalAngles();
	void SetLocalAngles(QAngle rot);
	QAngle GetOldAngRotation();
	void SetOldAngRotation(QAngle rot);
	void SetAbsAngles(const QAngle& rot);
	Vector GetMins();
	void SetMins(Vector mins);
	Vector GetMaxs();
	void SetMaxs(Vector maxs);
	int GetSequence();
	void SetSequence(int seq);
	void GetPoseParameterRange(int index, float& flMin, float& flMax);
	float GetPoseParameter(int index);
	float GetPoseParameterUnscaled(int index);
	float GetPoseParameterUnScaledServer(int index);
	int LookupPoseParameter(char *name);
	void SetPoseParameter(int index, float p);
	void SetPoseParameterScaled(int index, float p);
	void CopyPoseParameters(float* dest);
	unsigned char GetClientSideAnimation();
	void SetClientSideAnimation(unsigned char a);
	float GetCycle();
	void SetCycle(float cycle);
	float GetNextAttack();
	void SetNextAttack(float att);
	BOOLEAN IsDucked();
	void SetDucked(bool ducked);
	BOOLEAN IsDucking();
	void SetDucking(bool ducking);
	BOOLEAN IsInDuckJump();
	void SetInDuckJump(bool induckjump);
	int GetDuckTimeMsecs();
	void SetDuckTimeMsecs(int msecs);
	int GetJumpTimeMsecs();
	void SetJumpTimeMsecs(int msecs);
	float GetFallVelocity();
	void SetFallVelocity(float vel);
	Vector GetViewOffset();
	void SetViewOffset(Vector off);
	int GetNextThinkTick();
	void SetNextThinkTick(int tick);
	float GetDuckAmount();
	void SetDuckAmount(float duckamt);
	float GetDuckSpeed();
	void SetDuckSpeed(float spd);
	float GetVelocityModifier();
	void SetVelocityModifier(float vel);
	float GetPlaybackRate();
	void SetPlaybackRate(float rate);
	float GetAnimTime();
	void SetAnimTime(float time);
	float GetSimulationTime();
	float GetSimulationTimeServer();
	float GetOldSimulationTime();
	void SetSimulationTime(float time);
	float GetLaggedMovement();
	void SetLaggedMovement(float mov);
	CBaseEntity* GetGroundEntity();
	void SetGroundEntity(CBaseEntity* groundent);
	Vector GetVecLadderNormal();
	void SetVecLadderNormal(Vector norm);
	float GetLowerBodyYaw();
	float GetLowerBodyYawServer();
	void SetLowerBodyYaw(float yaw);
	C_CSGOPlayerAnimState* GetPlayerAnimState();
	void* GetPlayerAnimStateServer();
	float GetNextLowerBodyyawUpdateTimeServer();
	bool IsWeapon();
	bool IsProjectile();
	bool IsFlashGrenade();
	bool IsChicken();
	void DrawHitboxes(ColorRGBA color, float livetimesecs);
	void DrawHitboxesFromCache(ColorRGBA color, float livetimesecs, matrix3x4_t *matrix);
	INetChannelInfo* GetNetChannel();
	void DisableInterpolation();
	void EnableInterpolation();
	void UpdateClientSideAnimation();
	float GetLastClientSideAnimationUpdateTime();
	void SetLastClientSideAnimationUpdateTime(float time);
	int GetLastClientSideAnimationUpdateGlobalsFrameCount();
	void SetLastClientSideAnimationUpdateGlobalsFrameCount(int framecount);
	float FrameAdvance(float flInterval);
	int GetEffects();
	void SetEffects(int effects);
	int GetObserverMode();
	void SetObserverMode(int mode);
	CUtlVectorSimple* GetAnimOverlayStruct() const;
	C_AnimationLayer* GetAnimOverlay(int i);
	int GetNumAnimOverlays() const;
	void CopyAnimLayers(C_AnimationLayer* dest);
	void InvalidatePhysicsRecursive(int nChangeFlags);
	Vector* GetAbsOrigin();
	Vector GetAbsOriginServer();
	int entindex();
	int entindexServer();
	float GetCurrentFeetYawServer();
	float GetCurrentFeetYaw();
	void SetCurrentFeetYaw(float yaw);
	float GetGoalFeetYawServer();
	float GetGoalFeetYaw();
	void SetGoalFeetYaw(float yaw);
	float GetFriction();
	void SetFriction(float friction);
	float GetStepSize();
	void SetStepSize(float stepsize);
	float GetMaxSpeed();
	void SetMaxSpeed(float maxspeed);
	bool IsParentChanging();
	void SetLocalVelocity(const Vector& vecVelocity);
	int GetTakeDamage() { return *(DWORD*)((DWORD)this + 0x27C); }
	inline const char* GetClassname() {
		return ((const char*(__thiscall*)(CBaseEntity*)) *(DWORD*)(*(DWORD*)this + 0x22C) )(this);
	}
	inline int GetUnknownInt()
	{
		return (*(int(__thiscall **)(CBaseEntity*))(*(DWORD *)this + 0x1E8))(this);
	}
	inline CBoneAccessor* GetBoneAccessor()
	{
		return (CBoneAccessor*)((DWORD)this + dw_m_BoneAccessor);
	}
	void SetBoneAccessor(CBoneAccessor* boneAccessor)
	{
		*(DWORD*)((DWORD)this + dw_m_BoneAccessor) = (DWORD)boneAccessor;
	}
	void StandardBlendingRules(CStudioHdr *hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask);
	void BuildTransformations(CStudioHdr *hdr, Vector *pos, Quaternion *q, const matrix3x4_t &cameraTransform, int boneMask, byte *boneComputed);
	inline int GetCreationTick()
	{
		return *(int*)((DWORD)this + m_nCreationTick);
	}
	inline void SetCreationTick(int tick)
	{
		*(int*)((DWORD)this + m_nCreationTick) = tick;
	}
	inline float GetProxyRandomValue()
	{
		return *(float*)((DWORD)this + m_flProxyRandomValue);
	}
	inline void SetProxyRandomValue(float value)
	{
		*(float*)((DWORD)this + m_flProxyRandomValue) = value;
	}
	inline bool IsRagdoll()
	{
		return *(DWORD*)((DWORD)this + m_pRagdoll) && *(char*)((DWORD)this + m_bClientSideRagdoll);
	}
	CUtlVectorSimple* GetCachedBoneData()
	{
		return (CUtlVectorSimple*)((DWORD)this + m_CachedBoneData);
	}
	inline unsigned long GetMostRecentModelBoneCounter()
	{
		return *(unsigned long*)((DWORD)this + m_iMostRecentModelBoneCounter);
	}
	inline void SetMostRecentModelBoneCounter(unsigned long counter)
	{
		*(unsigned long*)((DWORD)this + m_iMostRecentModelBoneCounter) = counter;
	}
	inline int GetLastOcclusionCheckFrameCount()
	{
		return *(int*)((DWORD)this + m_iLastOcclusionCheckFrameCount);
	}
	inline void SetLastOcclusionCheckFrameCount(int count)
	{
		*(int*)((DWORD)this + m_iLastOcclusionCheckFrameCount) = count;
	}
	inline int GetLastOcclusionCheckFlags()
	{
		return *(int*)((DWORD)this + m_iLastOcclusionCheckFlags);
	}
	inline void SetLastOcclusionCheckFlags(int flags)
	{
		*(int*)((DWORD)this + m_iLastOcclusionCheckFlags) = flags;
	}
	void UpdateIKLocks(float currentTime);
	void CalculateIKLocks(float currentTime);
	void ControlMouth(CStudioHdr *pStudioHdr);
	void Wrap_SolveDependencies(DWORD m_pIk, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed);
	void Wrap_UpdateTargets(DWORD m_pIk, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed);
	void Wrap_AttachmentHelper(CStudioHdr * hdr);
	inline void ClearTargets(DWORD m_pIk)
	{
		//??? valve pls
		int max = *(int*)(m_pIk + 4080);

		int v110 = 0;

		if (max > 0)
		{
			DWORD v111 = (DWORD)(m_pIk + 208);
			do
			{
				*(int*)(v111) = -9999;
				v111 += 85;
				++v110;
			} while (v110 < *(int*)(m_pIk + 4080));
		}
	}
	void Wrap_IKInit(DWORD m_pIk, CStudioHdr * hdrs, QAngle &angles, Vector &pos, float flTime, int iFramecounter, int boneMask);
	int GetUnknownSetupBonesInt();
	void DoUnknownSetupBonesCall(CStudioHdr *hdr, Vector *pos, Quaternion *q, matrix3x4_t* bonearray, byte* computed, DWORD* m_pIK);
	DWORD Wrap_CreateIK();
	void MDLCACHE_CRITICAL_SECTION();
};

class VTHook;

struct HookedEntity
{
	int index;
	CBaseEntity* entity;
	VTHook* hook;
	DWORD OriginalHookedSub1;
	DWORD OriginalHookedSub2;
};


#if 1
extern std::vector<HookedEntity*> HookedEntities;

void DestructHookedEntity(CBaseEntity* me);
void DeleteEntityHook(VTHook* hook);
BOOLEAN __fastcall HookedEntityShouldInterpolate(CBaseEntity* me);
void __fastcall HookedUpdateClientSideAnimation(CBaseEntity* me);
void __fastcall HookedPlayFootstepSound(CBaseEntity* me, DWORD edx, Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force);

void HookAllBaseEntityNonLocalPlayers();
void ValidateAllEntityHooks();
#endif

struct C_CSGOPlayerAnimState
{
	char pad[3];
	char bUnknown; //0x4
	char pad2[91];
	CBaseEntity* pBaseEntity; //0x60
	CBaseCombatWeapon* pActiveWeapon; //0x64
	CBaseCombatWeapon* pLastActiveWeapon; //0x68
	float m_flLastClientSideAnimationUpdateTime; //0x6C
	int m_iLastClientSideAnimationUpdateFramecount; //0x70
	float m_flEyePitch; //0x74
	float m_flEyeYaw; //0x78
	float m_flPitch; //0x7C
	float m_flGoalFeetYaw; //0x80
	float m_flCurrentFeetYaw; //0x84
	float m_flCurrentTorsoYaw; //0x88
	float m_flUnknownVelocityLean; //0x8C //changes when moving/jumping/hitting ground
	float m_flLeanAmount; //0x90
	char pad4[4]; //NaN
	float m_flFeetCycle; //0x98 0 to 1
	float m_flFeetYawRate; //0x9C 0 to 1
	float m_fUnknown2;
	float m_fDuckAmount; //0xA4
	float m_fLandingDuckAdditiveSomething; //0xA8
	float m_fUnknown3; //0xAC
	Vector m_vOrigin; //0xB0, 0xB4, 0xB8
	Vector m_vLastOrigin; //0xBC, 0xC0, 0xC4
	float m_vVelocityX; //0xC8
	float m_vVelocityY; //0xCC
	char pad5[4];
	float m_flUnknownFloat1; //0xD4 Affected by movement and direction
	char pad6[8];
	float m_flUnknownFloat2; //0xE0 //from -1 to 1 when moving and affected by direction
	float m_flUnknownFloat3; //0xE4 //from -1 to 1 when moving and affected by direction
	float m_unknown; //0xE8
	float m_velocity; //0xEC
	float flUpVelocity; //0xF0
	float m_flSpeedNormalized; //0xF4 //from 0 to 1
	float m_flFeetSpeedForwardsOrSideWays; //0xF8 //from 0 to 2. something  is 1 when walking, 2.something when running, 0.653 when crouch walking
	float m_flFeetSpeedUnknownForwardOrSideways; //0xFC //from 0 to 3. something
	float m_flTimeSinceStartedMoving; //0x100
	float m_flTimeSinceStoppedMoving; //0x104
	unsigned char m_bOnGround; //0x108
	unsigned char m_bInHitGroundAnimation; //0x109
	char pad7[10];
	float m_flLastOriginZ; //0x114
	float m_flHeadHeightOrOffsetFromHittingGroundAnimation; //0x118 from 0 to 1, is 1 when standing
	float m_flStopToFullRunningFraction; //0x11C from 0 to 1, doesnt change when walking or crouching, only running
	char pad8[4]; //NaN
	float m_flUnknownFraction; //0x124 affected while jumping and running, or when just jumping, 0 to 1
	char pad9[4]; //NaN
	float m_flUnknown3;
	char pad10[528];
};