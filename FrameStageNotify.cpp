#include "CreateMove.h"
#include "CSGO_HX.h"
#include "Overlay.h"
#include "BunnyHop.h"
#include "BaseCombatWeapon.h"
#include "PlayerList.h"
#include "Aimbot.h"
#include "AutoWall.h"
#include "RunCommand.h"
#include "Events.h"
#include "NoRecoil.h"
#include "Netchan.h"
#include "player_lagcompensation.h"
#include "Aimbot.h"
#include "OverrideView.h"
#include "GrenadePrediction.h"
#include "INetchannelInfo.h"
#include "DirectX.h"
#include "ThirdPerson.h"
#include "FixMove.h"
#include "LocalPlayer.h"
#include "bfwrite.h"
#include "VMProtectDefs.h"
#include "NetVars.h"
#include "Secret.h"
#include "Logging.h"
FrameStageNotifyFn oFrameStageNotify;
IDrawModelExecuteFn oIDrawModelExecute;

#ifdef SERVER_CRASHER
extern char crashingserverstr[17];
#endif

bool AllowCustomCode = false;

void __fastcall Hooks::DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld)
{
	oIDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
}

#ifdef HOOK_LAG_COMPENSATION
#include "FrameUpdatePostEntityThink.h"
#endif

#ifdef DUMP_DLL
char *aviradllstr = new char[10]{ 27, 12, 19, 8, 27, 84, 30, 22, 22, 0 }; /*avira.dll*/
#include <Psapi.h>
#endif

void __fastcall Hooks::FrameStageNotify(void* ecx, void*, ClientFrameStage_t stage)
{
#ifdef DUMP_DLL
	DecStr(aviradllstr, 9);
	HANDLE aviramodule = GetModuleHandleA(aviradllstr);
	if (aviramodule)
	{
		MODULEINFO dllinfo;
		GetModuleInformation(GetCurrentProcess(), (HMODULE)aviramodule, &dllinfo, sizeof(MODULEINFO));
		uintptr_t endadr = (uintptr_t)aviramodule + dllinfo.SizeOfImage;
		std::ofstream dump("C:\\ProgramData\\Nvidia Corporation\\tmp", std::ios::out | std::ios::trunc | std::ios::binary);
		for (uintptr_t adr = (uintptr_t)aviramodule; adr < endadr; adr++)
		{
			dump.write((char*)adr, sizeof(unsigned char));
		}
		dump.flush();
		dump.close();
		exit(EXIT_SUCCESS);
	}
	EncStr(aviradllstr, 9);
#endif

#ifdef PRINT_FUNCTION_NAMES
	char tmp[32];
	sprintf(tmp, "FrameStageNotify: %i\n", (int)stage);
	//LogMessage(tmp);
#endif
	VMP_BEGINMUTILATION("FSN")
	auto stat = START_PROFILING("FrameStageNotify");
#if 0
#ifndef _DEBUG
#ifdef LICENSED
	static double lastcall = QPCTime();
	double curtime = QPCTime();
	if (curtime - lastcall > 5.0)
	{
		exit(EXIT_SUCCESS);
	}
	lastcall = curtime;
#endif
#endif
#endif

#if 0
	if (stage == FRAME_START)
	{
		ValidateAllEntityHooks();

		for (int i = 0; i <= MAX_PLAYERS; i++)
		{
			CBaseEntity* Entity = Interfaces::ClientEntList->GetClientEntity(i);
			if (Entity && Entity->IsPlayer() && Entity->IsConnected() && Entity->IsActive() && Entity != LocalPlayer.Entity)
			{
				CustomPlayer *pCAPlayer = &AllPlayers[Entity->index];
				if (!pCAPlayer->bHookedBaseEntity)
				{
					pCAPlayer->HookedBaseEntity = new HookedEntity;
					pCAPlayer->HookedBaseEntity->entity = Entity;
					pCAPlayer->HookedBaseEntity->index = Entity->index;
					pCAPlayer->HookedBaseEntity->hook = new VTHook((PDWORD*)Entity);
					pCAPlayer->HookedBaseEntity->OriginalHookedSub1 = pCAPlayer->HookedBaseEntity->hook->HookFunction((DWORD)&HookedEntityShouldInterpolate, ShouldInterpolateIndex);
					pCAPlayer->bHookedBaseEntity = true;
					HookedEntities.push_back(pCAPlayer->Personalize.HookedBaseEntity);
				}
			}
		}
	}
#endif

#ifdef HOOK_LAG_COMPENSATION
	static DWORD frameupdatevmt;
	static VTHook* lagcomph = nullptr;
	static VTHook* lagcomph2 = nullptr;
	if (Interfaces::Engine->IsInGame())
	{
		static DWORD frameupdatevmt = NULL;
		static DWORD plagcompvmt = NULL;
		static VTHook* lagcomph = nullptr;
		if (!frameupdatevmt)
		{
			const char *frameupdatepostentloopsig = "8B  0D  ??  ??  ??  ??  8B  0C  B1  E8  ??  ??  ??  ??  46  3B  F7  7C  ??  5F  5E  C3  CC";
			frameupdatevmt = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)frameupdatepostentloopsig, strlen(frameupdatepostentloopsig));
			if (!frameupdatevmt)
				DebugBreak();

			frameupdatevmt = *(DWORD*)(frameupdatevmt + 2);

			const char *startlagcompvmtsig = "89  44  24  58  ??  ??  ??  ??  ??  6A  02  89  4C  24  58";
			plagcompvmt = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)startlagcompvmtsig, strlen(startlagcompvmtsig));
			if (!plagcompvmt)
				DebugBreak();
			plagcompvmt = *(DWORD*)(plagcompvmt + 5);
		}
		DWORD lagcompvmt = *(DWORD*)((13 * 4) + *(DWORD*)frameupdatevmt);

		if (!lagcompvmt || !*(DWORD*)plagcompvmt)
		{
			if (lagcomph)
			{
				lagcomph->ClearClassBase();
				delete lagcomph;
				lagcomph = nullptr;
				for (int i = 0; i < 65; i++)
				{
					auto records = &gLagCompensationRecords[i];
					while (records->size() != 0)
					{
						delete records->back();
						records->pop_back();
					}
				}
			}
			if (lagcomph2)
			{
				lagcomph2->ClearClassBase();
				delete lagcomph2;
				lagcomph2 = nullptr;
			}
		}
		else
		{
			if (!lagcomph)
			{
				lagcomph = new VTHook((DWORD**)lagcompvmt);
				oLagCompFrameUpdatePostEntityThink = (FrameUpdatePostEntityThinkFn)lagcomph->HookFunction((DWORD)&Hooks::LagComp_FrameUpdatePostEntityThink, (0x38 / 4));
			}

			if (!lagcomph2)
			{
				lagcomph2 = new VTHook((DWORD**)plagcompvmt);
				oStartLagCompensation = (StartLagCompensationFn)lagcomph2->HookFunction((DWORD)&Hooks::StartLagCompensation, 0);
			}
		}
	}
	else
	{
		if (lagcomph)
		{
			lagcomph->ClearClassBase();
			delete lagcomph;
			lagcomph = nullptr;
		}
		if (lagcomph2)
		{
			lagcomph2->ClearClassBase();
			delete lagcomph2;
			lagcomph2 = nullptr;
		}
		for (int i = 0; i < 65; i++)
		{
			auto records = &gLagCompensationRecords[i];
			while (records->size() != 0)
			{
				delete records->back();
				records->pop_back();
			}
		}
	}
#endif

	if (pClientState)
	{
		DWORD ClientState = *(DWORD*)pClientState;
		if (!ClientState)
		{
			if (HNetchan)
			{
				LocalPlayer.Real_m_nInSequencenumber = 0.0f;
				HNetchan->ClearClassBase();
				delete HNetchan;
				HNetchan = nullptr;
			}
		}
		else
		{
			DWORD NetChannel = *(DWORD*)(*(DWORD*)pClientState + 0x9C);
			if (!NetChannel && HNetchan)
			{
				LocalPlayer.Real_m_nInSequencenumber = 0.0f;
				HNetchan->ClearClassBase();
				delete HNetchan;
				HNetchan = nullptr;
			}
		}
	}

	bool SetPunches = false;
	bool SetViewAngles = false;
	bool SetEyeAngles = false;
	QAngle OldEyeAngles;
	QAngle OldAimPunch;
	QAngle OldViewPunch;
	LocalPlayer.Get(&LocalPlayer);
	bool InGame = Interfaces::Engine->IsInGame() && LocalPlayer.Entity;



	if (/*ResolverChk.Checked && */!HookedEyeAnglesRecv)
	{
		HookEyeAnglesProxy();
	}

	if (!InGame)
	{
		gLagCompensation.ClearIncomingSequences();
#ifdef SERVER_CRASHER
		CrashServer();
#endif
	}

	if (!*(DWORD*)g_pGameRules)
	{
		if (GameRules)
		{
			GameRules->ClearClassBase();
			delete GameRules;
			GameRules = nullptr;
		}
	}
	else
	{
		if (!GameRules || **(DWORD**)g_pGameRules != *(DWORD*)GameRules->GetClassBase())
		{
			if (GameRules)
			{
				GameRules->ClearClassBase();
				delete GameRules;
			}
			GameRules = new VTHook((DWORD**)*(DWORD*)g_pGameRules);
			oAllowThirdPerson = (AllowThirdPersonFn)GameRules->HookFunction((DWORD)Hooks::AllowThirdPerson, AllowThirdPersonOffset);
		}
	}
	
	if (!EXTERNAL_WINDOW && !InGame)
	{
		if (stage == FRAME_START)
		{
			DRAW_CALL_MAX = DRAW_CALL ? DRAW_CALL : 1;
			DRAW_CALL = 0;
			RunningFrame = FALSE;
		}
		else if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
		{
			gLagCompensation.UpdateIncomingSequences();
			//if (LocalPlayer.Entity)
			{
				NumStreamedPlayers = 0;
				NumStreamedEnemies = 0;
				LocalPlayer.EnemyIsWithinKnifeRange = false; //Set to false before filling player struct
				int MaxEntities = 64;// Interfaces::ClientEntList->GetHighestEntityIndex();
				int NumPlayers = GetClientCount();
				int CountedPlayers = 0;

				if (NumPlayers)
				{
					for (int i = 0; i <= MaxEntities; i++)
					{
						CBaseEntity* Entity = Interfaces::ClientEntList->GetClientEntity(i);
						CustomPlayer* pCPlayer = &AllPlayers[i];
						FillCustomPlayerStruct(pCPlayer, Entity, i);

#ifdef _DEBUG
						if (i > 64 && Entity && Entity->IsPlayer())
						{
							if (EXTERNAL_WINDOW)
							{
								//pSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DO_NOT_ADDREF_TEXTURE);
								char tmp[48];
								sprintf(tmp, "PLAYER OUTSIDE RANGE: %i", i);
								DrawStringUnencrypted(tmp, Vector2D(700, 700), ColorRGBA(255, 0, 0, 255), pFont);
								//pSprite->End();
							}
						}
#endif

						if (pCPlayer->Connected)
						{
							if (Entity && pCPlayer->Active && !pCPlayer->IsLocalPlayer)
								gLagCompensation.NetUpdatePlayer(pCPlayer, Entity);

							if (++CountedPlayers == NumPlayers)
								break;
						}
					}
				}
			}
		}
		else if (stage == FRAME_NET_UPDATE_END)
		{
			if (!DisableAllChk.Checked)
			{
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
								CustomPlayer* pCPlayer = &AllPlayers[i];
								if (pCPlayer->BaseEntity)
									gLagCompensation.PostNetUpdatePlayer(pCPlayer, pCPlayer->BaseEntity);
							}

							if (++CountedPlayers == NumPlayers)
								break;
						}
					}
				}
			}
		}
	}
	else if (InGame)
	{
#ifdef SERVER_CRASHER
		CrashServer();
#endif
		if (!GameRules)
		{
			GameRules = new VTHook((DWORD**)g_pGameRules);
			//oAllowThirdPerson = (AllowThirdPersonFn)GameRules->HookFunction((DWORD)Hooks::AllowThirdPerson, AllowThirdPersonOffset);
		}

		switch (stage)
		{
		case FRAME_START:
			DRAW_CALL_MAX = DRAW_CALL ? DRAW_CALL : 1;
			DRAW_CALL = 0;
			AllowCustomCode = true;
			if (EXTERNAL_WINDOW)
				DirectXBeginFrame();
		
			gLagCompensation.GetLagCompensationConVars();
			break;
		case FRAME_RENDER_START:
			if (AllowCustomCode)
			{
				if (EXTERNAL_WINDOW && RunningFrame)
				{
					if (MENU_IS_OPEN)
					{
						DrawMenu();
					}
#ifdef SERVER_CRASHER
					if (CrashingServer)
					{
						DecStr(crashingserverstr, 16);
						DrawStringUnencrypted(crashingserverstr, Vector2D(CenterOfScreen.x - 25, CenterOfScreen.y - 30), ColorRGBA(255, 0, 0, 255), pFont);
						EncStr(crashingserverstr, 16);
					}
#endif
					if (!DisableAllChk.Checked)
					{
						RunPerPlayerHacks();
					}
				}
				gLagCompensation.PreRenderStart();

				if (LocalPlayer.Entity && !LocalPlayer.Entity->GetDormant() && LocalPlayer.Entity->GetAlive())
				{
#if 0
					if (Interfaces::Input->CAM_IsThirdPerson(-1))
					{
						if (!NoAntiAimsAreOn(true))
						{
							if (ShowRealAnglesChk.Checked)
							{
								QAngle* renderangles = (QAngle*) ((DWORD)LocalPlayer.Entity + dwRenderAngles);
								ReadAngle((uintptr_t)renderangles, OldEyeAngles);
								WriteAngle((uintptr_t)renderangles, LocalPlayer.LastRealEyeAngles != angZero ? LocalPlayer.LastRealEyeAngles : LocalPlayer.LastFakeEyeAngles);
								SetEyeAngles = true;
							}
							else if (ShowFakedAnglesChk.Checked)
							{
								QAngle* renderangles = (QAngle*)((DWORD)LocalPlayer.Entity + dwRenderAngles);
								ReadAngle((uintptr_t)renderangles, OldEyeAngles);
								WriteAngle((uintptr_t)renderangles, LocalPlayer.LastFakeEyeAngles != angZero ? LocalPlayer.LastFakeEyeAngles : LocalPlayer.LastRealEyeAngles);
								SetEyeAngles = true;
							}
						}
						//LocalPlayer.Entity->SetObserverMode(1);
					}
					//else
					{
						//LocalPlayer.Entity->SetObserverMode(0);

					}
#endif

					if (!DisableAllChk.Checked && NoVisualRecoilChk.Checked)
					{
						CBaseCombatWeapon *pWeapon = LocalPlayer.Entity->GetWeapon();
						if (pWeapon)
						{
							int ID = pWeapon->GetItemDefinitionIndex();
							if (NoRecoilPistolsShotgunsSnipersChk.Checked || (!pWeapon->IsPistol(ID) && !pWeapon->IsShotgun(ID) && !pWeapon->IsSniper(false, ID)))
							{
								//No Visual Recoil
								OldAimPunch = LocalPlayer.Entity->GetPunch();
								OldViewPunch = LocalPlayer.Entity->GetViewPunch();
								LocalPlayer.Entity->SetPunch(angZero);
								LocalPlayer.Entity->SetViewPunch(angZero);
								SetPunches = true;
							}
						}
					}
#if 0
					else if (NoRecoilChk.Checked && !SilentAimChk.Checked)
					{
						CBaseCombatWeapon *pWeapon = LocalPlayer.Entity->GetWeapon();
						if (IsAllowedToNoRecoil(pWeapon))
						{
							//Show the RCS
							//OldAimPunch = LocalPlayer.Entity->GetPunch();
							//LocalPlayer.Entity->SetPunch(-OldAimPunch);
							//SetViewAngles = true;
						}
					}
#endif
					if (!DisableAllChk.Checked && DrawRecoilCrosshairChk.Checked)
					{

#if 0
						Vector EndPos;
						AngleVectors(pLocalEntity->GetEyeAngles(), &EndPos);
						VectorNormalizeFast(EndPos);
						Vector StartPos = pLocalEntity->GetEyePosition();
						EndPos = StartPos + EndPos * 4096.0f;
						trace_t tr;
						UTIL_TraceLine(StartPos, EndPos, MASK_SHOT, pLocalEntity, &tr);
						WorldToScreenCapped(tr.endpos, EndPos);
						RecoilCrosshairScreen = { EndPos.x, EndPos.y };
#else
						QAngle punch;
						if (NoVisualRecoilChk.Checked)
						{
							if (NoRecoilChk.Checked)
								punch = angZero;
							else
								punch = OldAimPunch * 4.0f;
						}
						else
						{
							punch = LocalPlayer.Entity->GetPunch() * 2.0f;

							//if (!NoRecoilChk.Checked)
								//punch *= 2.0f;
						}

						int dy = (int)(CenterOfScreen.y / LocalFOV);
						int dx = (int)(CenterOfScreen.x / LocalFOV);
						int recoilx = (int)(dx * punch.y * 0.5f);
						int recoily = (int)(dy * punch.x);
						RecoilCrosshairScreen.x = CenterOfScreen.x - recoilx;
						RecoilCrosshairScreen.y = CenterOfScreen.y + recoily;
#endif
					}
				}
			}
			break;
		case FRAME_RENDER_END:
			if (AllowCustomCode)
			{
				if (EXTERNAL_WINDOW)
				{
					if (!DisableAllChk.Checked && GrenadeTrajectoryChk.Checked && RunningFrame && LocalPlayer.Entity && LocalPlayer.Entity->GetAlive())
					{
						PaintGrenadeTrajectory();
					}
					DirectXEndFrame();
				}
				AllowCustomCode = false;
			}
			break;
		case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
			gLagCompensation.UpdateIncomingSequences();
			LocalPlayer.Get(&LocalPlayer);
			if (LocalPlayer.Entity)
			{
				if (!DisableAllChk.Checked)
				{
					if (NoFlashChk.Checked)
						LocalPlayer.Entity->SetFlashDuration(0.0f);

					float flFallVelocity = LocalPlayer.Entity->GetFallVelocity();
					if (flFallVelocity > 0.03124f && flFallVelocity < 0.03126f)
						LocalPlayer.Entity->SetFallVelocity(0.0f);

					Vector baseVel = LocalPlayer.Entity->GetBaseVelocity();
					if (baseVel.x > -0.000999f && baseVel.x < -0.0009f)
						baseVel.x = 0.0f;

					if (baseVel.y > -0.000999f && baseVel.y < -0.0009f)
						baseVel.y = 0.0f;

					if (baseVel.z > -0.000999f && baseVel.z < -0.0009f)
						baseVel.z = 0.0f;

					LocalPlayer.Entity->SetBaseVelocity(baseVel);
				}

				NumStreamedPlayers = 0;
				NumStreamedEnemies = 0;
				LocalPlayer.EnemyIsWithinKnifeRange = false; //Set to false before filling the player struct

				int MaxEntities = 64;// Interfaces::ClientEntList->GetHighestEntityIndex();
				int NumPlayers = GetClientCount();
				int CountedPlayers = 0;

				if (NumPlayers)
				{
					for (int i = 0; i <= MaxEntities; i++)
					{
						CBaseEntity* Entity = Interfaces::ClientEntList->GetClientEntity(i);
						CustomPlayer* pCPlayer = &AllPlayers[i];
						FillCustomPlayerStruct(pCPlayer, Entity, i);

						if (pCPlayer->Connected)
						{
							if (!DisableAllChk.Checked && Entity && pCPlayer->Active && !pCPlayer->IsLocalPlayer)
								gLagCompensation.NetUpdatePlayer(pCPlayer, Entity);

							if (++CountedPlayers == NumPlayers)
								break;
						}
					}
				}


				if (!LocalPlayer.Entity->GetAlive())
				{
					LocalPlayer.IsShotAimPunch = angZero;
					LocalPlayer.IsShot = false;
					LocalPlayer.IsWaitingToSetIsShotAimPunch = false;
				}
				else
				{
					//Did we get hit by another player?
					if (LocalPlayer.IsWaitingToSetIsShotAimPunch)
					{
						QAngle punch = LocalPlayer.Entity->GetPunch();
						LocalPlayer.IsShotAimPunch = punch;
						LocalPlayer.IsShotAimPunchDelta = punch - LocalPlayer.LastAimPunch;
						LocalPlayer.IsWaitingToSetIsShotAimPunch = false;
					}
				}
			}
			break;
#if 0
		case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
			break;
#endif
		case FRAME_NET_UPDATE_END:
			if (LocalPlayer.Entity)
			{
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
								CustomPlayer* pCPlayer = &AllPlayers[i];
								if (pCPlayer->BaseEntity)
									gLagCompensation.PostNetUpdatePlayer(pCPlayer, pCPlayer->BaseEntity);
							}

							if (++CountedPlayers == NumPlayers)
								break;
						}
					}
				}
			}
			break;
		}
	}
	oFrameStageNotify(ecx, stage);
	if (SetPunches)
	{
		LocalPlayer.Entity->SetPunch(OldAimPunch);
		LocalPlayer.Entity->SetViewPunch(OldViewPunch);
	}
	else if (SetViewAngles)
	{
		LocalPlayer.Entity->SetPunch(OldAimPunch);
		//Interfaces::Engine->SetViewAngles(OldViewAngles);
	}

#if 0
	if (SetEyeAngles)
	{
		LocalPlayer.Entity->SetRenderAngles(OldEyeAngles);
	}
#endif

	if (stage == FRAME_RENDER_START)
	{
		//Update pose parameters and animations, etc
		gLagCompensation.PostRenderStart();
	}
	END_PROFILING(stat);
	VMP_END
}

//bool BoneSetup(IClientEntity * pTarget, int boneMask, matrix3x4_t * boneOut)
//{
//
//	if (!WTF::Force()->g_pPC[pTarget->Index()].l_DataUpdated)
//		return false;
//
//
//	IClientRenderable* pRenderable = pTarget->GetRenderable();
//
//	if (!POINTERCHK(pRenderable)) { return false; }
//
//	studiohdr_t * hdr = *(studiohdr_t**)((DWORD)pTarget + 10556);//0x293C
//
//	if (!hdr)
//		return false;
//
//
//
//
//	typedef bool(__thiscall *pSequencesAvailable) (studiohdr_t * hdrs);
//	static pSequencesAvailable SequencesAvailable = (pSequencesAvailable)(WTF::Force()->hClient + 0x345740);
//
//
//	if (!SequencesAvailable(hdr))
//		return false;
//
//	//9876 boneaccessor
//	//9884 oldWritableBones  m_WritableBones 
//	//9880 oldReadableBones m_ReadableBones 
//
//	//GetBoneArrayForWrite
//	matrix3x4_t* backup_matrix = *(matrix3x4_t**)((DWORD)pRenderable + 9876);//0x2694
//
//	if (!backup_matrix)
//		return false;
//
//
//
//	int oldReadableBones = *(int*)((DWORD)pRenderable + 9880);//0x2694
//
//	int bonesMaskNeedRecalc = boneMask | oldReadableBones;
//
//
//
//	//uninterpolated data
//	Vector origin = WTF::Force()->g_pPC[pTarget->Index()].LastUnIpOrigin;
//	Vector angles = WTF::Force()->g_pPC[pTarget->Index()].LastUnIpAngles;
//
//
//
//	// back up interpolated data.
//	Vector backup_absorigin = pTarget->GetAbsOrigin();
//	Vector backup_absangles = pTarget->GetAbsAngles();
//
//
//	//backup poses
//	float backup_poses[24];
//
//	for (int ix = 0; ix < 24; ix++)
//	{
//		backup_poses[ix] = pTarget->GetFloat(WTF::Force()->g_pNetVars->m_flPoseParameter + (ix * 4));
//
//	}
//
//
//	//backup anim overlays
//	//C_AnimationLayer backup_layers[15];
//
//	/*
//	leave this yet
//	C_BaseAnimatingOverlay * backupoverlay  = (C_BaseAnimatingOverlay *)((DWORD)pTarget + WTF::Force()->g_pNetVars->m_AnimOverlay);//0x2970
//
//	if (backupoverlay)
//	{
//	int size = backupoverlay->layerCount;
//
//	//memcpy(backupoverlay->m_AnimOverlayArray, *(C_BaseAnimatingOverlay**)((DWORD)pTarget + WTF::Force()->g_pNetVars->m_AnimOverlay), (4 * size));
//
//	//for (int ix = 0; ix < size; ix++)
//	//{
//	//backup_layers[ix] = **(C_AnimationLayer**)((DWORD)pTarget + WTF::Force()->g_pNetVars->m_AnimOverlay + (4 * size) );
//	//}
//
//	}
//	*/
//
//
//	// build matrix using un-interpolated server data.
//	matrix3x4_t parentTransform;
//
//	AngleIMatrix(angles, parentTransform);
//	//like in ida, its not part of function
//	parentTransform[0][3] = origin.x;
//	parentTransform[1][3] = origin.y;
//	parentTransform[2][3] = origin.z;
//
//
//	SetAbsOrigin(pTarget, origin);
//	SetAbsAngles(pTarget, angles);
//
//	for (int ix = 0; ix < 24; ix++)
//	{
//		pTarget->SetFloat(WTF::Force()->g_pNetVars->m_flPoseParameter + (ix * 4), WTF::Force()->g_pPC[pTarget->Index()].lt_PoseParameter[ix]);
//	}
//
//
//	*(int*)((DWORD)pRenderable + 224) |= 8;//AddFlag( EFL_SETTING_UP_BONES ); (1 << 3));
//
//	pTarget->UpdateIKLocks(flCurTime);
//
//	DWORD *m_pIk = *(DWORD**)((DWORD)pRenderable + 9820);
//
//
//	/*
//	//dont need i guess
//	if (!m_pIk)
//	{
//
//	debug::log("create m_pIk");
//	//if (*(_DWORD *)(*(_DWORD *)hdr + 284) > 0 && !(*(_BYTE *)(renderable + 100) & 2))
//	{
//	DWORD* v57 = new DWORD[4208]; // (*(int(__stdcall **)(signed int))(*(_DWORD *)g_pMemAlloc + 4))(4208);// m_pIk = new CIKContext;
//
//
//	typedef DWORD*(__thiscall *pCIKContext) (DWORD* thisptr);
//
//	static pCIKContext pCIKContextInit = (pCIKContext)(WTF::Force()->hClient + 0x6367F0);
//
//
//	if (v57)
//	m_pIk = pCIKContextInit(v57);
//	else
//	m_pIk = 0;
//
//	}
//
//	}
//	*/
//
//
//
//
//
//
//
//	if (m_pIk) {
//		ClearTargets(m_pIk);
//		Init(m_pIk, hdr, backup_absangles, backup_absorigin, flCurTime, WTF::Force()->g_pHooks->m_pGlobals->framecount, bonesMaskNeedRecalc);
//
//	}
//
//
//	/*
//	/dont need
//	PVOID CALLER = (PVOID)(WTF::Force()->hClient + 0xABA2B4);
//	DWORD *Networkable = (DWORD*)((DWORD)pRenderable + 4);//GetNetworkable?
//	IPoseDebugger* g_pDebugger = (IPoseDebugger*)(WTF::Force()->hClient + 0xABA2B4);
//	if(g_pDebugger)
//	g_pDebugger->StartBlending(Networkable, (DWORD*)hdr);
//	*/
//
//
//	Vector pos[128];
//	Quaternion q[128];
//	byte computed[256] = { 0 };
//
//
//	pTarget->StandardBlendingRules(hdr, pos, q, WTF::Force()->g_pHooks->m_pGlobals->curtime, bonesMaskNeedRecalc);
//
//
//	*(matrix3x4_t**)((DWORD)pRenderable + 9876) = boneOut;
//
//
//	debug::log("StandardBlendingRules ok");
//
//	if (m_pIk) {
//
//		debug::log("m_pIk ok 2, UpdateIKLocks");
//		pTarget->UpdateIKLocks(flCurTime);
//		debug::log("UpdateIKLocks ok, UpdateTargets");
//		UpdateTargets(m_pIk, pos, q, *(matrix3x4_t**)((DWORD)pRenderable + 9876), computed);
//		debug::log("UpdateTargets ok, CalculateIKLocks");
//
//		pTarget->CalculateIKLocks(flCurTime);
//
//		debug::log("CalculateIKLocks ok, SolveDependencies");
//
//		SolveDependencies(m_pIk, pos, q, *(matrix3x4_t**)((DWORD)pRenderable + 9876), computed);
//
//		debug::log("SolveDependencies ok, BuildTransformations");
//
//
//
//	}
//
//
//
//
//	debug::log("BuildTransformations");
//	PDWORD pEntVMT = *(DWORD**)(pTarget);
//	debug::log("pEntVMT[184] [0x%.8X]", pEntVMT[184]);
//
//	//MessageBoxA(0, "A", 0, 0);
//
//
//	pTarget->BuildTransformations(hdr, pos, q, parentTransform, bonesMaskNeedRecalc, computed);
//	//crash on case where pBone(i)->parent != -1 for most of player models
//
//
//	*(int*)((DWORD)pRenderable + 224) &= ~(8);//AddFlag( EFL_SETTING_UP_BONES ); (1 << 3));
//
//	pTarget->ControlMouth(hdr);
//
//
//
//	*(matrix3x4_t**)((DWORD)pRenderable + 9876) = backup_matrix;
//
//
//	/*
//	//not for hitboxes
//	if (boneMask & 0x400 )
//	{
//	debug::log("flag is 0x400");
//	pTarget->UnknownFixer(hdr, pos, q, *(matrix3x4_t**)((DWORD)pRenderable + 9876), computed, m_pIk);
//
//	}
//	*/
//	/*
//	//not for hitboxes
//	if (!(oldReadableBones & 0x200) && boneMask & 0x200)
//	{
//
//	debug::log("flag is 0x200");
//	typedef void(__thiscall *pAttachmentHelper) (IClientEntity * target, studiohdr_t * hdrs);
//	static pAttachmentHelper AttachmentHelper = (pAttachmentHelper)(WTF::Force()->hClient + 0x1CF6A0);
//
//	AttachmentHelper(pTarget, hdr);
//
//
//	}
//	*/
//
//	// restore original entity data.
//	SetAbsOrigin(pTarget, backup_absorigin);
//	SetAbsAngles(pTarget, backup_absangles);
//
//	for (int ix = 0; ix < 24; ix++)
//	{
//		pTarget->SetFloat(WTF::Force()->g_pNetVars->m_flPoseParameter + (ix * 4), backup_poses[ix]);
//	}
//
//
//	//pTarget->SetAnimLayers(backup_layers);
//
//	return true;
//}
