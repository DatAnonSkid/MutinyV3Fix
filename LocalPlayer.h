#pragma once

#include "GameMemory.h"
#include "Interfaces.h"
#include "BaseEntity.h"
#include "BaseCombatWeapon.h"
#include "Netchan.h"

struct MyWeapon
{
	int iItemDefinitionIndex;
	bool IsGun;
	bool IsShotgun;
	bool IsPistol;
	bool IsSniper;
	bool IsKnife;
	bool IsGrenade;
	bool IsC4;
	bool IsAutoSniper;
	bool IsRevolver;
};

extern HookedEntity LocalPlayerHookedEntity;

//Local Player
class MyPlayer
{
public:
	CBaseEntity *Entity;
	QAngle LastEyeAngles; //Last eye angles sent to the server
	QAngle CurrentEyeAngles; //Current real eye angles without any antiaim
	QAngle TargetEyeAngles; //Eye angles to send to the server
	QAngle EyeAnglesOnServer; //Eye angles received from the server
	QAngle LastRealEyeAngles; //Last real eye angles sent to the server
	QAngle LastFakeEyeAngles; //Last fake eye angles sent to the server
	QAngle LastAimPunch; //Last local player's aimpunch
	QAngle CurrentAimPunchDelta; //Delta between current aim punch and last aim punch
	QAngle AimPunchSubtracted; //The amount of aimpunch subtracted
	QAngle AimbotAimPunchSubtracted;
	QAngle angAbsRotationAfterUpdatingClientSideAnimation;
	QAngle angRotationAfterUpdatingClientSideAnimation;
	QAngle angNetworkAnglesAfterUpdatingClientSideAnimation;

	QAngle IsShotAimPunch; //The aimpunch when the local player was shot
	QAngle IsShotAimPunchDelta; //The delta between the aimpunch after getting shot and last CreateMove aimpunch
	bool fakewalk = false;
	Vector OriginalMovementValues; //Movement before setting to 0 for engine prediction during fake walk
	bool isastronaut = false;
	int nasaindex = 0;
	bool airstuck = false;
	bool bSpecialFiring = false;
	bool IsWaitingToSetIsShotAimPunch; //Local player was shot and hack is waiting to receive the resulting aimpunch
	bool IsShot; //Local player was shot
	bool EnemyIsWithinKnifeRange; //Is there an enemy within knife radius of us?
	bool PressingJumpButton;
	bool RanEdgeAntiAim;
	bool RanManualAntiAim;
	bool bInPrediction;
	BOOL IsAlive;
	int Real_m_nInSequencenumber;
	//int Original_m_nInSequenceNr;
	//float Original_last_received;
	bool ModifiedSequenceNr;
	Vector LastAimbotBonePosition;
	float LastAimbotCurrentFeetYawClamped; //clamped to -180/180
	float LastAimbotGoalFeetYawClamped; //clamped to -180/180
	float LastAimbotLowerBodyYaw; //naturally clamped to -180/180

	unsigned char bIsValveDS;

	HookedEntity* HookedBaseEntity = nullptr;
	
	//Weapon
	CBaseCombatWeapon *CurrentWeapon;
	CBaseCombatWeapon *LastWeapon;
	MyWeapon WeaponVars;

	float LowerBodyYaw = 0.0f;
	bool LowerBodyYawUpdated = false;
	int LowerBodyYawUpdateTick = 0;
	float NextLowerBodyUpdateTime = 0.0f;
	float NextEstimatedLowerBodyUpdateTime = 0.0f;
	int OriginalObserverMode = 0;
	float TimeSinceWeaponChange = 0.0f;
	int iLastSendPacketCommandNumber = 0;

	void DrawUserCmdInfo(CUserCmd* cmd);
	void BeginEnginePrediction(CUserCmd *cmd);
	void FinishEnginePrediction(CUserCmd *cmd);
	void SetRenderAngles(CUserCmd* cmd, bool StrafeModifiedAngles, bool DontSetViewAngles, void* FramePointer, bool RanAntiAim);
	void StopPlayerMovement(CUserCmd* cmd, int& buttons)
	{
		//Return for now because I don't know if this is causing Untrusted bans
		WriteFloat((uintptr_t)&cmd->forwardmove, 0.0f);
		WriteFloat((uintptr_t)&cmd->sidemove, 0.0f);
		buttons &= ~IN_MOVELEFT;
		buttons &= ~IN_MOVERIGHT;
		buttons &= ~IN_FORWARD;
		buttons &= ~IN_BACK;
		buttons &= ~IN_JUMP;
	}
	void Crouch(int& buttons) { buttons |= IN_DUCK; }
	void ClearVariables() {
		memset(this, 0, sizeof(MyPlayer));
	}
	bool IsManuallyFiring(CBaseCombatWeapon *weapon)
	{
		return weapon && ((ReadInt((uintptr_t)&CurrentUserCmd.cmd->buttons) & IN_ATTACK) || (weapon && (ReadInt((uintptr_t)&CurrentUserCmd.cmd->buttons) & IN_ATTACK2) && weapon->GetItemDefinitionIndex() == WEAPON_REVOLVER));
	}
	void UpdateCurrentWeapon()
	{
		CurrentWeapon = Entity->GetWeapon();
		if (CurrentWeapon != LastWeapon)
		{
			TimeSinceWeaponChange = Time;
			LastWeapon = CurrentWeapon;
			if (CurrentWeapon)
			{
				int index = CurrentWeapon->GetItemDefinitionIndex();
				WeaponVars.iItemDefinitionIndex = index;
				if (index == WEAPON_C4)
				{
					WeaponVars.IsGun = false;
					WeaponVars.IsShotgun = false;
					WeaponVars.IsPistol = false;
					WeaponVars.IsSniper = false;
					WeaponVars.IsKnife = false;
					WeaponVars.IsGrenade = false;
					WeaponVars.IsC4 = true;
					WeaponVars.IsAutoSniper = false;
					WeaponVars.IsRevolver = false;

				}
				else if (CurrentWeapon->IsKnife(index))
				{
					WeaponVars.IsGun = false;
					WeaponVars.IsShotgun = false;
					WeaponVars.IsPistol = false;
					WeaponVars.IsSniper = false;
					WeaponVars.IsKnife = true;
					WeaponVars.IsGrenade = false;
					WeaponVars.IsC4 = false;
					WeaponVars.IsAutoSniper = false;
					WeaponVars.IsRevolver = false;
				}
				else if (CurrentWeapon->IsSniper(true, index))
				{
					WeaponVars.IsGun = true;
					WeaponVars.IsShotgun = false;
					WeaponVars.IsPistol = false;
					WeaponVars.IsSniper = true;
					WeaponVars.IsKnife = false;
					WeaponVars.IsGrenade = false;
					WeaponVars.IsC4 = false;
					WeaponVars.IsAutoSniper = index == WEAPON_G3SG1 || index == WEAPON_SCAR20;
					WeaponVars.IsRevolver = false;
				}
				else if (CurrentWeapon->IsGrenade(index))
				{
					WeaponVars.IsGun = false;
					WeaponVars.IsShotgun = false;
					WeaponVars.IsPistol = false;
					WeaponVars.IsSniper = false;
					WeaponVars.IsKnife = false;
					WeaponVars.IsGrenade = true;
					WeaponVars.IsC4 = false;
					WeaponVars.IsAutoSniper = false;
					WeaponVars.IsRevolver = false;
				}
				else if (CurrentWeapon->IsShotgun(index))
				{
					WeaponVars.IsGun = true;
					WeaponVars.IsShotgun = true;
					WeaponVars.IsPistol = false;
					WeaponVars.IsSniper = false;
					WeaponVars.IsKnife = false;
					WeaponVars.IsGrenade = false;
					WeaponVars.IsC4 = false;
					WeaponVars.IsAutoSniper = false;
					WeaponVars.IsRevolver = false;
				}
				else if (CurrentWeapon->IsPistol(index))
				{
					WeaponVars.IsGun = true;
					WeaponVars.IsShotgun = false;
					WeaponVars.IsPistol = true;
					WeaponVars.IsSniper = false;
					WeaponVars.IsKnife = false;
					WeaponVars.IsGrenade = false;
					WeaponVars.IsC4 = false;
					WeaponVars.IsAutoSniper = false;
					WeaponVars.IsRevolver = index == WEAPON_REVOLVER;
				}
				else
				{
					WeaponVars.IsGun = true;
					WeaponVars.IsShotgun = false;
					WeaponVars.IsPistol = false;
					WeaponVars.IsSniper = false;
					WeaponVars.IsKnife = false;
					WeaponVars.IsGrenade = false;
					WeaponVars.IsC4 = false;
					WeaponVars.IsAutoSniper = false;
					WeaponVars.IsRevolver = false;
				}
			}
			else
			{
				//No CurrentWeapon
				WeaponVars.iItemDefinitionIndex = 0;
				WeaponVars.IsGun = false;
				WeaponVars.IsShotgun = false;
				WeaponVars.IsPistol = false;
				WeaponVars.IsSniper = false;
				WeaponVars.IsKnife = false;
				WeaponVars.IsGrenade = false;
				WeaponVars.IsC4 = false;
				WeaponVars.IsAutoSniper = false;
				WeaponVars.IsRevolver = false;
			}
		}
	}
	void CheckNASA(CUserCmd *cmd)
	{
		if ((GetAsyncKeyState(NASAKeyTxt.iValue) & (1 << 16)) || (NASAWhileMovingChk.Checked && (cmd->forwardmove != 0 || cmd->sidemove != 0)))
		{
			if (cmd->buttons & IN_ATTACK || cmd->weaponselect != 0)
			{
				isastronaut = false;
			}
			else
			{
				isastronaut = true;
			}
		}
		else
		{
			isastronaut = false;
		}

		if (!NASAAlternativeChk.Checked)
		{
			if (isastronaut)
			{
				DWORD ClientState = (DWORD)ReadInt(pClientState);
				if (ClientState)
				{
					INetChannel *netchan = (INetChannel*)ReadInt((uintptr_t)(ClientState + 0x9C));
					if (netchan)
					{
						netchan->m_nOutSequenceNr += nasaindex ? 50000 : 0;
						if (nasaindex > NASAIndexTxt.iValue)
							nasaindex = 0;
						else
							nasaindex++;
					}
				}
			}
			else
			{
				nasaindex = 0;
			}
		}
	}
	void CheckAirStuck(CUserCmd *cmd)
	{
		if ((GetAsyncKeyState(AirStuckKeyTxt.iValue) & (1 << 16)) && Time >= nextairstuck)
		{
			airstuck = !airstuck;
			nextairstuck = Time + 0.2f;
		}

		if (airstuck && !(ReadInt((uintptr_t)&cmd->buttons) & IN_ATTACK) && !(ReadInt((uintptr_t)&cmd->buttons) & IN_ATTACK2))
			WriteInt((uintptr_t)&cmd->tick_count, INT_MAX);
	}
	CBaseEntity* Get(MyPlayer* dest)
	{
		CBaseEntity* pNewPlayer = Interfaces::ClientEntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
		if (pNewPlayer != dest->Entity)
		{
			if (dest->HookedBaseEntity)
			{
				dest->HookedBaseEntity->hook->ClearClassBase();
				delete dest->HookedBaseEntity->hook;
				dest->HookedBaseEntity = nullptr;
			}

			if (pNewPlayer)
			{
				dest->ClearVariables();
				dest->HookedBaseEntity = &LocalPlayerHookedEntity;
				dest->HookedBaseEntity->entity = pNewPlayer;
				dest->HookedBaseEntity->index = pNewPlayer->index;
				dest->HookedBaseEntity->hook = new VTHook((PDWORD*)pNewPlayer);
				dest->HookedBaseEntity->OriginalHookedSub1 = dest->HookedBaseEntity->hook->HookFunction((DWORD)&HookedUpdateClientSideAnimation, OffsetOf_UpdateClientSideAnimation);
				dest->HookedBaseEntity->OriginalHookedSub2 = dest->HookedBaseEntity->hook->HookFunction((DWORD)&HookedPlayFootstepSound, PlayFootstepSoundOffset);
			}
			dest->Entity = pNewPlayer;
		}
		return pNewPlayer;
	}
	void DrawTextInFrontOfPlayer(char *text, float flStayTime)
	{
		Vector vForward;
		QAngle realangs;
		Interfaces::Engine->GetViewAngles(realangs);
		AngleVectors(realangs, &vForward);
		Interfaces::DebugOverlay->AddTextOverlay(Entity->GetLocalOrigin() + vForward * 250.0f, flStayTime, text);
	}
private:
	//Engine Prediction
	int originalrandomseed;
	int originalflags;
	float originalframetime;
	float originalcurtime;
	CMoveData NewMoveData;
	CUserCmd *originalcurrentcommand;
	DWORD originalpredictionplayer;

	//Others
	float nextairstuck = 0.0f;
};
extern MyPlayer LocalPlayer;
