#include <Windows.h>
#include "BaseEntity.h"
#include "Offsets.h"
#include "VTHook.h"
#include "Trace.h"
#include "Math.h"
#include "Interfaces.h"
#include "Overlay.h"
#include "BaseAnimating.h"
#include "Interpolation.h"
#include "Animation.h"
#include "LocalPlayer.h"
#include "ThirdPerson.h"
#include "ConVar.h"

IKInitFn IKInit;
UpdateTargetsFn UpdateTargets;
SolveDependenciesFn SolveDependencies;
AttachmentHelperFn AttachmentHelper;
CreateIKFn CreateIK;
TeleportedFn Teleported;
ShouldSkipAnimationFrameFn ShouldSkipAnimationFrame;
MDLCacheCriticalSectionCallFn MDLCacheCriticalSectionCall;

bool* s_bEnableInvalidateBoneCache;
bool* bAllowBoneAccessForViewModels;
bool* bAllowBoneAccessForNormalModels;
unsigned long* g_iModelBoneCounter;
int unknownsetupbonescall_offset;
bool* g_bInThreadedBoneSetup;
DWORD UnknownSetupBonesVTable;

DWORD UnknownSetupBonesCall()
{
	UnknownSetupBonesFn call = (UnknownSetupBonesFn)*(DWORD*)(*(DWORD*)UnknownSetupBonesVTable + 0x34);
	return call(UnknownSetupBonesVTable);
}

UpdateClientSideAnimationFn oUpdateClientSideAnimation;

#if 1
std::vector<HookedEntity*> HookedEntities;

//FIXME: figure out how to hook ~C_BaseEntity
void DestructHookedEntity(CBaseEntity* me)
{
	for (auto hooked = HookedEntities.begin(); hooked != HookedEntities.end(); hooked++)
	{
		if ((*hooked)->entity != me)
			continue;
		(*hooked)->hook->ClearClassBase();
		delete (*hooked)->hook;
		/*hooked = */HookedEntities.erase(hooked);
		return;
	}
}

void ValidateAllEntityHooks()
{
	for (auto hooked = HookedEntities.begin(); hooked != HookedEntities.end(); hooked++)
	{
		CBaseEntity *entity = Interfaces::ClientEntList->GetClientEntity((*hooked)->index);
		if (entity != (*hooked)->entity)
		{
			(*hooked)->hook->ClearClassBase();
			delete (*hooked)->hook;
			hooked = HookedEntities.erase(hooked);
		}
	}
}

void DeleteEntityHook(VTHook* hook)
{
	for (auto hooked = HookedEntities.begin(); hooked != HookedEntities.end(); hooked++)
	{
		if ((*hooked)->hook != hook)
			continue;
		(*hooked)->hook->ClearClassBase();
		delete (*hooked)->hook;
		HookedEntities.erase(hooked);
		return;
	}
}

BOOLEAN __fastcall HookedEntityShouldInterpolate(CBaseEntity* me)
{
	if (me == LocalPlayer.Entity)
		return 1;
	if (me->IsPlayer())
	{
		CustomPlayer *pCPlayer = &AllPlayers[me->index];
		if (pCPlayer->HookedBaseEntity && pCPlayer->HookedBaseEntity->OriginalHookedSub1)
			return ((BOOLEAN(__thiscall*)(CBaseEntity*))pCPlayer->HookedBaseEntity->OriginalHookedSub1)(me);
	}

	return true;
}

void __fastcall HookedUpdateClientSideAnimation(CBaseEntity* me)
{
	CBaseEntity* LocalEntity = Interfaces::ClientEntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (me == LocalEntity && !DisableAllChk.Checked)
	{
		SetThirdPersonAngles();

		//oUpdateClientSideAnimation(me);
		return;
	}

	oUpdateClientSideAnimation(me);
}

void __fastcall HookedPlayFootstepSound(CBaseEntity* me, DWORD edx, Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force)
{
	CBaseEntity* LocalEntity = Interfaces::ClientEntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (me == LocalEntity && !DisableAllChk.Checked)
	{
		if (EnginePredictChk.Checked && LocalPlayer.bInPrediction)
			return;
	}
	((void(__thiscall*)(CBaseEntity*, Vector &, surfacedata_t *, float, bool))LocalPlayer.HookedBaseEntity->OriginalHookedSub2)(me, vecOrigin, psurface, fvol, force);
}

void HookAllBaseEntityNonLocalPlayers()
{
	for (int i = 0; i <= MAX_PLAYERS; i++)
	{
		CBaseEntity* Entity = Interfaces::ClientEntList->GetClientEntity(i);
		if (Entity && Entity->IsPlayer() && Entity != LocalPlayer.Entity)
		{
			CustomPlayer *pCAPlayer = &AllPlayers[Entity->index];
			if (!pCAPlayer->bHookedBaseEntity)
			{
				pCAPlayer->HookedBaseEntity = new HookedEntity;
				pCAPlayer->HookedBaseEntity->entity = Entity;
				pCAPlayer->HookedBaseEntity->index = Entity->index;
				pCAPlayer->HookedBaseEntity->hook = new VTHook((PDWORD*)Entity);
				//pCAPlayer->HookedBaseEntity->OriginalHookedSub1 = pCAPlayer->HookedBaseEntity->hook->HookFunction((DWORD)&HookedEntityShouldInterpolate, ShouldInterpolateIndex);
				pCAPlayer->bHookedBaseEntity = true;
				HookedEntities.push_back(pCAPlayer->HookedBaseEntity);
			}
		}
	}
}
#endif

INetChannelInfo* GetPlayerNetInfoServer(int entindex)
{
	static DWORD EngineInterfaceServer = NULL;
	const char *sig = "8B  0D  ??  ??  ??  ??  52  8B  01  8B  40  54";

	if (!EngineInterfaceServer)
	{
		EngineInterfaceServer = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)sig, strlen(sig));
		if (!EngineInterfaceServer)
			DebugBreak();

		EngineInterfaceServer = *(DWORD*)(EngineInterfaceServer + 2);
	}

	DWORD table = *(DWORD*)EngineInterfaceServer;

	return ((INetChannelInfo*(__thiscall *)(DWORD, int))*(DWORD*)(*(DWORD*)table + 0x54))(table, entindex);
}

CustomPlayer* CBaseEntity::ToCustomPlayer()
{
	return &AllPlayers[index];
}

int	CBaseEntity::GetHealth()
{
	return ReadInt((DWORD)this + m_iHealth);//*(int*)((DWORD)this + m_iHealth);
}

int CBaseEntity::GetTeam()
{
	return ReadInt((DWORD)this + m_iTeamNum);//*(int*)((DWORD)this + m_iTeamNum);
}

int CBaseEntity::GetFlags()
{
	return ReadInt((DWORD)this + m_fFlags);//*(int*)((DWORD)this + m_fFlags);
}

void CBaseEntity::SetFlags(int flags)
{
	WriteInt((DWORD)this + m_fFlags, flags);
}

int CBaseEntity::GetTickBase()
{
	return ReadInt((DWORD)this + m_nTickBase);//*(int*)((DWORD)this + m_nTickBase);
}

void CBaseEntity::SetTickBase(int base)
{
	WriteInt((DWORD)this + m_nTickBase, base);
}

int CBaseEntity::GetShotsFired()
{
	return ReadInt((DWORD)this + m_iShotsFired);//*(int*)((DWORD)this + m_iShotsFired);
}

int CBaseEntity::GetMoveType()
{
	return ReadInt((DWORD)this + m_nMoveType);//*(int*)((DWORD)this + m_nMoveType);
}

void CBaseEntity::SetMoveType(int type)
{
	WriteInt((DWORD)this + m_nMoveType, type);
}

int CBaseEntity::GetModelIndex()
{
	return ReadInt((DWORD)this + m_nModelIndex);// *(int*)((DWORD)this + m_nModelIndex);
}

void CBaseEntity::SetModelIndex(int index)
{
	WriteInt((DWORD)this + m_nModelIndex, index);
}

int CBaseEntity::GetHitboxSet()
{
	return ReadInt((DWORD)this + m_nHitboxSet);//*(int*)((DWORD)this + m_nHitboxSet);
}

void CBaseEntity::SetHitboxSet(int set)
{
	WriteInt((DWORD)this + m_nHitboxSet, set);
}

int CBaseEntity::GetUserID()
{
	player_info_t info;
	GetPlayerInfo(&info);
	return info.userid; //this->GetPlayerInfo().userid; //DYLAN FIX
}

int CBaseEntity::GetArmor()
{
	return ReadInt((DWORD)this + m_ArmorValue);//*(int*)((DWORD)this + m_ArmorValue);
}

int CBaseEntity::PhysicsSolidMaskForEntity()
{
	typedef unsigned int(__thiscall* OriginalFn)(void*);
	return GetVFunc<OriginalFn>(this, 148)(this); //154
}

CBaseEntity* CBaseEntity::GetOwner()
{
	DWORD Handle = ReadInt((DWORD)this + m_hOwnerEntity);
	return (CBaseEntity*)Interfaces::ClientEntList->GetClientEntityFromHandle(Handle);
}

int CBaseEntity::GetGlowIndex()
{
	return ReadInt((DWORD)this + m_iGlowIndex);//*(int*)((DWORD)this + m_iGlowIndex);
}

float CBaseEntity::GetBombTimer()
{
	return 0.0f;//dylan fix
	//float bombTime = *(float*)((DWORD)this + m_flC4Blow);
	//float returnValue = bombTime - Interfaces::Globals->curtime;
	//return (returnValue < 0) ? 0.f : returnValue;
}

float CBaseEntity::GetFlashDuration()
{
	return ReadFloat((DWORD)this + m_flFlashDuration);//*(float*)((DWORD)this + m_flFlashDuration);
}

void CBaseEntity::SetFlashDuration(float dur)
{
	WriteFloat((DWORD)this + m_flFlashDuration, dur);
}

BOOLEAN CBaseEntity::IsFlashed()
{
	return (BOOLEAN)GetFlashDuration() > 0 ? true : false;
}

BOOL CBaseEntity::GetAlive()
{
	//return (bool)(*(int*)((DWORD)this + m_lifeState) == 0);
	return (BOOL)(ReadInt((DWORD)this + m_lifeState) == 0);
}

BOOLEAN CBaseEntity::GetAliveServer()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, (0x114 / 4))(this);
}

void CBaseEntity::CalcAbsolutePositionServer()
{
#ifdef HOOK_LAG_COMPENSATION
	static DWORD absposfunc = NULL;
	if (!absposfunc)
	{
		const char* sig = "55  8B  EC  83  E4  F0  83  EC  68  56  8B  F1  57  8B  8E  D0  00  00  00";
		absposfunc = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)sig, strlen(sig));
		if (!absposfunc)
			DebugBreak();
	}
	((void(__thiscall*)(CBaseEntity*))absposfunc)(this);
#endif
}

BOOLEAN CBaseEntity::GetDormant()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bDormant);//*(bool*)((DWORD)this + m_bDormant);
}

BOOLEAN CBaseEntity::GetImmune()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bGunGameImmunity);//*(bool*)((DWORD)this + m_bGunGameImmunity);
}

BOOLEAN CBaseEntity::HasHelmet()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bHasHelmet);//*(bool*)((DWORD)this + m_bHasHelmet);
}

BOOLEAN CBaseEntity::HasDefuseKit()
{
	return (BOOLEAN)ReadInt((DWORD)this + m_bHasDefuser);
}

BOOLEAN CBaseEntity::IsDefusing()
{
	return (BOOLEAN)ReadInt((DWORD)this + m_bIsDefusing);
}

bool CBaseEntity::IsTargetingLocal()
{
	Vector src, dst, forward;
	trace_t tr;

	QAngle viewangle = this->GetEyeAngles();

	AngleVectors(viewangle, &forward);
	VectorNormalizeFast(forward);
	forward *= 8142.f;
	src = this->GetEyePosition();
	dst = src + forward;

	Ray_t ray;
	ray.Init(src, dst);

	CTraceFilterPlayersOnlyNoWorld filter;
	filter.AllowTeammates = true;
	filter.pSkip = (IHandleEntity*)this;

	Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

	//UTIL_TraceLine(src, dst, MASK_SHOT, this, &tr);

	if (tr.m_pEnt == LocalPlayer.Entity)
		return true;

	return false;
}

model_t* CBaseEntity::GetModel()
{
	return (model_t*)ReadInt((DWORD)this + 0x6C); //DYLAN TEST THIS  //*(model_t**)((DWORD)this + 0x6C);
}

CStudioHdr* CBaseEntity::GetModelPtr()
{
	return (CStudioHdr*)*(DWORD*)((DWORD)this + m_pStudioHdr);
}

studiohdr_t* CBaseEntity::GetStudioHDR()
{
	return (studiohdr_t*)*(DWORD*)((DWORD)this + m_pStudioHdr2);
}

void CBaseEntity::SetModel(model_t* mod)
{
	WriteInt((DWORD)this + 0x6C, (int)mod);
}

bool CBaseEntity::IsEnemy()
{
	return false;//dylan fix
	//return (this->GetTeam() == G::LocalPlayer.Entity->GetTeam() || this->GetTeam() == 0) ? false : true;
}

bool CBaseEntity::IsVisible(int bone)
{
	/*
	Ray_t ray;
	trace_t tr;
	m_visible = false;
	ray.Init(G::LocalPlayer.Entity->GetEyePosition(), this->GetBonePosition(bone)); // replace with config->aimbone

	CTraceFilter filter;
	filter.pSkip = G::LocalPlayer.Entity;

	I::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

	if (tr.m_pEnt == this)
	{
		m_visible = true;
		return true;
	}

	return false;
	*/ //dylan fix
	return false;
}

BOOLEAN CBaseEntity::IsBroken()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bIsBroken);//*(bool*)((DWORD)this + m_bIsBroken);
}

QAngle CBaseEntity::GetViewPunch()
{
	QAngle tempangle;
	ReadAngle((DWORD)this + m_viewPunchAngle, tempangle);
	return tempangle;
	//return *(QAngle*)((DWORD)this + m_viewPunchAngle);
}

void CBaseEntity::SetViewPunch(QAngle punch)
{
	WriteAngle((DWORD)this + m_viewPunchAngle, punch);
}

QAngle CBaseEntity::GetPunch()
{
	QAngle tempangle;
	ReadAngle((DWORD)this + m_aimPunchAngle, tempangle);
	return tempangle;
	//return *(QAngle*)((DWORD)this + m_aimPunchAngle);
}

void CBaseEntity::SetPunch(QAngle punch)
{
	WriteAngle((DWORD)this + m_aimPunchAngle, punch);
}

Vector CBaseEntity::GetPunchVel()
{
	Vector tempangle;
	ReadVector((DWORD)this + m_aimPunchAngleVel, tempangle);
	return tempangle;
	//return *(QAngle*)((DWORD)this + m_viewPunchAngle);
}

void CBaseEntity::SetPunchVel(Vector vel)
{
	WriteVector((DWORD)this + m_aimPunchAngleVel, vel);
}

QAngle CBaseEntity::GetEyeAngles()
{
	//return ((QAngle(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_OffsetEyeAngles))(this);
	QAngle tempangle;
	ReadAngle((DWORD)this + m_angEyeAngles, tempangle);
	return tempangle;
	//return *(QAngle*)((DWORD)this + m_angEyeAngles);
}

QAngle CBaseEntity::GetEyeAnglesServer()
{
	QAngle tempangle;
	ReadAngle((DWORD)this + m_angEyeAnglesServer, tempangle);
	return tempangle;
}

QAngle* CBaseEntity::EyeAngles()
{
	typedef QAngle*(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, EyeAnglesIndex)(this);
}

void CBaseEntity::SetEyeAngles(QAngle angles)
{
	WriteAngle((DWORD)this + m_angEyeAngles, angles);
}

QAngle CBaseEntity::GetRenderAngles()
{
	QAngle ret;
	QAngle* renderanglesadr = (QAngle*)((DWORD)LocalPlayer.Entity + dwRenderAngles);
	ReadAngle((uintptr_t)renderanglesadr, ret);
	return ret;
}

void CBaseEntity::SetRenderAngles(QAngle angles)
{
	QAngle* renderanglesadr = (QAngle*)((DWORD)LocalPlayer.Entity + dwRenderAngles);
	WriteAngle((uintptr_t)renderanglesadr, angles);
}

Vector CBaseEntity::GetNetworkOrigin()
{
	Vector tempvector;
	ReadVector((DWORD)this + m_vecOrigin, tempvector);
	return tempvector;
	//return *(Vector*)((DWORD)this + m_vecOrigin);
}

Vector CBaseEntity::GetOldOrigin()
{
	return *(Vector*)((DWORD)this + m_vecOldOrigin);
}

Vector CBaseEntity::GetOrigin()
{
	Vector tempvector;
	ReadVector((DWORD)this + m_vecOriginReal, tempvector);
	return tempvector;
	//return ((Vector(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwGetAbsOrigin))(this);
}

void CBaseEntity::SetNetworkOrigin(Vector origin)
{
	WriteVector((DWORD)this + m_vecOrigin, origin);
}

void CBaseEntity::SetOrigin(Vector origin)
{
	WriteVector((DWORD)this + m_vecOriginReal, origin);
}

void CBaseEntity::SetAbsOrigin(const Vector &origin)
{
	((void(__thiscall*)(CBaseEntity*, const Vector&))AdrOf_SetAbsOrigin)(this, origin);
}

QAngle CBaseEntity::GetAbsAngles()
{
#if 0
	__asm {
		mov eax, [ecx]
		call[eax + 0x2C]
		ret 4
	}
#endif
#if 1
	typedef QAngle*(__thiscall* OriginalFn)(void*);
	QAngle* ret = GetVFunc<OriginalFn>(this, (m_dwGetAbsAnglesOffset / 4))(this);
	QAngle retval;
	ReadAngle((uintptr_t)ret, retval);
	return retval;

	//int adr = ReadInt((uintptr_t)this);
	//QAngle test = ((QAngle(__thiscall*)(CBaseEntity* me))ReadInt(adr + m_dwGetAbsAnglesOffset))(this);

	//return test;

#endif
}

QAngle CBaseEntity::GetAbsAnglesServer()
{
	if ((*(DWORD *)((DWORD)this + 0x0D0) >> 11) & 1)
		CalcAbsolutePositionServer();
	return *(QAngle*)((DWORD)this + 0x1E4);
}

Vector CBaseEntity::WorldSpaceCenter()
{
	typedef Vector*(__thiscall* OriginalFn)(void*);
	Vector* ret = GetVFunc<OriginalFn>(this, (m_dwWorldSpaceCenterOffset / 4))(this);
	Vector retval;
	ReadVector((uintptr_t)ret, retval);
	return retval;
}

Vector CBaseEntity::GetEyePosition()
{
	//return ((Vector(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_OffsetEyePos))(this);
	if (this != LocalPlayer.Entity)
	{
		Vector origin = GetOrigin();

		Vector vDuckHullMin = Interfaces::GameMovement->GetPlayerMins(true);
		Vector vStandHullMin = Interfaces::GameMovement->GetPlayerMins(false);

		float fMore = (vDuckHullMin.z - vStandHullMin.z);

		Vector vecDuckViewOffset = Interfaces::GameMovement->GetPlayerViewOffset(true);
		Vector vecStandViewOffset = Interfaces::GameMovement->GetPlayerViewOffset(false);
		float duckFraction = GetDuckAmount();
		//Vector temp = GetViewOffset();
		float tempz = ((vecDuckViewOffset.z - fMore) * duckFraction) +
			(vecStandViewOffset.z * (1 - duckFraction));

		origin.z += tempz;

		return(origin);
	}
	else
	{
		return GetOrigin() + GetViewOffset();
	}
}

BOOLEAN CBaseEntity::ShouldCollide(int collisionGroup, int contentsMask)
{
	return ((BOOLEAN(__thiscall*)(CBaseEntity*, int, int))ReadInt(ReadInt((uintptr_t)this) + m_dwShouldCollide))(this, collisionGroup, contentsMask);
	//int adr = (*(int**)this)[193];
	//adr = adr - (int)ClientHandle;
	//typedef BOOLEAN(__thiscall* OriginalFn)(void*, int, int);
	//return GetVFunc<OriginalFn>(this, 193)(this, collisionGroup, contentsMask);
}

BOOLEAN CBaseEntity::IsTransparent()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(void*);
	return GetVFunc<OriginalFn>(this, 50)(this);
}

SolidType_t CBaseEntity::GetSolid()
{
	return ((SolidType_t(__thiscall*)(CBaseEntity*))ReadInt(ReadInt((uintptr_t)this) + m_dwGetSolid))(this); //(SolidType_t)ReadInt((uintptr_t)this + m_nSolidType);
}

Vector CBaseEntity::GetBonePosition(int HitboxID, float time, bool ForceSetup, bool StoreInCache, StoredNetvars *Record)
{
	//((CBaseEntity*)this->GetBaseAnimating())->InvalidateBoneCache();

	CustomPlayer *pCPlayer = nullptr;
	bool bCheckBodyYawIndex = false;
	if (IsPlayer())
	{
		pCPlayer = &AllPlayers[index];
		if (pCPlayer->PersistentData.ShouldResolve())
			bCheckBodyYawIndex = true;
	}

	CBaseAnimating* anim = this->GetBaseAnimating();

	if (anim)
	{
		matrix3x4_t *destBoneMatrixes;
		matrix3x4_t tempBoneMatrixes[MAXSTUDIOBONES];

		//if (!Record)
		{
			ForceSetup = true;
			StoreInCache = false;
		}

#if 1
		destBoneMatrixes = tempBoneMatrixes;
		if (pCPlayer)
		{
			if (pCPlayer->bAlreadyCachedBonesThisTick)
				((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
			else
			{
				((CBaseEntity*)anim)->InvalidateBoneCache();
				((CBaseEntity*)anim)->SetLastOcclusionCheckFlags(0);
				((CBaseEntity*)anim)->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
				((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time);
				pCPlayer->bAlreadyCachedBonesThisTick = true;
			}
		}
		else
		{
			((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
		}
#else
		if (!ForceSetup && Record->bCachedBones && (!bCheckBodyYawIndex || pCPlayer->PersistentData.correctedbodyyawindex == Record->corrected_body_yaw_index))
		{
			//Use cached bones
			destBoneMatrixes = Record->CachedBoneMatrixes;
			if (!pCPlayer->bUsedCachedBonesThisTick)
			{
				CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();
				if (!pBoneSetupLock->TryLock())
				{
					pBoneSetupLock->Lock();
				}
				CBoneAccessor *accessor = GetBoneAccessor();
				CUtlVectorSimple *CachedBones = GetCachedBoneData();

				//Backup existing game bone data
				pCPlayer->iBackupGameCachedBonesCount = CachedBones->Count();
				pCPlayer->iBackupGameReadableBones = accessor->GetReadableBones();
				pCPlayer->iBackupGameWritableBones = accessor->GetWritableBones();
				memcpy(pCPlayer->BackupGameCachedBoneMatrixes, CachedBones->Base(), sizeof(matrix3x4_t) * CachedBones->Count());

				//Now set the games data to our cached data for traceray
				//accessor->SetReadableBones(Record->iReadableBones);
				//accessor->SetWritableBones(Record->iWritableBones);
				CachedBones->count = Record->iCachedBonesCount;
				memcpy(CachedBones->Base(), Record->CachedBoneMatrixes, sizeof(matrix3x4_t) * Record->iCachedBonesCount);
				pCPlayer->bUsedCachedBonesThisTick = true;
				pBoneSetupLock->Unlock();
			}
		}
		else
		{
			if (StoreInCache)
			{
				destBoneMatrixes = Record->CachedBoneMatrixes;
				((CBaseEntity*)anim)->SetupBonesRebuiltSimple(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time);
				CBoneAccessor *accessor = GetBoneAccessor();
				Record->iCachedBonesCount = GetCachedBoneData()->count;
				Record->iReadableBones = accessor->GetReadableBones();
				Record->iWritableBones = accessor->GetWritableBones();
				Record->iTickCachedBones = Interfaces::Globals->tickcount;
				Record->bCachedBones = true;
				if (pCPlayer)
					Record->corrected_body_yaw_index = pCPlayer->PersistentData.correctedbodyyawindex;
			}
			else
			{
				//Use a temporary matrix buffer
				destBoneMatrixes = tempBoneMatrixes;
				((CBaseEntity*)anim)->SetupBonesRebuilt(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
			}
		}
#endif


		const model_t* mod = this->GetModel();
		if (mod)
		{
			studiohdr_t* hdr = Interfaces::ModelInfoClient->GetStudioModel(mod);
			if (hdr)
			{
				mstudiohitboxset_t* set = hdr->pHitboxSet(anim->GetHitboxSet());
				if (set)
				{
					mstudiobbox_t* hitbox = set->pHitbox(HitboxID);
					if (hitbox)
					{
						Vector vMin, vMax, vCenter;
						//matrix3x4a_t* scaled = static_cast<matrix3x4a_t*>(boneMatrixes);
						VectorTransformZ(hitbox->bbmin, destBoneMatrixes[hitbox->bone], vMin);
						VectorTransformZ(hitbox->bbmax, destBoneMatrixes[hitbox->bone], vMax);
						vCenter = (vMin + vMax) * 0.5f;
						return vCenter;
					}
				}
			}
		}
	}
	return vecZero;
}

Vector CBaseEntity::GetBonePositionTotal(int HitboxID, float time, Vector &BoneMax, Vector &BoneMin, bool ForceSetup, bool StoreInCache, StoredNetvars *Record)
{
	CBaseAnimating* anim = this->GetBaseAnimating();
	CustomPlayer *pCPlayer = nullptr;
	bool bCheckBodyYawIndex = false;
	if (IsPlayer())
	{
		pCPlayer = &AllPlayers[index];
		if (pCPlayer->PersistentData.ShouldResolve())
			bCheckBodyYawIndex = true;
	}

	if (anim)
	{
		matrix3x4_t *destBoneMatrixes;
		matrix3x4_t tempBoneMatrixes[MAXSTUDIOBONES];

		//if (!Record)
		{
			ForceSetup = true;
			StoreInCache = false;
		}

#if 1
		destBoneMatrixes = tempBoneMatrixes;
		if (pCPlayer)
		{
			if (pCPlayer->bAlreadyCachedBonesThisTick)
				((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
			else
			{
				((CBaseEntity*)anim)->InvalidateBoneCache();
				((CBaseEntity*)anim)->SetLastOcclusionCheckFlags(0);
				((CBaseEntity*)anim)->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
				((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time);
				pCPlayer->bAlreadyCachedBonesThisTick = true;
			}
		}
		else
		{
			((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
		}
#else
		if (!ForceSetup && Record->bCachedBones && (!bCheckBodyYawIndex || pCPlayer->PersistentData.correctedbodyyawindex == Record->corrected_body_yaw_index))
		{
			//Use cached bones
			destBoneMatrixes = Record->CachedBoneMatrixes;
			if (!pCPlayer->bUsedCachedBonesThisTick)
			{
				CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();
				if (!pBoneSetupLock->TryLock())
				{
					pBoneSetupLock->Lock();
				}
				CBoneAccessor *accessor = GetBoneAccessor();
				CUtlVectorSimple *CachedBones = GetCachedBoneData();

				//Backup existing game bone data
				pCPlayer->iBackupGameCachedBonesCount = CachedBones->Count();
				pCPlayer->iBackupGameReadableBones = accessor->GetReadableBones();
				pCPlayer->iBackupGameWritableBones = accessor->GetWritableBones();
				memcpy(pCPlayer->BackupGameCachedBoneMatrixes, CachedBones->Base(), sizeof(matrix3x4_t) * CachedBones->Count());

				//Now set the games data to our cached data for traceray
				//accessor->SetReadableBones(Record->iReadableBones);
				//accessor->SetWritableBones(Record->iWritableBones);
				CachedBones->count = Record->iCachedBonesCount;
				memcpy(CachedBones->Base(), Record->CachedBoneMatrixes, sizeof(matrix3x4_t) * Record->iCachedBonesCount);
				pCPlayer->bUsedCachedBonesThisTick = true;
				pBoneSetupLock->Unlock();
			}
		}
		else
		{
			if (StoreInCache)
			{
				destBoneMatrixes = Record->CachedBoneMatrixes;
				((CBaseEntity*)anim)->SetupBonesRebuiltSimple(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time);
				CBoneAccessor *accessor = GetBoneAccessor();
				Record->iCachedBonesCount = GetCachedBoneData()->count;
				Record->iReadableBones = accessor->GetReadableBones();
				Record->iWritableBones = accessor->GetWritableBones();
				Record->iTickCachedBones = Interfaces::Globals->tickcount;
				Record->bCachedBones = true;
				if (pCPlayer)
					Record->corrected_body_yaw_index = pCPlayer->PersistentData.correctedbodyyawindex;
			}
			else
			{
				//Use a temporary matrix buffer
				destBoneMatrixes = tempBoneMatrixes;
				((CBaseEntity*)anim)->SetupBonesRebuilt(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
			}
		}
#endif

		const model_t* mod = this->GetModel();
		if (mod)
		{
			studiohdr_t* hdr = Interfaces::ModelInfoClient->GetStudioModel(mod);
			if (hdr)
			{
				mstudiohitboxset_t* set = hdr->pHitboxSet(anim->GetHitboxSet());
				if (set)
				{
					mstudiobbox_t* hitbox = set->pHitbox(HitboxID);
					if (hitbox)
					{
						VectorTransformZ(hitbox->bbmin, destBoneMatrixes[hitbox->bone], BoneMin);
						VectorTransformZ(hitbox->bbmax, destBoneMatrixes[hitbox->bone], BoneMax);
						Vector vCenter = (BoneMin + BoneMax) * 0.5f;
						return vCenter;
					}
				}
			}
		}
	}
	return vecZero;
}


void CBaseEntity::GetBonePosition(int iBone, Vector &origin, QAngle &angles, float time, bool ForceSetup, bool StoreInCache, StoredNetvars *Record)
{
	//WriteInt((uintptr_t)this + AdrOf_m_nWritableBones, 0);
	//WriteInt((uintptr_t)this + (AdrOf_m_nWritableBones - 4), 0); //Readable bones
	//WriteInt((uintptr_t)this + AdrOf_m_iDidCheckForOcclusion, reinterpret_cast<int*>(AdrOf_m_dwOcclusionArray)[1]);
	//((CBaseEntity*)this->GetBaseAnimating())->InvalidateBoneCache();

	CBaseAnimating* anim = this->GetBaseAnimating();
	CustomPlayer *pCPlayer = nullptr;
	bool bCheckBodyYawIndex = false;
	if (IsPlayer())
	{
		pCPlayer = &AllPlayers[index];
		if (pCPlayer->PersistentData.ShouldResolve())
		{
			bCheckBodyYawIndex = true;
		}
	}

	if (anim)
	{
		matrix3x4_t *destBoneMatrixes;
		matrix3x4_t tempBoneMatrixes[MAXSTUDIOBONES];

		//if (!Record)
		{
			ForceSetup = true;
			StoreInCache = false;
		}

#if 1
		destBoneMatrixes = tempBoneMatrixes;
		if (pCPlayer)
		{
			if (pCPlayer->bAlreadyCachedBonesThisTick)
				((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
			else
			{
				((CBaseEntity*)anim)->InvalidateBoneCache();
				((CBaseEntity*)anim)->SetLastOcclusionCheckFlags(0);
				((CBaseEntity*)anim)->SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);
				((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time);
				pCPlayer->bAlreadyCachedBonesThisTick = true;
			}
		}
		else
		{
			((CBaseEntity*)anim)->SetupBones(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
		}
#else
		if (!ForceSetup && Record->bCachedBones && (!bCheckBodyYawIndex || pCPlayer->PersistentData.correctedbodyyawindex == Record->corrected_body_yaw_index))
		{
			//Use cached bones
			destBoneMatrixes = Record->CachedBoneMatrixes;
			if (!pCPlayer->bUsedCachedBonesThisTick)
			{
				CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();
				if (!pBoneSetupLock->TryLock())
				{
					pBoneSetupLock->Lock();
				}
				CBoneAccessor *accessor = GetBoneAccessor();
				CUtlVectorSimple *CachedBones = GetCachedBoneData();

				//Backup existing game bone data
				pCPlayer->iBackupGameCachedBonesCount = CachedBones->Count();
				pCPlayer->iBackupGameReadableBones = accessor->GetReadableBones();
				pCPlayer->iBackupGameWritableBones = accessor->GetWritableBones();
				memcpy(pCPlayer->BackupGameCachedBoneMatrixes, CachedBones->Base(), sizeof(matrix3x4_t) * CachedBones->Count());

				//Now set the games data to our cached data for traceray
				//accessor->SetReadableBones(Record->iReadableBones);
				//accessor->SetWritableBones(Record->iWritableBones);
				CachedBones->count = Record->iCachedBonesCount;
				memcpy(CachedBones->Base(), Record->CachedBoneMatrixes, sizeof(matrix3x4_t) * Record->iCachedBonesCount);
				pCPlayer->bUsedCachedBonesThisTick = true;
				pBoneSetupLock->Unlock();
			}
		}
		else
		{
			if (StoreInCache)
			{
				destBoneMatrixes = Record->CachedBoneMatrixes;
				((CBaseEntity*)anim)->SetupBonesRebuiltSimple(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time);
				CBoneAccessor *accessor = GetBoneAccessor();
				Record->iCachedBonesCount = GetCachedBoneData()->count;
				Record->iReadableBones = accessor->GetReadableBones();
				Record->iWritableBones = accessor->GetWritableBones();
				Record->iTickCachedBones = Interfaces::Globals->tickcount;
				Record->bCachedBones = true;
				if (pCPlayer)
					Record->corrected_body_yaw_index = pCPlayer->PersistentData.correctedbodyyawindex;
			}
			else
			{
				//Use a temporary matrix buffer
				destBoneMatrixes = tempBoneMatrixes;
				((CBaseEntity*)anim)->SetupBonesRebuilt(destBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, time);
			}
		}
#endif

		const model_t* mod = this->GetModel();
		if (mod)
		{
			studiohdr_t* hdr = Interfaces::ModelInfoClient->GetStudioModel(mod);
			if (hdr)
			{
				mstudiohitboxset_t* set = hdr->pHitboxSet(anim->GetHitboxSet());
				if (set)
				{
					mstudiobbox_t* hitbox = set->pHitbox(iBone);
					if (hitbox)
					{
						Vector vMin, vMax;
						VectorTransformZ(hitbox->bbmin, destBoneMatrixes[hitbox->bone], vMin);
						VectorTransformZ(hitbox->bbmax, destBoneMatrixes[hitbox->bone], vMax);
						origin = (vMin + vMax) * 0.5f;
						MatrixAngles(destBoneMatrixes[hitbox->bone], angles, origin);
					}
				}
			}
		}
	}
}

Vector CBaseEntity::GetBonePositionCachedOnly(int HitboxID, matrix3x4_t* matrixes)
{
	CBaseAnimating* anim = this->GetBaseAnimating();

	if (anim)
	{
		const model_t* mod = this->GetModel();
		if (mod)
		{
			studiohdr_t* hdr = Interfaces::ModelInfoClient->GetStudioModel(mod);
			if (hdr)
			{
				mstudiohitboxset_t* set = hdr->pHitboxSet(anim->GetHitboxSet());
				if (set)
				{
					mstudiobbox_t* hitbox = set->pHitbox(HitboxID);
					if (hitbox)
					{
						Vector vMin, vMax;
						VectorTransformZ(hitbox->bbmin, matrixes[hitbox->bone], vMin);
						VectorTransformZ(hitbox->bbmax, matrixes[hitbox->bone], vMax);
						Vector vCenter = (vMin + vMax) * 0.5f;
						return vCenter;
					}
				}
			}
		}
	}
	return vecZero;
}

//returns true if current bones are cached
bool CBaseEntity::CacheCurrentBones(float flTime, bool bForce)
{
	CustomPlayer *pCPlayer = &AllPlayers[index];
	StoredNetvars* CurrentRecord = pCPlayer->GetCurrentRecord();

	if (!CurrentRecord)
		return false;

	bool bCheckBodyYawIndex = false;
	if (pCPlayer->PersistentData.ShouldResolve())
		bCheckBodyYawIndex = true;

	if (!CurrentRecord->bCachedBones || bForce || (bCheckBodyYawIndex && pCPlayer->PersistentData.correctedbodyyawindex != CurrentRecord->corrected_body_yaw_index))
	{
		SetupBones(CurrentRecord->CachedBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, flTime);
		CBoneAccessor *accessor = GetBoneAccessor();
		CurrentRecord->iCachedBonesCount = GetCachedBoneData()->count;
		CurrentRecord->iReadableBones = accessor->GetReadableBones();
		CurrentRecord->iWritableBones = accessor->GetWritableBones();
		CurrentRecord->iTickCachedBones = Interfaces::Globals->tickcount;
		CurrentRecord->bCachedBones = true;
	}

	return true;
}

//returns true if we cached
bool CBaseEntity::CacheBones(float flTime, bool bForce, StoredNetvars *Record)
{
	if (Record)
	{
		CustomPlayer *pCPlayer = &AllPlayers[index];
		bool bCheckBodyYawIndex = false;
		if (pCPlayer->PersistentData.ShouldResolve())
			bCheckBodyYawIndex = true;

		if (!Record->bCachedBones || bForce || (bCheckBodyYawIndex && pCPlayer->PersistentData.correctedbodyyawindex != Record->corrected_body_yaw_index))
		{
			SetupBones(Record->CachedBoneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, flTime);
			CBoneAccessor *accessor = GetBoneAccessor();
			Record->iCachedBonesCount = GetCachedBoneData()->count;
			Record->iReadableBones = accessor->GetReadableBones();
			Record->iWritableBones = accessor->GetWritableBones();
			Record->iTickCachedBones = Interfaces::Globals->tickcount;
			Record->bCachedBones = true;
			return true;
		}
	}
	return false;
}

bool CBaseEntity::IsViewModel()
{
	using OriginalFn = bool(__thiscall*)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, IsViewModel_index)(this);
}

float CBaseEntity::LastBoneChangedTime()
{
	using OriginalFn = float(__thiscall*)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, LastBoneChangedTime_Offset)(this);
}

bool CBaseEntity::SetupBonesRebuiltNoCache(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
#ifdef _DEBUG
	if (!IsBoneAccessAllowed())
		DebugBreak();
#endif

	boneMask = boneMask | 0x80000; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.

	if (GetSequence() != -1)
	{
		CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();

		if (*g_bInThreadedBoneSetup)
		{
			if (!pBoneSetupLock->TryLock())
			{
				// someone else is handling
				return false;
			}
			// else, we have the lock
		}
		else
		{
			pBoneSetupLock->Lock();
		}

		// If we're setting up LOD N, we have set up all lower LODs also
		// because lower LODs always use subsets of the bones of higher LODs.
		int nLOD = 0;
		int nMask = BONE_USED_BY_VERTEX_LOD0;
		for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
		{
			if (boneMask & nMask)
				break;
		}
		for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
		{
			boneMask |= nMask;
		}

		// A bit of a hack, but this way when in prediction we use the "correct" gpGlobals->curtime -- rather than the
		// one that the player artificially advances
		using InPredictionFn = bool(__thiscall*)(DWORD);

		if (GetPredictable() &&
			GetVFunc<InPredictionFn>((PVOID)Interfaces::Prediction, 14) ((DWORD)Interfaces::Prediction)) //Interfaces::Prediction->InPrediction())
		{
			currentTime = *(float*)((DWORD)Interfaces::Prediction + 0x5C);//Interfaces::Prediction->GetSavedTime();
		}

		DWORD hdr = (DWORD)GetModelPtr();
		if (hdr)//&& hdr->SequencesAvailable()) //TODO:
		{
			CBoneAccessor* m_BoneAccessor = GetBoneAccessor();

			matrix3x4a_t TempBoneCache[MAXSTUDIOBONES];
			int backup_readablebones = m_BoneAccessor->GetReadableBones();
			int backup_writablebones = m_BoneAccessor->GetWritableBones();
			matrix3x4a_t *backup_matrix = m_BoneAccessor->GetBoneArrayForWrite();
			int backup_previousbonemask = GetPreviousBoneMask();
			int backup_mostrecentmodelbonecounter = GetMostRecentModelBoneCounter();
			int backup_occlusionflags = GetLastOcclusionCheckFlags();
			int backup_occlusionframecount = GetLastOcclusionCheckFrameCount();
			int backup_lastsetupbonesframecount = GetLastSetupBonesFrameCount();


			m_BoneAccessor->SetBoneArrayForWrite(&TempBoneCache[0]);
			m_BoneAccessor->SetReadableBones(0);
			m_BoneAccessor->SetWritableBones(0);

			SetPreviousBoneMask(GetAccumulatedBoneMask());
			SetAccumulatedBoneMask(0);
			SetAccumulatedBoneMask(GetAccumulatedBoneMask() | boneMask);
			SetMostRecentModelBoneCounter(*g_iModelBoneCounter);
			MDLCACHE_CRITICAL_SECTION();

			CBaseEntity* renderable = (CBaseEntity*)((DWORD)this + 4);

			// Setup our transform based on render angles and origin.
			__declspec (align(16)) matrix3x4_t parentTransform;
			AngleMatrix(GetAbsAngles(), *GetAbsOrigin(), parentTransform);

			// Load the boneMask with the total of what was asked for last frame.
			//boneMask |= GetPreviousBoneMask();

			// Allow access to the bones we're setting up so we don't get asserts in here.
			int oldReadableBones = m_BoneAccessor->GetReadableBones();
			int oldWritableBones = m_BoneAccessor->GetWritableBones();
			m_BoneAccessor->SetWritableBones(oldReadableBones | boneMask);
			m_BoneAccessor->SetReadableBones(m_BoneAccessor->GetWritableBones());

			//if (!(hdr->flags() & STUDIOHDR_FLAGS_STATIC_PROP))
			if (!(*(char *)(*(DWORD *)hdr + 152) & 0x10))
			{
				// This is necessary because it's possible that CalculateIKLocks will trigger our move children
				// to call GetAbsOrigin(), and they'll use our OLD bone transforms to get their attachments
				// since we're right in the middle of setting up our new transforms. 
				//
				// Setting this flag forces move children to keep their abs transform invalidated.
				*(int*)((DWORD)renderable + 224) |= 8;//AddFlag( EFL_SETTING_UP_BONES ); (1 << 3));

				DWORD m_pIk = (DWORD)GetIK();

				// only allocate an ik block if the npc can use it
				//if (!m_pIk && hdr->numikchains() > 0 && !(m_EntClientFlags & ENTCLIENTFLAG_DONTUSEIK))
				if (!m_pIk && *(DWORD *)(*(DWORD *)hdr + 284) > 0 && !(GetEntClientFlags() & 2))
				{
					m_pIk = Wrap_CreateIK();
					SetIK(m_pIk);
				}

				Vector pos[MAXSTUDIOBONES];
				QuaternionAligned q[MAXSTUDIOBONES];
#ifdef _DEBUG
				// Having these uninitialized means that some bugs are very hard
				// to reproduce. A memset of 0xFF is a simple way of getting NaNs.
				memset(pos, 0xFF, sizeof(pos));
				memset(q, 0xFF, sizeof(q));
#endif

				if (m_pIk)
				{
					if (Teleported(this) || GetEffects() & EF_NOINTERP)
					{
						ClearTargets(m_pIk);
					}
					Wrap_IKInit(GetIK(), (CStudioHdr*)hdr, GetAbsAngles(), *GetAbsOrigin(), currentTime, Interfaces::Globals->framecount, boneMask);
				}

				int v115;
				int tempBoneMask;

				//bool bSkipAnimFrame = ShouldSkipAnimationFrame(this);

				v115 = *(DWORD*)((DWORD)this + 2596);
				int tmpmask = boneMask;
				if (v115 != -1)
					tmpmask = boneMask & v115;

				tempBoneMask = tmpmask | 0x80000;

#ifdef _DEBUG
				if (UnknownSetupBonesCall())
					DebugBreak();
#endif

				SetLastOcclusionCheckFlags(0);
				SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);

				StandardBlendingRules((CStudioHdr*)hdr, pos, q, currentTime, tempBoneMask);

				DWORD Unknown[2048];
				memset(Unknown, 0x0, sizeof(Unknown));

				if ((bool)(*(int(__thiscall **)(CBaseEntity*))(*(DWORD *)((DWORD)this) + 608))(this))
				{
					if (tempBoneMask != boneMask)
					{
						DWORD v108 = *(DWORD *)hdr;
						if (*(DWORD *)(*(DWORD *)hdr + 156) > 0)
						{
							DWORD v68 = *(DWORD*)(hdr + 48);
							DWORD qptr = (DWORD)&q[0];
							DWORD v69 = (DWORD)((DWORD)renderable + 2660);
							DWORD v97 = *(DWORD *)(hdr + 48);
							DWORD v70 = 0;
							DWORD v71 = (DWORD)((DWORD)renderable + 5724);
							DWORD v72 = (DWORD)&Unknown[255];

							do
							{
								if (!(*(DWORD*)v68 & 0x80300))
								{
									*(DWORD*)(v72 - 8) = *(DWORD*)(v69 - 8);
									*(DWORD*)(v72 - 4) = *(DWORD*)(v69 - 4);
									*(DWORD *)v72 = *(DWORD*)v69;
									for (int i = 0; i < 16; i += 4)
									{
										*(DWORD *)(qptr + i) = *(DWORD*)(v71 + i);
									}
									v68 = v97;
								}
								qptr += 16;
								v97 = v68 + 4;
								++v70;
								v69 += 12;
								v72 += 12;
								v71 += 16;
								v68 += 4;
							} while (v70 < *(DWORD *)(v108 + 156));
						}
					}
				}

				SetLastSetupBonesFrameCount(Interfaces::Globals->framecount);

				byte computed[256] = { 0 };

				// don't calculate IK on ragdolls
				if (GetIK() && !IsRagdoll())
				{
					UpdateIKLocks(currentTime);
					Wrap_UpdateTargets(m_pIk, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
					CalculateIKLocks(currentTime);
					Wrap_SolveDependencies(m_pIk, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
					if (GetUnknownSetupBonesInt() && (boneMask & 0x400))
					{
						DoUnknownSetupBonesCall((CStudioHdr*)hdr, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), &computed[0], (DWORD*)GetIK());
					}
				}

				BuildTransformations((CStudioHdr*)hdr, pos, q, parentTransform, boneMask, &computed[0]);

				*(int*)((DWORD)renderable + 224) &= ~(8);

				ControlMouth((CStudioHdr*)hdr);

				memcpy((void *)((DWORD)renderable + 2652), &pos[0], 12 * (*(DWORD *)(*(DWORD *)hdr + 156)));
				memcpy((void *)((DWORD)renderable + 5724), &q[0], 16 * (*(DWORD *)(*(DWORD *)hdr + 156)));
			}
			else
			{
				MatrixCopy(parentTransform, m_BoneAccessor->GetBoneForWrite(0));
			}

			if (!(oldReadableBones & 0x200) && boneMask & 0x200)
				Wrap_AttachmentHelper((CStudioHdr*)hdr);

			// Do they want to get at the bone transforms? If it's just making sure an aiment has 
			// its bones setup, it doesn't need the transforms yet.
			if (pBoneToWorldOut)
			{
				CUtlVectorSimple* CachedBoneData = GetCachedBoneData();
				if ((unsigned int)nMaxBones >= CachedBoneData->count)
				{
					memcpy((void*)pBoneToWorldOut, TempBoneCache, sizeof(matrix3x4_t) * CachedBoneData->Count());
				}
			}

			m_BoneAccessor->SetReadableBones(backup_readablebones);
			m_BoneAccessor->SetWritableBones(backup_writablebones);
			m_BoneAccessor->SetBoneArrayForWrite(backup_matrix);
			SetPreviousBoneMask(backup_previousbonemask);
			SetMostRecentModelBoneCounter(backup_mostrecentmodelbonecounter);
			SetLastOcclusionCheckFlags(backup_occlusionflags);
			SetLastOcclusionCheckFrameCount(backup_occlusionframecount);
			SetLastSetupBonesFrameCount(backup_lastsetupbonesframecount);

			pBoneSetupLock->Unlock();
			return true;
		}
		pBoneSetupLock->Unlock();
	}
	return false;
}

bool CBaseEntity::SetupBonesRebuiltSimple(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
#ifdef _DEBUG
	if (!IsBoneAccessAllowed())
		DebugBreak();
#endif

	boneMask = boneMask | 0x80000; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.

	if (GetSequence() != -1)
	{
		CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();

		if (*g_bInThreadedBoneSetup)
		{
			if (!pBoneSetupLock->TryLock())
			{
				// someone else is handling
				return false;
			}
			// else, we have the lock
		}
		else
		{
			pBoneSetupLock->Lock();
		}

		// If we're setting up LOD N, we have set up all lower LODs also
		// because lower LODs always use subsets of the bones of higher LODs.
		int nLOD = 0;
		int nMask = BONE_USED_BY_VERTEX_LOD0;
		for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
		{
			if (boneMask & nMask)
				break;
		}
		for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
		{
			boneMask |= nMask;
		}

		// A bit of a hack, but this way when in prediction we use the "correct" gpGlobals->curtime -- rather than the
		// one that the player artificially advances
		using InPredictionFn = bool(__thiscall*)(DWORD);

		if (GetPredictable() &&
			GetVFunc<InPredictionFn>((PVOID)Interfaces::Prediction, 14) ((DWORD)Interfaces::Prediction)) //Interfaces::Prediction->InPrediction())
		{
			currentTime = *(float*)((DWORD)Interfaces::Prediction + 0x5C);//Interfaces::Prediction->GetSavedTime();
		}

		CBoneAccessor* m_BoneAccessor = GetBoneAccessor();

		m_BoneAccessor->SetReadableBones(0);
		m_BoneAccessor->SetWritableBones(0);
		SetLastBoneSetupTime(currentTime);
		SetPreviousBoneMask(GetAccumulatedBoneMask());
		SetAccumulatedBoneMask(0);
		SetAccumulatedBoneMask(GetAccumulatedBoneMask() | boneMask);
		SetMostRecentModelBoneCounter(*g_iModelBoneCounter);
		MDLCACHE_CRITICAL_SECTION();

		DWORD hdr = (DWORD)GetModelPtr();
		if (hdr)//&& hdr->SequencesAvailable()) //TODO:
		{
			CBaseEntity* renderable = (CBaseEntity*)((DWORD)this + 4);

			// Setup our transform based on render angles and origin.
			__declspec (align(16)) matrix3x4_t parentTransform;
			AngleMatrix(GetAbsAngles(), *GetAbsOrigin(), parentTransform);

			// Load the boneMask with the total of what was asked for last frame.
			//boneMask |= GetPreviousBoneMask();

			// Allow access to the bones we're setting up so we don't get asserts in here.
			int oldReadableBones = m_BoneAccessor->GetReadableBones();
			int oldWritableBones = m_BoneAccessor->GetWritableBones();
			m_BoneAccessor->SetWritableBones(oldReadableBones | boneMask);
			m_BoneAccessor->SetReadableBones(m_BoneAccessor->GetWritableBones());

			//if (!(hdr->flags() & STUDIOHDR_FLAGS_STATIC_PROP))
			if (!(*(char *)(*(DWORD *)hdr + 152) & 0x10))
			{
				// This is necessary because it's possible that CalculateIKLocks will trigger our move children
				// to call GetAbsOrigin(), and they'll use our OLD bone transforms to get their attachments
				// since we're right in the middle of setting up our new transforms. 
				//
				// Setting this flag forces move children to keep their abs transform invalidated.
				*(int*)((DWORD)renderable + 224) |= 8;//AddFlag( EFL_SETTING_UP_BONES ); (1 << 3));

				DWORD m_pIk = (DWORD)GetIK();

				// only allocate an ik block if the npc can use it
				//if (!m_pIk && hdr->numikchains() > 0 && !(m_EntClientFlags & ENTCLIENTFLAG_DONTUSEIK))
				if (!m_pIk && *(DWORD *)(*(DWORD *)hdr + 284) > 0 && !(GetEntClientFlags() & 2))
				{
					m_pIk = Wrap_CreateIK();
					SetIK(m_pIk);
				}

				Vector pos[MAXSTUDIOBONES];
				QuaternionAligned q[MAXSTUDIOBONES];
#ifdef _DEBUG
				// Having these uninitialized means that some bugs are very hard
				// to reproduce. A memset of 0xFF is a simple way of getting NaNs.
				memset(pos, 0xFF, sizeof(pos));
				memset(q, 0xFF, sizeof(q));
#endif

				if (m_pIk)
				{
					if (Teleported(this) || GetEffects() & EF_NOINTERP)
					{
						ClearTargets(m_pIk);
					}
					Wrap_IKInit(GetIK(), (CStudioHdr*)hdr, GetAbsAngles(), *GetAbsOrigin(), currentTime, Interfaces::Globals->framecount, boneMask);
				}

				int v115;
				int tempBoneMask;

				//bool bSkipAnimFrame = ShouldSkipAnimationFrame(this);

				v115 = *(DWORD*)((DWORD)this + 2596);
				int tmpmask = boneMask;
				if (v115 != -1)
					tmpmask = boneMask & v115;

				tempBoneMask = tmpmask | 0x80000;

#ifdef _DEBUG
				if (UnknownSetupBonesCall())
					DebugBreak();
#endif

				SetLastOcclusionCheckFlags(0);
				SetLastOcclusionCheckFrameCount(Interfaces::Globals->framecount);

				StandardBlendingRules((CStudioHdr*)hdr, pos, q, currentTime, tempBoneMask);

				DWORD Unknown[2048];

#if 1
				if ((bool)(*(int(__thiscall **)(CBaseEntity*))(*(DWORD *)((DWORD)this) + 608))(this))
				{
					if (tempBoneMask != boneMask)
					{
						DWORD v108 = *(DWORD *)hdr;
						if (*(DWORD *)(*(DWORD *)hdr + 156) > 0)
						{
							DWORD v68 = *(DWORD*)(hdr + 48);
							DWORD qptr = (DWORD)&q[0];
							DWORD v69 = (DWORD)((DWORD)renderable + 2660);
							DWORD v97 = *(DWORD *)(hdr + 48);
							DWORD v70 = 0;
							DWORD v71 = (DWORD)((DWORD)renderable + 5724);
							DWORD v72 = (DWORD)&Unknown[255];

							do
							{
								if (!(*(DWORD*)v68 & 0x80300))
								{
									*(DWORD*)(v72 - 8) = *(DWORD*)(v69 - 8);
									*(DWORD*)(v72 - 4) = *(DWORD*)(v69 - 4);
									*(DWORD *)v72 = *(DWORD*)v69;
									for (int i = 0; i < 16; i += 4)
									{
										*(DWORD *)(qptr + i) = *(DWORD*)(v71 + i);
									}
									v68 = v97;
								}
								qptr += 16;
								v97 = v68 + 4;
								++v70;
								v69 += 12;
								v72 += 12;
								v71 += 16;
								v68 += 4;
							} while (v70 < *(DWORD *)(v108 + 156));
						}
					}
				}
#endif

				SetLastSetupBonesFrameCount(Interfaces::Globals->framecount);

				byte computed[256] = { 0 };

				// don't calculate IK on ragdolls
				if (GetIK() && !IsRagdoll())
				{
					UpdateIKLocks(currentTime);
					Wrap_UpdateTargets(m_pIk, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
					CalculateIKLocks(currentTime);
					Wrap_SolveDependencies(m_pIk, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
					if (GetUnknownSetupBonesInt() && (boneMask & 0x400))
					{
						DoUnknownSetupBonesCall((CStudioHdr*)hdr, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), &computed[0], (DWORD*)GetIK());
					}
				}

				BuildTransformations((CStudioHdr*)hdr, pos, q, parentTransform, boneMask, &computed[0]);

				*(int*)((DWORD)renderable + 224) &= ~(8);

				ControlMouth((CStudioHdr*)hdr);

				memcpy((void *)((DWORD)renderable + 2652), &pos[0], 12 * (*(DWORD *)(*(DWORD *)hdr + 156)));
				memcpy((void *)((DWORD)renderable + 5724), &q[0], 16 * (*(DWORD *)(*(DWORD *)hdr + 156)));
			}
			else
			{
				MatrixCopy(parentTransform, m_BoneAccessor->GetBoneForWrite(0));
			}

			if (!(oldReadableBones & 0x200) && boneMask & 0x200)
				Wrap_AttachmentHelper((CStudioHdr*)hdr);

			// Do they want to get at the bone transforms? If it's just making sure an aiment has 
			// its bones setup, it doesn't need the transforms yet.
			if (pBoneToWorldOut)
			{
				CUtlVectorSimple* CachedBoneData = GetCachedBoneData();
				if ((unsigned int)nMaxBones >= CachedBoneData->count)
				{
					memcpy((void*)pBoneToWorldOut, CachedBoneData->Base(), sizeof(matrix3x4_t) * CachedBoneData->count);
				}
			}
			pBoneSetupLock->Unlock();
			return true;
		}
		pBoneSetupLock->Unlock();
	}
	return false;
}

bool CBaseEntity::SetupBonesRebuilt(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
#ifdef _DEBUG
	if (!IsBoneAccessAllowed())
	{
		static float lastWarning = 0.0f;

		// Prevent spammage!!!
		if (Interfaces::Globals->realtime >= lastWarning + 1.0f)
		{
			AllocateConsole();
			printf("*** ERROR: Bone access not allowed (entity %i:%s)\n", index, GetClassname());
			lastWarning = Interfaces::Globals->realtime;
		}
	}
#endif

	boneMask = boneMask | 0x80000; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.

	if (GetSequence() == -1)
		return false;

	if (boneMask == -1)
	{
		boneMask = GetPreviousBoneMask();
	}

	// We should get rid of this someday when we have solutions for the odd cases where a bone doesn't
	// get setup and its transform is asked for later.
#if 0
	if (Interfaces::Cvar->FindVar("cl_SetupAllBones")->GetInt())
	{
		boneMask |= BONE_USED_BY_ANYTHING;
	}
#endif

	// Set up all bones if recording, too
	if (IsToolRecording())
	{
		boneMask |= BONE_USED_BY_ANYTHING;
	}

	CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();

	if (*g_bInThreadedBoneSetup)
	{
		if (!pBoneSetupLock->TryLock())
		{
			// someone else is handling
			return false;
		}
		// else, we have the lock
	}
	else
	{
		pBoneSetupLock->Lock();
	}

	// If we're setting up LOD N, we have set up all lower LODs also
	// because lower LODs always use subsets of the bones of higher LODs.
	int nLOD = 0;
	int nMask = BONE_USED_BY_VERTEX_LOD0;
	for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
	{
		if (boneMask & nMask)
			break;
	}
	for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
	{
		boneMask |= nMask;
	}

	// A bit of a hack, but this way when in prediction we use the "correct" gpGlobals->curtime -- rather than the
	// one that the player artificially advances
	using InPredictionFn = bool(__thiscall*)(DWORD);

	if (GetPredictable() &&
		GetVFunc<InPredictionFn>((PVOID)Interfaces::Prediction, 14) ((DWORD)Interfaces::Prediction)) //Interfaces::Prediction->InPrediction())
	{
		currentTime = *(float*)((DWORD)Interfaces::Prediction + 0x5C);//Interfaces::Prediction->GetSavedTime();
	}

	CBoneAccessor* m_BoneAccessor = GetBoneAccessor();

	if (GetMostRecentModelBoneCounter() != *g_iModelBoneCounter)
	{
		// Clear out which bones we've touched this frame if this is 
		// the first time we've seen this object this frame.
		// BUGBUG: Time can go backward due to prediction, catch that here until a better solution is found
		float flLastBoneSetupTime = GetLastBoneSetupTime();
		//LastBoneChangedTime return FLT_MAX
		if (LastBoneChangedTime() >= flLastBoneSetupTime || currentTime < flLastBoneSetupTime)
		{
			m_BoneAccessor->SetReadableBones(0);
			m_BoneAccessor->SetWritableBones(0);
			SetLastBoneSetupTime(currentTime);
		}
		SetPreviousBoneMask(GetAccumulatedBoneMask());
		SetAccumulatedBoneMask(0);
	}

	//MarkForThreadedBoneSetup();

	// Keep track of everthing asked for over the entire frame
	// But not those things asked for during bone setup
	//	if ( !g_bInThreadedBoneSetup )
	{
		SetAccumulatedBoneMask(GetAccumulatedBoneMask() | boneMask);
	}

	// Make sure that we know that we've already calculated some bone stuff this time around.
	SetMostRecentModelBoneCounter(*g_iModelBoneCounter);


	// Have we cached off all bones meeting the flag set?
	if ((m_BoneAccessor->GetReadableBones() & boneMask) != boneMask)
	{
		MDLCACHE_CRITICAL_SECTION();


		DWORD hdr = (DWORD)GetModelPtr();
		if (!hdr)//|| !hdr->SequencesAvailable()) //TODO:
		{
			pBoneSetupLock->Unlock();
			return false;
		}

		CBaseEntity* renderable = (CBaseEntity*)((DWORD)this + 4);

		//matrix3x4a_t* backup_matrix = m_BoneAccessor->GetBoneArrayForWrite();
		//if (!backup_matrix)
		//	return false;

		// Setup our transform based on render angles and origin.
		__declspec (align(16)) matrix3x4_t parentTransform;
		AngleMatrix(GetAbsAngles(), *GetAbsOrigin(), parentTransform);

		// Load the boneMask with the total of what was asked for last frame.
		boneMask |= GetPreviousBoneMask();

		// Allow access to the bones we're setting up so we don't get asserts in here.
		int oldReadableBones = m_BoneAccessor->GetReadableBones();
		int oldWritableBones = m_BoneAccessor->GetWritableBones();
		m_BoneAccessor->SetWritableBones(oldReadableBones | boneMask);
		m_BoneAccessor->SetReadableBones(m_BoneAccessor->GetWritableBones()); 

		//if (hdr->flags() & STUDIOHDR_FLAGS_STATIC_PROP)
		if (*(char *)(*(DWORD *)hdr + 152) & 0x10)
		{
			MatrixCopy(parentTransform, m_BoneAccessor->GetBoneForWrite(0));
		}
		else
		{
			// This is necessary because it's possible that CalculateIKLocks will trigger our move children
			// to call GetAbsOrigin(), and they'll use our OLD bone transforms to get their attachments
			// since we're right in the middle of setting up our new transforms. 
			//
			// Setting this flag forces move children to keep their abs transform invalidated.
			*(int*)(renderable + 224) |= 8;//AddFlag( EFL_SETTING_UP_BONES ); (1 << 3));

			DWORD m_pIk = (DWORD)GetIK();

			// only allocate an ik block if the npc can use it
			//if (!m_pIk && hdr->numikchains() > 0 && !(m_EntClientFlags & ENTCLIENTFLAG_DONTUSEIK))
			if (!m_pIk && *(DWORD *)(*(DWORD *)hdr + 284) > 0 && !(GetEntClientFlags() & 2))
			{
				m_pIk = Wrap_CreateIK();
				SetIK(m_pIk);
			}

			Vector pos[MAXSTUDIOBONES];
			QuaternionAligned q[MAXSTUDIOBONES];
#ifdef _DEBUG
			// Having these uninitialized means that some bugs are very hard
			// to reproduce. A memset of 0xFF is a simple way of getting NaNs.
			memset(pos, 0xFF, sizeof(pos));
			memset(q, 0xFF, sizeof(q));
#endif

			//int bonesMaskNeedRecalc = boneMask | oldReadableBones; // Hack to always recalc bones, to fix the arm jitter in the new CS player anims until Ken makes the real fix

			if (m_pIk)
			{
				if (Teleported(this) || GetEffects() & EF_NOINTERP)
				{
					ClearTargets(m_pIk);
				}

				Wrap_IKInit(GetIK(), (CStudioHdr*)hdr, GetAbsAngles()/*backup_absangles*/, *GetAbsOrigin()/*backup_absorigin*/, currentTime, Interfaces::Globals->framecount, boneMask);
			}

			// Let pose debugger know that we are blending
			//g_pPoseDebugger->StartBlending(this, hdr);

			int v115;
			int tempBoneMask;

			bool bSkipAnimFrame = ShouldSkipAnimationFrame(this);

			if (bSkipAnimFrame)
			{
				DWORD v134 = *(DWORD*)hdr;
				memcpy(&pos[0], (const void*)((DWORD)this + 0xA60), 12 * *(DWORD*)(v134 + 0x9C));
				memcpy(&q[0], (const void*)((DWORD)this + 0x1660), 16 * *(DWORD*)(v134 + 0x9C));
				boneMask = m_BoneAccessor->GetWritableBones();
			}
			else
			{
				v115 = *(DWORD*)((DWORD)this + 2596);
				int tmpmask = boneMask;
				if (v115 != -1)
					tmpmask = boneMask & v115;

				tempBoneMask = tmpmask | 0x80000;

#ifdef _DEBUG
				if (UnknownSetupBonesCall())
					DebugBreak();
#endif

#if 0
				if (UnknownSetupBonesCall())
				{
					int v117 = 0;
					DWORD a9 = *(DWORD*)hdr;
					if (*(DWORD *)(*(DWORD *)hdr + 156) > 0)
					{
						DWORD *v118 = *(DWORD **)((DWORD)hdr + 48);
						DWORD v119 = g_nNumBonesSetupBlendingRulesOnlyTemp;
						do
						{
							if (tempBoneMask & *v118)
								g_nNumBonesSetupBlendingRulesOnlyTemp = ++v119;
							++v117;
							++v118;
						} while (v117 < *(DWORD *)(a9 + 156));
					}
				}
#endif

				StandardBlendingRules((CStudioHdr*)hdr, pos, q, currentTime, tempBoneMask);

				DWORD Unknown[2048];

#if 1
				if ((bool)(*(int(__thiscall **)(CBaseEntity*))(*(DWORD *)((DWORD)this) + 608))(this))
				{
					if (tempBoneMask != boneMask)
					{
						DWORD v108 = *(DWORD *)hdr;
						if (*(DWORD *)(*(DWORD *)hdr + 156) > 0)
						{
							DWORD v68 = *(DWORD*)(hdr + 48);
							DWORD qptr = (DWORD)&q[0];
							DWORD v69 = (DWORD)((DWORD)renderable + 2660);
							DWORD v97 = *(DWORD *)(hdr + 48);
							DWORD v70 = 0;
							DWORD v71 = (DWORD)((DWORD)renderable + 5724);
							DWORD v72 = (DWORD)&Unknown[255];

							do
							{
								if (!(*(DWORD*)v68 & 0x80300))
								{
									*(DWORD*)(v72 - 8) = *(DWORD*)(v69 - 8);
									*(DWORD*)(v72 - 4) = *(DWORD*)(v69 - 4);
									*(DWORD *)v72 = *(DWORD*)v69;
									for (int i = 0; i < 16; i += 4)
									{
										*(DWORD *)(qptr + i) = *(DWORD*)(v71 + i);
									}
									v68 = v97;
								}
								qptr += 16;
								v97 = v68 + 4;
								++v70;
								v69 += 12;
								v72 += 12;
								v71 += 16;
								v68 += 4;
							} while (v70 < *(DWORD *)(v108 + 156));
						}
					}
				}
#endif

#if 0
				//this shit seems to draw skeletons or something
				if (UnknownSetupBonesCall())
				{
					DWORD tmp = 0;
					DWORD v126 = 0;
					do
					{
						DWORD v127 = tmp;
						float tmpflt;
						if (v158 & *(DWORD *)(*(DWORD *)(hdr + 48) + 4 * v126))
						{
							v128 = *(DWORD *)(*(DWORD *)(hdr + 68) + 4 * v126);
							if (v128 >= 0)
							{
								v129 = *(DWORD *)(renderable + 9876);
								v130 = 6 * v128;
								a18 = *(DWORD *)(v129 + tmp + 12);
								tmpflt = *(float *)(v129 + tmp + 28);
								a20 = *(DWORD *)(v129 + tmp + 44);
								v161 = *(DWORD *)(v129 + 8 * v130 + 12);
								a16 = *(float *)(v129 + 8 * v130 + 28);
								v131 = *(DWORD *)(v129 + 8 * v130 + 44);
								v132 = *(DWORD *)(renderable - 4);
								a17 = v131;
								v133 = (unsigned __int8)(*(int(__stdcall **)(int))(v132 + 608))(vars0) == 0;
								vars0 = 0;
								v134 = *(DWORD *)dword_219DAED8;
								if (v133)
								{
									a27 = a16 + 2.0;
									a28 = a17;
									a29 = a18;
									a30 = tmpflt + 2.0;
									a31 = a20;
									*(float *)&m_pIK = a21;
									(*(void(__stdcall **)(float *, float *, signed int, signed int, signed int, signed int))(v134 + 20))(
										&a30,
										&a27,
										200,
										0,
										255,
										1);
								}
								else
								{
									(*(void(__stdcall **)(float *, float *, signed int, signed int, signed int, signed int))(v134 + 20))(
										&tmpflt,
										&a16,
										0,
										255,
										255,
										1);
								}
								v127 = tmp;
								v126 = boneMask;
							}
						}
						boneMask = ++v126;
						tmp = v127 + 48;
					} while (v126 < *(DWORD *)(*(DWORD *)hdr + 156));
				}
#endif
				SetLastSetupBonesFrameCount(Interfaces::Globals->framecount);
			}

			byte computed[256] = { 0 };

			// don't calculate IK on ragdolls
			if (GetIK() && !IsRagdoll())
			{
				UpdateIKLocks(currentTime);
				Wrap_UpdateTargets(m_pIk, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
				CalculateIKLocks(currentTime);
				Wrap_SolveDependencies(m_pIk, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
				if (GetUnknownSetupBonesInt() && (boneMask & 0x400))
				{
					DoUnknownSetupBonesCall((CStudioHdr*)hdr, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), &computed[0], (DWORD*)GetIK());
				}
			}

			BuildTransformations((CStudioHdr*)hdr, pos, q, parentTransform, boneMask, &computed[0]);

#if 0
			if (UnknownSetupBonesCall())// UnknownSetupBonesCall
			{
				v159 = *(_DWORD *)LODWORD(hdr3);
				if (*(_DWORD *)(*(_DWORD *)LODWORD(hdr3) + 156) > 0)
				{
					v141 = *(_DWORD **)(LODWORD(hdr3) + 48);
					v142 = 0;
					v143 = dword_1F91977C;
					do
					{
						if (boneMaskArgumenta & *v141)
							dword_1F91977C = ++v143;
						++v142;
						++v141;
					} while (v142 < *(_DWORD *)(v159 + 156));
					EntPlus4 = v158;
					hdr3 = a9;
				}
			}
#endif

#if 0
			// Draw skeleton?
			//if (enable_skeleton_draw.GetBool())
			if (((int(__thiscall *)(int(__stdcall ***)(char)))off_1D4F5B00[13])(&off_1D4F5B00))
				sub_1CBFC000(LODWORD(hdr3), boneMaskArgumenta);
#endif

			*(int*)(renderable + 224) &= ~(8);

			ControlMouth((CStudioHdr*)hdr);

			if (!bSkipAnimFrame)
			{
				memcpy((void *)(renderable + 2652), &pos[0], 12 * *(DWORD *)(*(DWORD *)hdr + 156));
				memcpy((void *)(renderable + 5724), &q[0], 16 * *(DWORD *)(*(DWORD *)hdr + 156));
			}
		}

		if (!(oldReadableBones & 0x200) && boneMask & 0x200)
			Wrap_AttachmentHelper((CStudioHdr*)hdr);

	}

	// Do they want to get at the bone transforms? If it's just making sure an aiment has 
	// its bones setup, it doesn't need the transforms yet.
	if (pBoneToWorldOut)
	{
		CUtlVectorSimple* CachedBoneData = GetCachedBoneData();
		if ((unsigned int)nMaxBones >= CachedBoneData->count)
		{
			memcpy((void*)pBoneToWorldOut, CachedBoneData->Base(), sizeof(matrix3x4_t) * CachedBoneData->Count());
		}
		else
		{
#ifdef _DEBUG
			AllocateConsole();
			printf("SetupBones: invalid bone array size(%d - needs %d)\n", nMaxBones, CachedBoneData->Count());
#endif
			pBoneSetupLock->Unlock();
			return false;
		}
	}

	pBoneSetupLock->Unlock();
	return true;
}

bool CBaseEntity::SetupBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	auto prof = START_PROFILING("SetupBones");
	//*(int*)((DWORD)this + AdrOf_m_nWritableBones) = 0;
	//WriteInt((uintptr_t)this + (AdrOf_m_nWritableBones - 4), 0); //Readable bones
	//*(int*)((DWORD)this + AdrOf_m_iDidCheckForOcclusion) = reinterpret_cast<int*>(AdrOf_m_dwOcclusionArray)[1];

	bool ret = GetVFunc<SetupBonesFn>((void*)((uintptr_t)this + 0x4), 13)((void*)((uintptr_t)this + 0x4), pBoneToWorldOut, nMaxBones, boneMask, currentTime);
	END_PROFILING(prof);
	return ret;
#if 0
	__asm
	{
		mov edi, this
		lea ecx, dword ptr ds : [edi + 0x4]
		mov edx, dword ptr ds : [ecx]
		push currentTime
		push boneMask
		push nMaxBones
		push pBoneToWorldOut
		call dword ptr ds : [edx + 0x34]
	}
#endif
}

Vector CBaseEntity::GetVelocity()
{
	Vector tempvector;
	ReadVector((DWORD)this + m_vecVelocity, tempvector);
	return tempvector;
}

void CBaseEntity::SetVelocity(Vector velocity)
{
	WriteVector((DWORD)this + m_vecVelocity, velocity);
}

void CBaseEntity::SetAbsVelocity(Vector velocity)
{
	((void(__thiscall*)(CBaseEntity*, Vector*))AdrOf_SetAbsVelocity)(this, &velocity);
}

Vector CBaseEntity::GetBaseVelocity()
{
	Vector tmp;
	ReadVector((DWORD)this + m_vecBaseVelocity, tmp);
	return tmp;
}

void CBaseEntity::SetBaseVelocity(Vector velocity)
{
	WriteVector((DWORD)this + m_vecBaseVelocity, velocity);
}

Vector CBaseEntity::GetPredicted(Vector p0)
{
	return vecZero;
	//return M::ExtrapolateTick(p0, this->GetVelocity()); //dylan fix
}

ICollideable* CBaseEntity::GetCollideable()
{
	return (ICollideable*)((DWORD)this + m_Collision);
}

void CBaseEntity::GetPlayerInfo(player_info_t *dest)
{
	Interfaces::Engine->GetPlayerInfo(index, dest);
}

std::string	CBaseEntity::GetName()
{
	player_info_t info;
	GetPlayerInfo(&info);
	return std::string(info.name);
}

void CBaseEntity::GetSteamID(char* dest)
{
	player_info_t info;
	GetPlayerInfo(&info);
	memcpy(dest, &info.guid, 33);
}

std::string CBaseEntity::GetLastPlace()
{
	return std::string((const char*)this + m_szLastPlaceName);
}

CBaseCombatWeapon* CBaseEntity::GetWeapon()
{
	//DWORD weaponData = *(DWORD*)((DWORD)this + m_hActiveWeapon);
	//return (CBaseCombatWeapon*)Interfaces::ClientEntList->GetClientEntityFromHandle(weaponData);
	DWORD weaponData = ReadInt((DWORD)this + m_hActiveWeapon);
	return (CBaseCombatWeapon*)Interfaces::ClientEntList->GetClientEntityFromHandle(weaponData);
	//dylan fix this^
	//int weaponIndex = (ReadInt((DWORD)this + m_hActiveWeapon) & 0xFFF) - 1;

	//return (CBaseCombatWeapon*)ReadInt(((DWORD)ClientHandle + m_dwEntityList + weaponIndex * 0x10) - 0x10);
}

ClientClass* CBaseEntity::GetClientClass()
{
	PVOID pNetworkable = (PVOID)((DWORD)(this) + 0x8);
	typedef ClientClass*(__thiscall* OriginalFn)(PVOID);
	return GetVFunc<OriginalFn>(pNetworkable, 2)(pNetworkable);
}

int CBaseEntity::GetCollisionGroup()
{
	return ReadInt((DWORD)this + m_CollisionGroup);//*(int*)((DWORD)this + m_CollisionGroup);
}

CBaseAnimating* CBaseEntity::GetBaseAnimating(void)
{
	int adr = ReadInt((uintptr_t)this);
	if (adr)
	{
		return ((CBaseAnimating*(__thiscall*)(CBaseEntity* me))ReadInt(adr + m_dwGetBaseAnimating))(this);
	}
	return NULL;
}

void CBaseEntity::InvalidateBoneCache()
{
	bool orig = *s_bEnableInvalidateBoneCache;
	*s_bEnableInvalidateBoneCache = true;
	((CBaseAnimating*(__fastcall*)(CBaseEntity*))AdrOfInvalidateBoneCache)(this);
	*s_bEnableInvalidateBoneCache = orig;
}

HANDLE CBaseEntity::GetObserverTarget()
{
	return (HANDLE)ReadInt((uintptr_t)this + m_hObserverTarget);
}

BOOLEAN CBaseEntity::IsPlayer()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, (m_dwIsPlayer / 4))(this);
	//return ((BOOLEAN(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwIsPlayer))(this);
}

CUserCmd* CBaseEntity::GetLastUserCommand()
{
	return (CUserCmd*)ReadInt(ReadInt((uintptr_t)this) + m_dwGetLastUserCommand);
}

BOOLEAN CBaseEntity::IsConnected()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, (m_dwIsConnected / 4))(this);
	//return ((BOOLEAN(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwIsConnected))(this);
}

BOOLEAN CBaseEntity::IsSpawned()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, (m_dwIsSpawned / 4))(this);
	//return ((BOOLEAN(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwIsSpawned))(this);
}

BOOLEAN CBaseEntity::IsActive()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, (m_dwIsActive / 4))(this);
	//return ((BOOLEAN(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwIsActive))(this);
}

BOOLEAN CBaseEntity::IsFakeClient()
{
	typedef BOOLEAN(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, (m_dwIsFakeClient / 4))(this);
	//return ((BOOLEAN(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwIsFakeClient))(this);
}

bool CBaseEntity::IsTargetingLocalPunch()
{
	Vector src, dst, forward;
	trace_t tr;
	Ray_t ray;
	CTraceFilterIgnoreWorld filter;

	int index = ReadInt((uintptr_t)&this->index);
	CustomPlayer *pCAPlayer = &AllPlayers[index];
	if (!pCAPlayer->Dormant)
	{
		QAngle viewangle = pCAPlayer->PersistentData.m_PlayerRecords.size() > 0 ? pCAPlayer->PersistentData.m_PlayerRecords.front()->eyeangles : pCAPlayer->BaseEntity->GetEyeAngles();
		viewangle -= this->GetPunch() * 2.f;

		AngleVectors(viewangle, &forward);
		VectorNormalizeFast(forward);
		forward *= 8192.f;
		src = this->GetEyePosition();
		dst = src + forward;
		filter.pSkip = (IHandleEntity*)this;
		filter.m_icollisionGroup = COLLISION_GROUP_NONE;
		ray.Init(src, dst);

		Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, (ITraceFilter*)&filter, &tr);

		return tr.m_pEnt == LocalPlayer.Entity;
	}
	return false;
}

QAngle CBaseEntity::GetAngleRotation()
{
	QAngle temp;
	ReadAngle((DWORD)this + m_angRotation, temp);
	return temp;
}

void CBaseEntity::SetAngleRotation(QAngle rot)
{
	//((void(__thiscall*)(CBaseEntity*, QAngle*))AdrOf_SetAbsAngles)(this, &rot);
	WriteAngle((DWORD)this + m_angRotation, rot);
}

QAngle CBaseEntity::GetLocalAngles()
{
	return *(QAngle*)((DWORD)this + m_vecLocalAngles);
}

void CBaseEntity::SetLocalAngles(QAngle rot)
{
	*(QAngle*)((DWORD)this + m_vecLocalAngles) = rot;
}

QAngle CBaseEntity::GetOldAngRotation()
{
	return *(QAngle*)((DWORD)this + m_vecOldAngRotation);
}

void CBaseEntity::SetOldAngRotation(QAngle rot)
{
	*(QAngle*)((DWORD)this + m_vecOldAngRotation) = rot;
}

void CBaseEntity::SetAbsAngles(const QAngle &rot)
{
	((void(__thiscall*)(CBaseEntity*, const QAngle&))AdrOf_SetAbsAngles)(this, rot);
}

Vector CBaseEntity::GetMins()
{
	Vector temp;
	ReadVector((DWORD)this + m_vecMins, temp);
	return temp;
}

void CBaseEntity::SetMins(Vector mins)
{
	WriteVector((DWORD)this + m_vecMins, mins);
}

Vector CBaseEntity::GetMaxs()
{
	Vector temp;
	ReadVector((DWORD)this + m_vecMaxs, temp);
	return temp;
}

void CBaseEntity::SetMaxs(Vector maxs)
{
	WriteVector((DWORD)this + m_vecMaxs, maxs);
}

int CBaseEntity::GetSequence()
{
	return ReadInt((DWORD)this + m_nSequence);
}

void CBaseEntity::SetSequence(int seq)
{
	WriteInt((DWORD)this + m_nSequence, seq);
}

void CBaseEntity::GetPoseParameterRange(int index, float& flMin, float& flMax)
{
	GetPoseParameterRangeGame(this, index, flMin, flMax);
}

float CBaseEntity::GetPoseParameter(int index)
{
	return ReadFloat((DWORD)this + m_flPoseParameter + (sizeof(float) * index));
}

float CBaseEntity::GetPoseParameterUnscaled(int index)
{
	float flMin, flMax;
	GetPoseParameterRangeGame(this, index, flMin, flMax);
	return ReadFloat((DWORD)this + m_flPoseParameter + (sizeof(float) * index)) * (flMax - flMin) + flMin;
}

float CBaseEntity::GetPoseParameterUnScaledServer(int index)
{
#ifdef HOOK_LAG_COMPENSATION
	static DWORD getposeadr = NULL;
	if (!getposeadr)
	{
		const char *sig = "55  8B  EC  56  57  8B  F9  83  BF  9C  04  00  00  00  75  ??  A1  ??  ??  ??  ??  8B  30  8B  07  FF  50  18  8B  ??  ??  ??  ??  ??  50  FF  56  04  85  C0  74  ??  8B  CF  E8  7F  28  00  00  8B  B7  9C  04  00  00";
		getposeadr = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)sig, strlen(sig));
		if (!getposeadr)
			DebugBreak();
	}

	((void(__thiscall *)(CBaseEntity*, int))getposeadr)(this, index); //call as void to not fuck up xmm0 registers
	float res;
	__asm movss res, xmm0
	return res;
#endif
	return 0.0f;
}

int CBaseEntity::LookupPoseParameter(char *name)
{
	return LookupPoseParameterGame(this, name);
}

void CBaseEntity::SetPoseParameter(int index, float p)
{
	float flMin, flMax;
	GetPoseParameterRangeGame(this, index, flMin, flMax);
	float scaledValue = (p - flMin) / (flMax - flMin);
#if 0
	DWORD dc = ReadInt(AdrOf_DataCacheSetPoseParmaeter);
	DWORD me = (DWORD)this;
	__asm {
		mov esi, dc
		mov ecx, esi
		mov eax, [esi]
		call [eax + 0x80]
		push index
		movss xmm2, scaledValue
		mov ecx, me
		call SetPoseParameterGame
		mov eax, [esi]
		mov ecx, esi
		call [eax + 0x84]
	}
#endif
	WriteFloat((DWORD)this + m_flPoseParameter + (sizeof(float) * index), scaledValue);
}

void CBaseEntity::SetPoseParameterScaled(int index, float p)
{
	WriteFloat((DWORD)this + m_flPoseParameter + (sizeof(float) * index), p);
}

void CBaseEntity::CopyPoseParameters(float* dest)
{
	float* flPose = (float*)((DWORD)this + m_flPoseParameter);
	memcpy(dest, flPose, sizeof(float) * 24);
}

unsigned char CBaseEntity::GetClientSideAnimation()
{
	return ReadByte((DWORD)this + m_bClientSideAnimation);
}

void CBaseEntity::SetClientSideAnimation(unsigned char a)
{
	WriteByte((DWORD)this + m_bClientSideAnimation, a);
}

float CBaseEntity::GetCycle()
{
	return ReadFloat((DWORD)this + m_flCycle);
}

void CBaseEntity::SetCycle(float cycle)
{
	WriteFloat((DWORD)this + m_flCycle, cycle);
}

float CBaseEntity::GetNextAttack()
{
	return ReadFloat((DWORD)this + m_flNextAttack);
}

void CBaseEntity::SetNextAttack(float att)
{
	WriteFloat((DWORD)this + m_flNextAttack, att);
}

BOOLEAN CBaseEntity::IsDucked()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bDucked);
}

void CBaseEntity::SetDucked(bool ducked)
{
	WriteByte((DWORD)this + m_bDucked, ducked);
}

BOOLEAN CBaseEntity::IsDucking()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bDucking);
}

void CBaseEntity::SetDucking(bool ducking)
{
	WriteByte((DWORD)this + m_bDucking, ducking);
}

BOOLEAN CBaseEntity::IsInDuckJump()
{
	return (BOOLEAN)ReadByte((DWORD)this + m_bInDuckJump);
}

void CBaseEntity::SetInDuckJump(bool induckjump)
{
	WriteByte((DWORD)this + m_bInDuckJump, induckjump);
}

int CBaseEntity::GetDuckTimeMsecs()
{
	return ReadInt((DWORD)this + m_nDuckTimeMsecs);
}

void CBaseEntity::SetDuckTimeMsecs(int msecs)
{
	WriteInt((DWORD)this + m_nDuckTimeMsecs, msecs);
}

int CBaseEntity::GetJumpTimeMsecs()
{
	return ReadInt((DWORD)this + m_nJumpTimeMsecs);
}

void CBaseEntity::SetJumpTimeMsecs(int msecs)
{
	WriteInt((DWORD)this + m_nJumpTimeMsecs, msecs);
}

float CBaseEntity::GetFallVelocity()
{
	return ReadFloat((DWORD)this + m_flFallVelocity);
}

void CBaseEntity::SetFallVelocity(float vel)
{
	WriteFloat((DWORD)this + m_flFallVelocity, vel);
}

Vector CBaseEntity::GetViewOffset()
{
	Vector tmp;
	ReadVector((DWORD)this + m_vecViewOffset, tmp);
	return tmp;
}

void CBaseEntity::SetViewOffset(Vector off)
{
	WriteVector((DWORD)this + m_vecViewOffset, off);
}

int CBaseEntity::GetNextThinkTick()
{
	return ReadInt((DWORD)this + m_nNextThinkTick);
}

void CBaseEntity::SetNextThinkTick(int tick)
{
	WriteInt((DWORD)this + m_nNextThinkTick, tick);
}

float CBaseEntity::GetDuckAmount()
{
	return ReadFloat((DWORD)this + m_flDuckAmount);
}

void CBaseEntity::SetDuckAmount(float duckamt)
{
	WriteFloat((DWORD)this + m_flDuckAmount, duckamt);
}

float CBaseEntity::GetDuckSpeed()
{
	return ReadFloat((DWORD)this + m_flDuckSpeed);
}

void CBaseEntity::SetDuckSpeed(float spd)
{
	WriteFloat((DWORD)this + m_flDuckSpeed, spd);
}

float CBaseEntity::GetVelocityModifier()
{
	return ReadFloat((DWORD)this + m_flVelocityModifier);
}

void CBaseEntity::SetVelocityModifier(float vel)
{
	WriteFloat((DWORD)this + m_flVelocityModifier, vel);
}

float CBaseEntity::GetPlaybackRate()
{
	return ReadFloat((DWORD)this + m_flPlaybackRate);
}

void CBaseEntity::SetPlaybackRate(float rate)
{
	WriteFloat((DWORD)this + m_flPlaybackRate, rate);
}

float CBaseEntity::GetAnimTime()
{
	return ReadFloat((DWORD)this + m_flAnimTime);
}

void CBaseEntity::SetAnimTime(float time)
{
	WriteFloat((DWORD)this + m_flAnimTime, time);
}

float CBaseEntity::GetSimulationTime()
{
	//this is sometimes being 0 causing crash
	return ReadFloat((DWORD)this + m_flSimulationTime);
}

float CBaseEntity::GetSimulationTimeServer()
{
	return *(float*)((DWORD)this + m_flSimulationTimeServer);
}

float CBaseEntity::GetOldSimulationTime()
{
	return *(float*)((DWORD)this + (m_flSimulationTime + 4));
}

void CBaseEntity::SetSimulationTime(float time)
{
	WriteFloat((DWORD)this + m_flSimulationTime, time);
}

float CBaseEntity::GetLaggedMovement()
{
	return ReadFloat((DWORD)this + m_flLaggedMovementValue);
}

void CBaseEntity::SetLaggedMovement(float mov)
{
	WriteFloat((DWORD)this + m_flLaggedMovementValue, mov);
}

CBaseEntity* CBaseEntity::GetGroundEntity()
{
	return (CBaseEntity*)ReadInt((DWORD)this + m_hGroundEntity);
}

void CBaseEntity::SetGroundEntity(CBaseEntity* groundent)
{
	WriteInt((DWORD)this + m_hGroundEntity, (int)groundent);
}

Vector CBaseEntity::GetVecLadderNormal()
{
	Vector tmp;
	ReadVector((DWORD)this + m_vecLadderNormal, tmp);
	return tmp;
}

void CBaseEntity::SetVecLadderNormal(Vector norm)
{
	WriteVector((DWORD)this + m_vecLadderNormal, norm);
}

float CBaseEntity::GetLowerBodyYaw()
{
	return ReadFloat((DWORD)this + m_flLowerBodyYawTarget);
}

void CBaseEntity::SetLowerBodyYaw(float yaw)
{
	WriteFloat((DWORD)this + m_flLowerBodyYawTarget, yaw);
}

C_CSGOPlayerAnimState* CBaseEntity::GetPlayerAnimState()
{
	return (C_CSGOPlayerAnimState*)ReadInt((uintptr_t)this + m_hPlayerAnimState);
}

void* CBaseEntity::GetPlayerAnimStateServer()
{
	return (void*)ReadInt((uintptr_t)(this) + m_hPlayerAnimStateServer);
}

float CBaseEntity::GetNextLowerBodyyawUpdateTimeServer()
{
	void* animstate = GetPlayerAnimStateServer();
	if (animstate)
	{
		return *(float*)((DWORD)animstate + m_flNextLowerBodyYawUpdateTimeServer);
	}
	return 0.0f;
}

#include "ClassIDS.h"
bool CBaseEntity::IsWeapon()
{
	//if (IsPlayer())
		//return false;

#if 1
	typedef bool(__thiscall* OriginalFn)(void*);
	return GetVFunc<OriginalFn>(this, 160)(this);
#else
	ClientClass *clclass = GetClientClass();
	int classid = ReadInt((uintptr_t)&clclass->m_ClassID);
#if 0
	char* networkname = clclass->m_pNetworkName;
	//CWeapon
	if ((networkname[0] == 'C' && networkname[1] == 'W' && networkname[2] == 'e') || (classid == _CAK47 || classid == _CDEagle))
		return true;
	return false;
#else
	switch (classid)
	{
	case _CWeaponXM1014:
	case _CWeaponTaser:
	case _CSmokeGrenade:
	case _CWeaponSG552:
	case _CSensorGrenade:
	case _CWeaponSawedoff:
	case _CWeaponNOVA:
	case _CIncendiaryGrenade:
	case _CMolotovGrenade:
	case _CWeaponM3:
	case _CKnifeGG:
	case _CKnife:
	case _CHEGrenade:
	case _CFlashbang:
	case _CWeaponElite:
	case _CDecoyGrenade:
	case _CDEagle:
	case _CWeaponUSP:
	case _CWeaponM249:
	case _CWeaponUMP45:
	case _CWeaponTMP:
	case _CWeaponTec9:
	case _CWeaponSSG08:
	case _CWeaponSG556:
	case _CWeaponSG550:
	case _CWeaponScout:
	case _CWeaponSCAR20:
	case _CSCAR17:
	case _CWeaponP90:
	case _CWeaponP250:
	case _CWeaponP228:
	case _CWeaponNegev:
	case _CWeaponMP9:
	case _CWeaponMP7:
	case _CWeaponMP5Navy:
	case _CWeaponMag7:
	case _CWeaponMAC10:
	case _CWeaponM4A1:
	case _CWeaponHKP2000:
	case _CWeaponGlock:
	case _CWeaponGalilAR:
	case _CWeaponGalil:
	case _CWeaponG3SG1:
	case _CWeaponFiveSeven:
	case _CWeaponFamas:
	case _CWeaponBizon:
	case _CWeaponAWP:
	case _CWeaponAug:
	case _CAK47:
	case _CSmokeGrenadeProjectile:
	case _CSensorGrenadeProjectile:
	case _CMolotovProjectile:
	case _CDecoyProjectile:
		return true;
	}
	return false;
#endif
#endif
}

bool CBaseEntity::IsProjectile()
{
	//if (IsPlayer())
	//return false;

	ClientClass *clclass = GetClientClass();
	int classid = ReadInt((uintptr_t)&clclass->m_ClassID);

	switch (classid)
	{
		case _CSmokeGrenadeProjectile:
		case _CSensorGrenadeProjectile:
		case _CMolotovProjectile:
		case _CDecoyProjectile:
		case _CBaseCSGrenadeProjectile:
		return true;
	}
	return false;
}

bool CBaseEntity::IsFlashGrenade()
{
	ClientClass *clclass = GetClientClass();
	int classid = ReadInt((uintptr_t)&clclass->m_ClassID);

	return classid == _CBaseCSGrenadeProjectile;
}

bool CBaseEntity::IsChicken()
{
	ClientClass *clclass = GetClientClass();
	int classid = ReadInt((uintptr_t)&clclass->m_ClassID);

	return classid == _CChicken;
}

void CBaseEntity::DrawHitboxes(ColorRGBA color, float livetimesecs)
{
	model_t *pmodel = GetModel();
	if (!pmodel)
		return;

	studiohdr_t* pStudioHdr = Interfaces::ModelInfoClient->GetStudioModel(pmodel);

	if (!pStudioHdr)
		return;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(GetHitboxSet());
	if (!set)
		return;

	matrix3x4_t boneMatrixes[MAXSTUDIOBONES];
	SetupBones(boneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, /*GetSimulationTime()*/ Interfaces::Globals->curtime);

	Vector hitBoxVectors[8];

	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		if (pbox)
		{
			if (pbox->radius == -1.0f)
			{
				//Slow DirectX method, but works
#if 0
				Vector points[8] = { Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmax.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmax.z) };

				for (int index = 0; index < 8; ++index)
				{
					// scale down the hitbox size a tiny bit (default is a little too big)
#if 0
					points[index].x *= 0.9f;
					points[index].y *= 0.9f;
					points[index].z *= 0.9f;
#endif

					// transform the vector
					VectorTransformZ(points[index], boneMatrixes[pbox->bone], hitBoxVectors[index]);
				}

				DrawHitbox(color, hitBoxVectors);
#else
				Vector position;
				QAngle angles;
				//matrix3x4a_t* scaled = static_cast<matrix3x4a_t*>(boneMatrixes);
				MatrixAngles(boneMatrixes[pbox->bone], angles, position);
				Interfaces::DebugOverlay->AddBoxOverlay(position, pbox->bbmin, pbox->bbmax, angles, color.r, color.g, color.b, 0, livetimesecs);

#endif
			}
			else
			{
				Vector vMin, vMax;
				VectorTransformZ(pbox->bbmin, boneMatrixes[pbox->bone], vMin);
				VectorTransformZ(pbox->bbmax, boneMatrixes[pbox->bone], vMax);

				Interfaces::DebugOverlay->DrawPill(vMin, vMax, pbox->radius, color.r, color.g, color.b, color.a, livetimesecs);
			}
		}
	}
}

void CBaseEntity::DrawHitboxesFromCache(ColorRGBA color, float livetimesecs, matrix3x4_t *matrix)
{
	model_t *pmodel = GetModel();
	if (!pmodel)
		return;

	studiohdr_t* pStudioHdr = Interfaces::ModelInfoClient->GetStudioModel(pmodel);

	if (!pStudioHdr)
		return;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(GetHitboxSet());
	if (!set)
		return;

	Vector hitBoxVectors[8];

	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		if (pbox)
		{
			if (pbox->radius == -1.0f)
			{
				//Slow DirectX method, but works
#if 0
				Vector points[8] = { Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmax.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmax.z) };

				for (int index = 0; index < 8; ++index)
				{
					// scale down the hitbox size a tiny bit (default is a little too big)
#if 0
					points[index].x *= 0.9f;
					points[index].y *= 0.9f;
					points[index].z *= 0.9f;
#endif

					// transform the vector
					VectorTransformZ(points[index], boneMatrixes[pbox->bone], hitBoxVectors[index]);
				}

				DrawHitbox(color, hitBoxVectors);
#else
				Vector position;
				QAngle angles;
				//matrix3x4a_t* scaled = static_cast<matrix3x4a_t*>(boneMatrixes);
				MatrixAngles(matrix[pbox->bone], angles, position);
				Interfaces::DebugOverlay->AddBoxOverlay(position, pbox->bbmin, pbox->bbmax, angles, color.r, color.g, color.b, 0, livetimesecs);

#endif
			}
			else
			{
				Vector vMin, vMax;
				VectorTransformZ(pbox->bbmin, matrix[pbox->bone], vMin);
				VectorTransformZ(pbox->bbmax, matrix[pbox->bone], vMax);

				Interfaces::DebugOverlay->DrawPill(vMin, vMax, pbox->radius, color.r, color.g, color.b, color.a, livetimesecs);
			}
		}
	}
}

#include "INetchannelInfo.h"
INetChannelInfo* CBaseEntity::GetNetChannel()
{
	return nullptr;
	//todo: fix me
	//return ((INetChannelInfo*(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_dwGetNetChannel))(this);
}

void CBaseEntity::DisableInterpolation()
{
	CustomPlayer *pCPlayer = &AllPlayers[index];

	VarMapping_t *map = (VarMapping_t*)((DWORD)this + 0x24); // tf2 = 20
	if (pCPlayer->iOriginalInterpolationEntries == 0)
	{
		pCPlayer->iOriginalInterpolationEntries = map->m_nInterpolatedEntries;
	}
	map->m_nInterpolatedEntries = 0;

	/*for (int i = 0; i < map->m_nInterpolatedEntries; ++i)
	{
	VarMapEntry_t *e = &map->m_Entries[i];

	e->m_bNeedsToInterpolate = false;
	}*/

	//crash city, fuck this shit. phage's just crashes in client.dll
#if 0
	auto var_map_list_count = *(int*)((DWORD)map + 20);

	for (auto index = 0; index < var_map_list_count; index++)
	{
		*(DWORD*)((DWORD)map + (index * 12)) = 0;
	}
#endif
}

void CBaseEntity::EnableInterpolation() 
{
	CustomPlayer *pCPlayer = &AllPlayers[index];
	VarMapping_t *map = (VarMapping_t*)((DWORD)this + 0x24); // tf2 = 20
	if (map->m_nInterpolatedEntries == 0 && pCPlayer->iOriginalInterpolationEntries > 0 && !(GetEffects() & EF_NOINTERP))
	{
		map->m_nInterpolatedEntries = pCPlayer->iOriginalInterpolationEntries;
	}
#if 0
	auto var_map_list_count = *(int*)((DWORD)map + 20);

	for (auto index = 0; index < var_map_list_count; index++)
	{
		*(DWORD*)((DWORD)map + (index * 12)) = 1;
	}
#endif
}

void CBaseEntity::UpdateClientSideAnimation()
{
	GetVFunc<UpdateClientSideAnimationFn>(this, OffsetOf_UpdateClientSideAnimation)(this);
}

float CBaseEntity::GetLastClientSideAnimationUpdateTime()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_flLastClientSideAnimationUpdateTime;
	}
	
	return 0.0f;
}

void CBaseEntity::SetLastClientSideAnimationUpdateTime(float time)
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_flLastClientSideAnimationUpdateTime = time;
	}
}

int CBaseEntity::GetLastClientSideAnimationUpdateGlobalsFrameCount()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_iLastClientSideAnimationUpdateFramecount;
	}

	return 0;
}

void CBaseEntity::SetLastClientSideAnimationUpdateGlobalsFrameCount(int framecount)
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_iLastClientSideAnimationUpdateFramecount = framecount;
	}
}

float CBaseEntity::FrameAdvance(float flInterval)
{
#if 0
	CustomPlayer *pCPlayer = &AllPlayers[ReadInt((uintptr_t)&this->index)];

	float curtime = ReadFloat((uintptr_t)&Interfaces::Globals->curtime);

	if (flInterval == 0.0f)
	{
		flInterval = (curtime - pCPlayer->flAnimTime);
		if (flInterval <= 0.001f)
		{
			return 0.0f;
		}
	}

	if (!pCPlayer->flAnimTime)
	{
		flInterval = 0.0f;
	}

	pCPlayer->flAnimTime = curtime;

#endif
	return flInterval;
}

int CBaseEntity::GetEffects()
{
	return ReadInt((uintptr_t)this + m_fEffects);
}

void CBaseEntity::SetEffects(int effects)
{
	WriteInt((uintptr_t)this + m_fEffects, effects);
}

int CBaseEntity::GetObserverMode()
{
	return ReadInt((uintptr_t)this + m_iObserverMode);
}

void CBaseEntity::SetObserverMode(int mode)
{
	WriteInt((uintptr_t)this + m_iObserverMode, mode);
}

CUtlVectorSimple* CBaseEntity::GetAnimOverlayStruct() const
{
	return (CUtlVectorSimple*) ((DWORD)this + dwm_nAnimOverlay);
}

C_AnimationLayer* CBaseEntity::GetAnimOverlay(int i)
{
	if (i >= 0 && i < MAX_OVERLAYS)
	{
		CUtlVectorSimple *m_AnimOverlay = GetAnimOverlayStruct();
		return (C_AnimationLayer*)m_AnimOverlay->Retrieve(i, sizeof(C_AnimationLayer));
		//DWORD v19 = 7 * i;
		//DWORD m = *(DWORD*)((DWORD)m_AnimOverlay);
		//C_AnimationLayer* test = (C_AnimationLayer*)(m + 8 * v19);
		//DWORD sequence = *(DWORD *)(m + 8 * v19 + 24);

	}
	return nullptr;
}

int CBaseEntity::GetNumAnimOverlays() const
{
	CUtlVectorSimple *m_AnimOverlay = GetAnimOverlayStruct();
	return m_AnimOverlay ? m_AnimOverlay->count : 0;
}

void CBaseEntity::CopyAnimLayers(C_AnimationLayer* dest)
{
	int count = GetNumAnimOverlays();
	for (int i = 0; i < count; i++)
	{
		dest[i] = *GetAnimOverlay(i);
	}
}

// Invalidates the abs state of all children
void CBaseEntity::InvalidatePhysicsRecursive(int nChangeFlags)
{
	return ((void(__thiscall*)(CBaseEntity*, int))AdrOf_InvalidatePhysicsRecursive)(this, nChangeFlags);
}

Vector* CBaseEntity::GetAbsOrigin()
{
	return GetVFunc<Vector*(__thiscall*)(void*)>(this, 10)(this);
}

Vector CBaseEntity::GetAbsOriginServer()
{
	if ((*(DWORD *)((DWORD)this + 0x0D0) >> 11) & 1)
		CalcAbsolutePositionServer();
	return *(Vector*)((DWORD)this + 0x1D8);
}

int CBaseEntity::entindex()
{
	return GetVFunc<int(__thiscall*)(void*)>(this, (m_dwEntIndexOffset / 4))(this);
}

int CBaseEntity::entindexServer()
{
#ifdef HOOK_LAG_COMPENSATION
	static DWORD m_Network_Adr = NULL;
	if (!m_Network_Adr)
	{
		const char *sig = "8B  4F  1C  85  C9  74  0B  A1  ??  ??  ??  ??  2B  48  60  C1  F9  04";
		m_Network_Adr = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)sig, strlen(sig));
		if (!m_Network_Adr)
			DebugBreak();

		m_Network_Adr = *(DWORD*)(m_Network_Adr + 8);
	}
	DWORD ecx = *(DWORD*)((DWORD)this + 0x1C);
	if (ecx)
	{
		DWORD eax = *(DWORD*)m_Network_Adr;
		ecx -= *(DWORD*)(eax + 0x60);
		ecx = ecx >> 4;
		return ecx;
	}
#endif
	return -1;
}

float CBaseEntity::GetCurrentFeetYawServer()
{
	void* animstate = GetPlayerAnimStateServer();
	if (animstate)
	{
		return *(float*)((DWORD)animstate + m_flCurrentFeetYawServer);
	}
	return 0.0f;
}

float CBaseEntity::GetCurrentFeetYaw()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_flCurrentFeetYaw;
	}
	return 0.0f;
}

void CBaseEntity::SetCurrentFeetYaw(float yaw)
{
	//0x7C = pitch
	//0x74 = some type of radians?
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_flCurrentFeetYaw = yaw;
	}
}

float CBaseEntity::GetGoalFeetYawServer()
{
	void* animstate = GetPlayerAnimStateServer();
	if (animstate)
	{
		return *(float*)((DWORD)animstate + m_flGoalFeetYawServer);
	}
	return 0.0f;
}

float CBaseEntity::GetGoalFeetYaw()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_flGoalFeetYaw;
	}
	return 0.0f;
}

void CBaseEntity::SetGoalFeetYaw(float yaw)
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_flGoalFeetYaw = yaw;
	}
}

float CBaseEntity::GetFriction()
{
	return *(float*)((DWORD)this + m_flFriction);
}

void CBaseEntity::SetFriction(float friction)
{
	*(float*)((DWORD)this + m_flFriction) = friction;
}

float CBaseEntity::GetStepSize()
{
	return *(float*)((DWORD)this + m_flStepSize);
}

void CBaseEntity::SetStepSize(float stepsize)
{
	*(float*)((DWORD)this + m_flStepSize) = stepsize;
}

float CBaseEntity::GetMaxSpeed()
{
	return *(float*)((DWORD)this + m_flMaxSpeed);
}

void CBaseEntity::SetMaxSpeed(float maxspeed)
{
	*(float*)((DWORD)this + m_flMaxSpeed) = maxspeed;
}

bool CBaseEntity::IsParentChanging() 
{ 
	return *(DWORD*)((DWORD)this + m_hNetworkMoveParent) != *(DWORD*)((DWORD)this + m_pMoveParent); 
}

void CBaseEntity::SetLocalVelocity(const Vector& vecVelocity)
{
	if (GetVelocity() != vecVelocity)
	{
		InvalidatePhysicsRecursive(VELOCITY_CHANGED);
		SetVelocity(vecVelocity);
	}
}

void CBaseEntity::StandardBlendingRules(CStudioHdr *hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask)
{
	typedef void(__thiscall* OriginalFn)(CBaseEntity*, CStudioHdr*, Vector[], QuaternionAligned[], float, int);
	GetVFunc<OriginalFn>(this, standardblendingrules_offset)(this, hdr, pos, q, currentTime, boneMask);
}

void CBaseEntity::BuildTransformations(CStudioHdr *hdr, Vector *pos, Quaternion *q, const matrix3x4_t &cameraTransform, int boneMask, byte *boneComputed)
{
	typedef void(__thiscall* OriginalFn)(CBaseEntity*, CStudioHdr *, Vector *, Quaternion *, const matrix3x4_t &, int, byte *);
	GetVFunc<OriginalFn>(this, buildtransformations_offset)(this, hdr, pos, q, cameraTransform, boneMask, boneComputed);
}

void CBaseEntity::UpdateIKLocks(float currentTime)
{
	typedef void(__thiscall* oUpdateIKLocks)(PVOID, float currentTime);
	GetVFunc<oUpdateIKLocks>(this, 186)(this, currentTime);
}

void CBaseEntity::CalculateIKLocks(float currentTime)
{
	typedef void(__thiscall* oCalculateIKLocks)(PVOID, float currentTime);
	GetVFunc<oCalculateIKLocks>(this, 187)(this, currentTime);
}

void CBaseEntity::ControlMouth(CStudioHdr *pStudioHdr)
{
	typedef void(__thiscall* oControlMouth)(PVOID, CStudioHdr*);
	GetVFunc<oControlMouth>(this, 191)(this, pStudioHdr);
}

void CBaseEntity::Wrap_SolveDependencies(DWORD m_pIk, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed)
{
	//typedef void(__thiscall *pSolveDependencies) (DWORD *m_pIk, DWORD UNKNOWN, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed);

	SolveDependencies(m_pIk, pos, q, bonearray, computed);
}

void CBaseEntity::Wrap_UpdateTargets(DWORD m_pIk, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed)
{
	//typedef void(__thiscall *pUpdateTargets) (DWORD *m_pIk, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed);

	UpdateTargets(m_pIk, pos, q, bonearray, computed);
}

void CBaseEntity::Wrap_AttachmentHelper(CStudioHdr* hdr)
{
	AttachmentHelper(this, hdr);
}

void CBaseEntity::Wrap_IKInit(DWORD m_pIk, CStudioHdr * hdrs, QAngle &angles, Vector &pos, float flTime, int iFramecounter, int boneMask)
{
	//typedef void(__thiscall *pInit) (DWORD *m_pIk, studiohdr_t * hdrs, Vector &angles, Vector &pos, float flTime, int iFramecounter, int boneMask);
	IKInit(m_pIk, hdrs, angles, pos, flTime, iFramecounter, boneMask);
}

int CBaseEntity::GetUnknownSetupBonesInt()
{
	typedef int(__thiscall* OriginalFn)(CBaseEntity*);
	return GetVFunc<OriginalFn>(this, unknownsetupbonesfn_offset)(this);
}

void CBaseEntity::DoUnknownSetupBonesCall(CStudioHdr *hdr, Vector *pos, Quaternion *q, matrix3x4_t* bonearray, byte* computed, DWORD* m_pIK)
{
	typedef void(__thiscall* OriginalFn)(CBaseEntity*, CStudioHdr*, Vector*, Quaternion*, matrix3x4_t*, byte*, DWORD*);
	GetVFunc<OriginalFn>(this, unknownsetupbonesfn2_offset)(this, hdr, pos, q, bonearray, computed, m_pIK);
}

DWORD CBaseEntity::Wrap_CreateIK()
{
	void* memory = malloc(0x1070);
	if (memory)
	{
		return CreateIK(memory);
	}
	return (DWORD)memory;
}

void CBaseEntity::MDLCACHE_CRITICAL_SECTION()
{
	if (!GetModelPtr() && (*(int(__thiscall **)(CBaseEntity*))(*(DWORD *)(DWORD)this + 32))(this))
	{
		MDLCacheCriticalSectionCall(this);
	}
}