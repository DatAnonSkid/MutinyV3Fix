#include "GameEventListener.h"
#include "Events.h"
#include "Aimbot.h"
#include "EncryptString.h"
#include "player_lagcompensation.h"
#include "Overlay.h"
#include "HitChance.h"
#include "LocalPlayer.h"
#include "Logging.h"

char *bulletimpactstr = new char[14]{ 24, 15, 22, 22, 31, 14, 37, 19, 23, 10, 27, 25, 14, 0 }; /*bullet_impact*/
char *playerhurtstr = new char[12]{ 10, 22, 27, 3, 31, 8, 37, 18, 15, 8, 14, 0 }; /*player_hurt*/
char *player_deathstr = new char[13]{ 10, 22, 27, 3, 31, 8, 37, 30, 31, 27, 14, 18, 0 }; /*player_death*/

void CreateEventListeners()
{
	CGameEventListener *ImpactListener = new CGameEventListener(bulletimpactstr, GameEvent_BulletImpact, false);
	CGameEventListener *PlayerHurtListener = new CGameEventListener(playerhurtstr, GameEvent_PlayerHurt, false);
	CGameEventListener *PlayerDeath = new CGameEventListener(player_deathstr, GameEvent_PlayerDeath, false);

}

void DecayPlayerIsShot()
{
	if (!LocalPlayer.IsAlive)
	{
		LocalPlayer.IsShot = false;
	} 
	else if (LocalPlayer.IsShot)
	{
		QAngle Punch = LocalPlayer.Entity->GetPunch() * 2.0f;
		if (Punch == QAngle(0.0f, 0.0f, 0.0f))
		{
			LocalPlayer.IsShot = false;
		}
	}
}

char *useridstr = new char[7]{ 15, 9, 31, 8, 19, 30, 0 }; /*userid*/
char *attackerstr = new char[9]{ 27, 14, 14, 27, 25, 17, 31, 8, 0 }; /*attacker*/
char *hitgroupstr = new char[9]{ 18, 19, 14, 29, 8, 21, 15, 10, 0 }; /*hitgroup*/
char *useridstr2 = new char[7]{ 15, 9, 31, 8, 19, 30, 0 }; /*userid*/
char *dmg_healthstr = new char[11]{ 30, 23, 29, 37, 18, 31, 27, 22, 14, 18, 0 }; /*dmg_health*/

int TickHitPlayer = 0;
int TickHitWall = 0;
int OriginalCorrectedBodyYawIndex = 0;
int OriginalCloseFakeIndex = 0;
int OriginalCorrectedIndex = 0;
int OriginalResolveWhenShootingIndex = 0;
int OriginalShotsMissed = 0;
int OriginalCorrectedFakeWalkIndex = 0;
int OriginalCorrectedNotFakeIndex = 0;
int OriginalCorrectedLowerBodyYawBreakerIndex = 0;
int OriginalCorrectedLinearFakeIndex = 0;
int OriginalCorrectedFakeSpinsIndex = 0;
int OriginalCorrectedRandomFakeIndex = 0;
int OriginalCorrectedStaticFakeDynamicIndex = 0;
int OriginalCorrectedAverageLowerBodyYawDeltaIndex = 0;
bool OriginalIsCorrected = 0;
bool OriginalDontStatsThisShot = 0;

void GameEvent_PlayerDeath(IGameEvent* gameEvent)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("GameEvent_PlayerDeath\n");
#endif
	if (DisableAllChk.Checked || !LocalPlayer.Entity || !gameEvent)
		return;

	CBaseEntity* Entity = GetPlayerEntityFromEvent(gameEvent);
	if (!Entity)
		return;

	CustomPlayer *pCPlayer = &AllPlayers[Entity->index];

	pCPlayer->PersistentData.roundStart = pCPlayer->PersistentData.ShotsnotHit;
}

void GameEvent_PlayerHurt(IGameEvent* gameEvent)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("GameEvent_PlayerHurt\n");
#endif
	if (DisableAllChk.Checked || !LocalPlayer.Entity || !gameEvent)
		return;

	CBaseEntity* Entity = GetPlayerEntityFromEvent(gameEvent);
	if (!Entity)
		return;

	CustomPlayer *pCPlayer = &AllPlayers[Entity->index];

	if (Entity == LocalPlayer.Entity)
	{
		//This is used to stop no recoil if player is shot, and future stuff
		LocalPlayer.IsShot = true;
		LocalPlayer.IsWaitingToSetIsShotAimPunch = true;
	}
	else
	{
		if (!pCPlayer->Dormant && (DrawDamageChk.Checked || pCPlayer->PersistentData.ShouldResolve()))
		{
			int LocalUserID = LocalPlayer.Entity->GetUserID();
			int AttackerUserID = GetAttackerUserID(gameEvent);
			int HitgroupThatWasHit = GetHitgroupFromEvent(gameEvent);

			StoredNetvars *currentrecord = pCPlayer->GetCurrentRecord();
			
			pCPlayer->PersistentData.flSimulationTimePlayerWasShot = currentrecord ? currentrecord->simulationtime : Entity->GetSimulationTime();
			pCPlayer->PersistentData.iHitGroupPlayerWasShotOnServer = HitgroupThatWasHit;
			pCPlayer->PersistentData.bAwaitingBloodResolve = true;

			if (AttackerUserID == LocalUserID)
			{
				int DamageGivenToTarget = GetDamageFromEvent(gameEvent);

				if (LastTargetIndex != INVALID_PLAYER)
				{
					CustomPlayer *Target = &AllPlayers[LastTargetIndex];
					if (DamageGivenToTarget >= 1)
						Target->PersistentData.ShotsnotHit--;
				}

				if (DrawDamageChk.Checked)
				{
					if (LastPlayerDamageGiven == Entity)
					{
						iDamageGivenToTarget += DamageGivenToTarget;
						iHitsToTarget++;
					}
					else
					{
						iDamageGivenToTarget = DamageGivenToTarget;
						iHitsToTarget = 1;
					}
					iLastHitgroupHit = HitgroupThatWasHit;
					LastPlayerDamageGiven = Entity;
					flTimeDamageWasGiven = Interfaces::Globals->curtime;
				}

				Personalize *pPersistentData = &pCPlayer->PersistentData;

				//Don't count shots made when player is moving while jumping. Only count shots made if they jump up and down
				bool AirJumping = false;// ((Entity->GetGroundEntity() == nullptr || !(Entity->GetFlags() & FL_ONGROUND)) && Entity->GetVelocity().Length2D() > 1.0f);
				int tickcount = ReadInt((uintptr_t)&Interfaces::Globals->tickcount);
				if (TickHitWall == tickcount)
				{
					pPersistentData->correctedclosefakeindex = OriginalCloseFakeIndex;
					pPersistentData->correctedindex = OriginalCorrectedIndex;
					pPersistentData->isCorrected = OriginalIsCorrected;
					pPersistentData->DontStatsThisShot = OriginalDontStatsThisShot;
					pPersistentData->correctedresolvewhenshootingindex = OriginalResolveWhenShootingIndex;
					pPersistentData->ShotsMissed = OriginalShotsMissed;
					pPersistentData->correctedbodyyawindex = OriginalCorrectedBodyYawIndex;
					pPersistentData->correctedfakewalkindex = OriginalCorrectedFakeWalkIndex;
					pPersistentData->correctednotfakeindex = OriginalCorrectedNotFakeIndex;
					pPersistentData->correctedlbybreakerindex = OriginalCorrectedLowerBodyYawBreakerIndex;
					pPersistentData->correctedavglbyindex = OriginalCorrectedAverageLowerBodyYawDeltaIndex;
				}
				if (tickcount != TickHitPlayer)
				{
					TickHitPlayer = tickcount;
					pPersistentData->ShotsMissed = 0;

					if (FireWhenEnemiesShootChk.Checked && pCPlayer->ResolverFiredAtShootingAngles)
					{
						if (HitgroupThatWasHit != pPersistentData->LastHitGroupShotAt)
						{
							if (++pPersistentData->correctedresolvewhenshootingindex > (MAX_FIRING_WHEN_SHOOTING_MISSES - 1))
								pPersistentData->correctedresolvewhenshootingindex = 0;
						}
						pPersistentData->DontStatsThisShot = false;
					}
					else
					{
						if (!pPersistentData->DontStatsThisShot)
						{
							if (!AirJumping)
							{
								if (HitgroupThatWasHit == pPersistentData->LastHitGroupShotAt)
								{
									//pCPlayer->LastUpdatedNetVars.tickcount = TIME_TO_TICKS(pCPlayer->CurrentNetVars.simulationtime);//ReadInt((uintptr_t)&Interfaces::Globals->tickcount);

									if (pPersistentData->LastResolveModeUsed == ResolveYawModes::NotFaked)
									{
										pPersistentData->correctednotfakeindex = 0;
										pPersistentData->correctedlbybreakerindex = 0;
									}

									pPersistentData->isCorrected = true;
								}
								else //if (LastShotHitPercentage >= 82.0f || AimbotHitPercentageHeadTxt.flValue < 5)
								{
									pPersistentData->isCorrected = false;
									//if (++pPersistentData->correctedbodyyawindex > (MAX_BODY_YAW_MISSES - 1))
									//	pPersistentData->correctedbodyyawindex = 0;

									switch (pPersistentData->LastResolveModeUsed)
									{
									case ResolveYawModes::FakeWalk:
										if (++pPersistentData->correctedfakewalkindex > (MAX_FAKEWALK_MISSES - 1))
											pPersistentData->correctedfakewalkindex = 0;
										break;
									case ResolveYawModes::NoYaw:
										++pPersistentData->correctedlbybreakerindex;
										++pPersistentData->correctednotfakeindex;
										break;
									case ResolveYawModes::LinearFake:
										if (++pPersistentData->correctedlinearindex > (MAX_LINEARFAKE_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
											pPersistentData->correctedlinearindex = 0;
										break;
									case ResolveYawModes::FakeSpins:
										if (++pPersistentData->correctedfakespinindex > (MAX_FAKESPINS_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
											pPersistentData->correctedfakespinindex = 0;
										break;
									case ResolveYawModes::RandomFake:
										if (++pPersistentData->correctedrandomindex > (MAX_RANDOMFAKE_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
											pPersistentData->correctedrandomindex = 0;
										break;
									case ResolveYawModes::StaticFakeDynamic:
										if (++pPersistentData->correctedfakedynamicindex > (MAX_STATICFAKEDYNAMIC_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
											pPersistentData->correctedfakedynamicindex = 0;
										break;
									case ResolveYawModes::AverageLBYDelta:
										if (++pPersistentData->correctedavglbyindex > (MAX_AVGLBYDELTA_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
											pPersistentData->correctedavglbyindex = 0;
										break;
									}

									if (pCPlayer->CloseFakeAngle)
									{
										if (++pPersistentData->correctedclosefakeindex > (MAX_NEAR_BODY_YAW_RESOLVER_MISSES - 1))
											pPersistentData->correctedclosefakeindex = 0;
									}
									else
									{
										if (++pPersistentData->correctedindex > (MAX_RESOLVER_MISSES - 1))
											pPersistentData->correctedindex = 0;
									}
								}
							}
							pPersistentData->DontStatsThisShot = false;
						}
					}
				}
			}
			else
			{
				//Not us that attacked this player
				DecStr(hitgroupstr, 8);
				int iHitGroup = gameEvent->GetInt(hitgroupstr);
				EncStr(hitgroupstr, 8);
				pCPlayer->PersistentData.iHitGroupPlayerWasShotOnServer = iHitGroup;
				pCPlayer->PersistentData.bAwaitingBloodResolve = true;
				pCPlayer->PersistentData.flSimulationTimePlayerWasShot = pCPlayer->BaseEntity->GetSimulationTime();
			}
		}
	}
}

void GameEvent_BulletImpact(IGameEvent* gameEvent)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("GameEvent_BulletImpact\n");
#endif
	if (DisableAllChk.Checked || !LocalPlayer.Entity || !gameEvent)
		return;

	DecStr(useridstr2, 6);
	int iUser = gameEvent->GetInt(useridstr2);
	EncStr(useridstr2, 6);

	if (iUser == LocalPlayer.Entity->GetUserID())
	{
		if (LastTargetIndex != INVALID_PLAYER)
		{
			//int tickcount = ReadFloat((uintptr_t)&Interfaces::Globals->curtime);
			Vector Impact;
			Impact.x = gameEvent->GetFloat("x");
			Impact.y = gameEvent->GetFloat("y");
			Impact.z = gameEvent->GetFloat("z");
			Vector ShotFrom = LocalPlayer.Entity->GetEyePosition();
			//ImpactTime = 0;

			CustomPlayer *pCPlayer = &AllPlayers[LastTargetIndex];

			Personalize *pPersistentData = &pCPlayer->PersistentData;
			if (!pCPlayer->Dormant && pPersistentData->ShouldResolve())
			{
#if 0
				CTraceFilterPlayersOnlyNoWorld filter;
				filter.AllowTeammates = false;
				Ray_t ray;
				trace_t tr;
				ray.Init(ShotFrom, Impact);
				filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
				filter.m_icollisionGroup = COLLISION_GROUP_NONE;
				Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, (ITraceFilter*)&filter, &tr);
				if (tr.m_pEnt && ReadInt((uintptr_t)&tr.m_pEnt->index) == LastTargetIndex)
#else
				//if (LastShotHitPercentage >= 82.0f || AimbotHitPercentageHeadTxt.flValue < 5)
#endif
				{
					pPersistentData->ShotsnotHit++;
					if (pPersistentData->ShotsnotHit > 100)
						pPersistentData->ShotsnotHit = 1;
					CBaseEntity *Entity = pCPlayer->BaseEntity;
					//Don't count shots made when player is moving while jumping. Only count shots made if they jump up and down
					//FIX ME - cause issues in nospread downbelow
					bool AirJumping = false;// ((Entity->GetGroundEntity() == nullptr || !(Entity->GetFlags() & FL_ONGROUND)) && Entity->GetVelocity().Length2D() > 1.0f);
					int tickcount = ReadInt((uintptr_t)&Interfaces::Globals->tickcount);
					if (tickcount != TickHitWall)
					{
						TickHitWall = tickcount;
						OriginalDontStatsThisShot = pPersistentData->DontStatsThisShot;
						OriginalIsCorrected = pPersistentData->isCorrected;
						OriginalCorrectedIndex = pPersistentData->correctedindex;
						OriginalCloseFakeIndex = pPersistentData->correctedclosefakeindex;
						OriginalResolveWhenShootingIndex = pPersistentData->correctedresolvewhenshootingindex;
						OriginalShotsMissed = pPersistentData->ShotsMissed;
						OriginalCorrectedBodyYawIndex = pPersistentData->correctedbodyyawindex;
						OriginalCorrectedFakeWalkIndex = pPersistentData->correctedfakewalkindex;
						OriginalCorrectedNotFakeIndex = pPersistentData->correctednotfakeindex;
						OriginalCorrectedLowerBodyYawBreakerIndex = pPersistentData->correctedlbybreakerindex;
						OriginalCorrectedLinearFakeIndex = pPersistentData->correctedlinearindex;
						OriginalCorrectedFakeSpinsIndex = pPersistentData->correctedfakespinindex;
						OriginalCorrectedRandomFakeIndex = pPersistentData->correctedrandomindex;
						OriginalCorrectedStaticFakeDynamicIndex = pPersistentData->correctedfakedynamicindex;
						OriginalCorrectedAverageLowerBodyYawDeltaIndex = pPersistentData->correctedavglbyindex;
						if (tickcount != TickHitPlayer)
						{
							TickHitWall = tickcount;
							pPersistentData->ShotsMissed++;

							//if (++pPersistentData->correctedbodyyawindex > (MAX_BODY_YAW_MISSES - 1))
							//	pPersistentData->correctedbodyyawindex = 0;

							if (FireWhenEnemiesShootChk.Checked && pCPlayer->ResolverFiredAtShootingAngles)
							{
								pPersistentData->isCorrected = false;
								if (++pPersistentData->correctedresolvewhenshootingindex > (MAX_FIRING_WHEN_SHOOTING_MISSES - 1))
									pPersistentData->correctedresolvewhenshootingindex = 0;
								pPersistentData->DontStatsThisShot = false;
							}
							else
							{
								if (!pPersistentData->DontStatsThisShot)
								{
									if (!AirJumping)
									{
										pPersistentData->isCorrected = false;

										switch (pPersistentData->LastResolveModeUsed)
										{
										case ResolveYawModes::FakeWalk:
											if (++pPersistentData->correctedfakewalkindex > (MAX_FAKEWALK_MISSES - 1))
												pPersistentData->correctedfakewalkindex = 0;
											break;
										case ResolveYawModes::NoYaw:
										case ResolveYawModes::NotFaked:
											++pPersistentData->correctedlbybreakerindex;
											++pPersistentData->correctednotfakeindex;
											break;
										case ResolveYawModes::LinearFake:
											if (++pPersistentData->correctedlinearindex > (MAX_LINEARFAKE_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
												pPersistentData->correctedlinearindex = 0;
											break;
										case ResolveYawModes::FakeSpins:
											if (++pPersistentData->correctedfakespinindex > (MAX_FAKESPINS_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
												pPersistentData->correctedfakespinindex = 0;
											break;
										case ResolveYawModes::RandomFake:
											if (++pPersistentData->correctedrandomindex > (MAX_RANDOMFAKE_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
												pPersistentData->correctedrandomindex = 0;
											break;
										case ResolveYawModes::StaticFakeDynamic:
											if (++pPersistentData->correctedfakedynamicindex > (MAX_STATICFAKEDYNAMIC_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
												pPersistentData->correctedfakedynamicindex = 0;
											break;
										case ResolveYawModes::AverageLBYDelta:
											if (++pPersistentData->correctedavglbyindex > (MAX_AVGLBYDELTA_MISSES - 1 + BruteForceAnglesAfterXMissesTxt.iValue))
												pPersistentData->correctedavglbyindex = 0;
											break;
										}

										if (pCPlayer->CloseFakeAngle)
										{
											if (++pPersistentData->correctedclosefakeindex > (MAX_NEAR_BODY_YAW_RESOLVER_MISSES - 1))
												pPersistentData->correctedclosefakeindex = 0;
										}
										else
										{
											if (++pPersistentData->correctedindex > (MAX_RESOLVER_MISSES - 1))
												pPersistentData->correctedindex = 0;
										}
									}
									pPersistentData->DontStatsThisShot = false;
								}
							}
						}
					}
				}
			}
		}

		if (DrawDamageChk.Checked)
		{
			float curtime = ReadFloat((uintptr_t)&Interfaces::Globals->curtime);
			if (iDamageGivenToTarget <= 0)
			{
				iHitsToTarget = 0;
				iLastHitgroupHit = 0;
				flTimeDamageWasGiven = curtime;
			}
			else if (curtime - flTimeDamageWasGiven >= 1.2f)
			{
				iHitsToTarget = 0;
				iDamageGivenToTarget = 0;
				iLastHitgroupHit = 0;
				LastPlayerDamageGiven = nullptr;
				flTimeDamageWasGiven = curtime;
			}
		}
	}
}