#include "player_lagcompensation.h"
#include "Interfaces.h"
#include "CreateMove.h"
#include "ConVar.h"
#include "utlvectorsimple.h"
#include "INetchannelInfo.h"
#include <math.h>
#include "BaseCombatWeapon.h"
#include "Aimbot.h"
#include "Triggerbot.h"
#include "HitChance.h"
#include "Animation.h"
#include <fstream>
#include "LocalPlayer.h"
#include "ClientSideAnimationList.h"
#include "VMProtectDefs.h"
#include "Intersection.h"
#include "Netchan.h"

extern CMoveHelperServer* pMoveHelperServer;

LagCompensation gLagCompensation;
#define acceleration
#define predictTicks
//Haven't test with test data
//should work tho

//TODO: MOVE ME TO PROPER PLACE
ConVar *sv_unlag;
ConVar *sv_maxunlag;
ConVar *sv_lagflushbonecache;
ConVar *sv_lagpushticks;
ConVar *sv_client_min_interp_ratio;
ConVar *sv_client_max_interp_ratio;
ConVar *sv_maxupdaterate;
ConVar *sv_minupdaterate;
ConVar *cl_updaterate;
ConVar *sv_gravity = nullptr;
ConVar *cl_interp_ratio = nullptr;

char *sv_unlagstr = new char[9]{ 9, 12, 37, 15, 20, 22, 27, 29, 0 }; /*sv_unlag*/
char *sv_maxunlagstr = new char[12]{ 9, 12, 37, 23, 27, 2, 15, 20, 22, 27, 29, 0 }; /*sv_maxunlag*/
char *sv_lagflushbonecachestr = new char[21]{ 9, 12, 37, 22, 27, 29, 28, 22, 15, 9, 18, 24, 21, 20, 31, 25, 27, 25, 18, 31, 0 }; /*sv_lagflushbonecache*/
char *sv_lagpushticksstr = new char[16]{ 9, 12, 37, 22, 27, 29, 10, 15, 9, 18, 14, 19, 25, 17, 9, 0 }; /*sv_lagpushticks*/
char *sv_client_min_interp_ratio_str = new char[27]{ 9, 12, 37, 25, 22, 19, 31, 20, 14, 37, 23, 19, 20, 37, 19, 20, 14, 31, 8, 10, 37, 8, 27, 14, 19, 21, 0 }; /*sv_client_min_interp_ratio*/
char *sv_client_max_interp_ratio_str = new char[27]{ 9, 12, 37, 25, 22, 19, 31, 20, 14, 37, 23, 27, 2, 37, 19, 20, 14, 31, 8, 10, 37, 8, 27, 14, 19, 21, 0 }; /*sv_client_max_interp_ratio*/
char *sv_maxupdateratestr = new char[17]{ 9, 12, 37, 23, 27, 2, 15, 10, 30, 27, 14, 31, 8, 27, 14, 31, 0 }; /*sv_maxupdaterate*/
char *sv_minupdateratestr = new char[17]{ 9, 12, 37, 23, 19, 20, 15, 10, 30, 27, 14, 31, 8, 27, 14, 31, 0 }; /*sv_minupdaterate*/
char *cl_updateratestr = new char[14]{ 25, 22, 37, 15, 10, 30, 27, 14, 31, 8, 27, 14, 31, 0 }; /*cl_updaterate*/
char *cl_interp_ratiostr = new char[16]{ 25, 22, 37, 19, 20, 14, 31, 8, 10, 37, 8, 27, 14, 19, 21, 0 }; /*cl_interp_ratio*/
char *sv_gravitystr = new char[11]{ 9, 12, 37, 29, 8, 27, 12, 19, 14, 3, 0 }; /*sv_gravity*/

																			  //Gets various convars used for lag compensation
bool LagCompensation::GetLagCompensationConVars()
{
	if (LagCompensationConvarsReceived)
		return true;

	DecStr(sv_unlagstr, 8);
	DecStr(sv_maxunlagstr, 11);
	DecStr(sv_lagflushbonecachestr, 20);
	DecStr(sv_lagpushticksstr, 15);
	DecStr(sv_client_min_interp_ratio_str, 26);
	DecStr(sv_client_max_interp_ratio_str, 26);
	DecStr(sv_maxupdateratestr, 16);
	DecStr(sv_minupdateratestr, 16);
	DecStr(cl_updateratestr, 13);
	DecStr(cl_interp_ratiostr, 15);
	DecStr(sv_gravitystr, 10);
	sv_unlag = Interfaces::Cvar->FindVar(sv_unlagstr);
	sv_maxunlag = Interfaces::Cvar->FindVar(sv_maxunlagstr);
	sv_lagflushbonecache = Interfaces::Cvar->FindVar(sv_lagflushbonecachestr);
	sv_lagpushticks = Interfaces::Cvar->FindVar(sv_lagpushticksstr);
	sv_client_min_interp_ratio = Interfaces::Cvar->FindVar(sv_client_min_interp_ratio_str);
	sv_client_max_interp_ratio = Interfaces::Cvar->FindVar(sv_client_max_interp_ratio_str);
	sv_maxupdaterate = Interfaces::Cvar->FindVar(sv_maxupdateratestr);
	sv_minupdaterate = Interfaces::Cvar->FindVar(sv_minupdateratestr);
	cl_updaterate = Interfaces::Cvar->FindVar(cl_updateratestr);
	cl_interp_ratio = Interfaces::Cvar->FindVar(cl_interp_ratiostr);
	sv_gravity = Interfaces::Cvar->FindVar(sv_gravitystr);
	EncStr(sv_unlagstr, 8);
	EncStr(sv_maxunlagstr, 11);
	EncStr(sv_lagflushbonecachestr, 20);
	EncStr(sv_lagpushticksstr, 15);
	EncStr(sv_client_min_interp_ratio_str, 26);
	EncStr(sv_client_max_interp_ratio_str, 26);
	EncStr(sv_maxupdateratestr, 16);
	EncStr(sv_minupdateratestr, 16);
	EncStr(cl_updateratestr, 13);
	EncStr(cl_interp_ratiostr, 15);
	EncStr(sv_gravitystr, 10);
	if (cl_interp_ratio && sv_gravity)
	{
		LagCompensationConvarsReceived = true;
		return true;
	}
	return false;
}

//Gets the estimated server tickcount
int LagCompensation::GetEstimatedServerTickCount(float latency)
{
	return TIME_TO_TICKS(latency) + 1 + ReadInt((uintptr_t)&Interfaces::Globals->tickcount);
}

//Gets the estimated server curtime
float LagCompensation::GetEstimatedServerTime()
{
	return LocalPlayer.Entity->GetTickBase() * ReadFloat((uintptr_t)&Interfaces::Globals->interval_per_tick);
}

//Get latency for local player
float LagCompensation::GetNetworkLatency(int DIRECTION)
{
	// Get true latency
	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	if (nci)
	{
		//float IncomingLatency = nci->GetAvgLatency(FLOW_INCOMING);
		float OutgoingLatency = nci->GetAvgLatency(DIRECTION);
		return OutgoingLatency;// + IncomingLatency;
	}
	return 0.0f;
}
float LagCompensation::GetTrueNetworkLatency(int DIRECTION)
{
	// Get true latency
	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	if (nci)
	{
		//float IncomingLatency = nci->GetAvgLatency(FLOW_INCOMING);
		float OutgoingLatency = nci->GetLatency(DIRECTION);
		return OutgoingLatency;// + IncomingLatency;
	}
	return 0.0f;
}

//See if a tick is within limits to be accepted by the server for hit registration
bool LagCompensation::bTickIsValid(int tick)
{
#ifdef PRINT_FUNCTION_NAMES
	printf("bTickIsValid\n");
#endif
#define NEW_VERSION
#ifdef NEW_VERSION
	if (tick > 0)
	{

		float outlatency;
		float inlatency;
		INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
		if (nci)
		{
			inlatency = nci->GetLatency(FLOW_INCOMING);
			outlatency = nci->GetLatency(FLOW_OUTGOING);
		}
		else
		{
			inlatency = outlatency = 0.0f;
		}

		float totaloutlatency = outlatency;
		//if (BacktrackExploitChk.Checked)
		//	totaloutlatency += FAKE_LATENCY_AMOUNT;

		//if (BacktrackExploitChk.Checked)
		//	inlatency -= FAKE_LATENCY_AMOUNT;



		//float servertime = TICKS_TO_TIME(GetEstimatedServerTickCount(outlatency + inlatency) - 3);  //GetEstimatedServerTime()
		float servertime = GetEstimatedServerTime(); //this provides inaccurate values

													 //float servertime = TICKS_TO_TIME(*(int*)(*(DWORD*)pClientState + dwServerTickCount));



#if 0
		IGlobalVarsBase** gppGlobals = nullptr;
		char* gpGlobalsServerSig = "8B  15  ??  ??  ??  ??  66  0F  6E  50  08  F3  0F  10  62  10";
		gppGlobals = (IGlobalVarsBase**)FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)gpGlobalsServerSig, strlen(gpGlobalsServerSig));
		if (!gppGlobals)
			DebugBreak();
		gppGlobals = (IGlobalVarsBase**)*(DWORD*)((DWORD)gppGlobals + 2);
		IGlobalVarsBase* gpGlobals = (IGlobalVarsBase*)*gppGlobals;
		float servertimereal = gpGlobals->curtime;

#endif
		float correct = clamp(outlatency + GetLerpTime(), 0.0f, 1.0f);
		float flTargetTime = TICKS_TO_TIME(tick);
		float deltaTime = correct - (servertime - flTargetTime);

		float flMaxDelta = 0.2f;
		if (BacktrackExploitChk.Checked)
			flMaxDelta += FAKE_LATENCY_AMOUNT - Interfaces::Globals->interval_per_tick;
		return (fabsf(deltaTime) <= flMaxDelta);
	}

	return false;


	//return (ReadInt((uintptr_t)&Interfaces::Globals->tickcount) - tick <= TIME_TO_TICKS(0.2f));
#else
#define ALIENSWARM_VERSION
#ifdef ALIENSWARM_VERSION
	float latency = GetNetworkLatency(FLOW_OUTGOING);
#define SV_MAXUNLAG 1.0f
	float m_flLerpTime = cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat();
	float correct = clamp(latency + m_flLerpTime, 0.0f, SV_MAXUNLAG);
	float flTargetTime = TICKS_TO_TIME(tick) - m_flLerpTime;
	float deltaTime = correct - (GetEstimatedServerTime() - flTargetTime);
	////^oRIGINAL

	//float latency2 = GetNetworkLatency(FLOW_INCOMING) + GetNetworkLatency(FLOW_OUTGOING);
	//float TimeUntilNextFrame = 0;// Interfaces::Globals->absoluteframetime * 0.5f;
	//float TimeItWillBeWhenPacketReachesServer = TICKS_TO_TIME(CurrentUserCmd.cmd->tick_count + TIME_TO_TICKS(latency2 + TimeUntilNextFrame) + 1);
	////Time we reach the server - time the player was simulated on the server , clamped to sv_maxunlag (1.0 seconds)
	//float deltaTime2 = fminf(TimeItWillBeWhenPacketReachesServer - TICKS_TO_TIME(tick), 1.0f);
	////^ fake lag fix


	//float tickInterval = 1.0 / Interfaces::Globals->interval_per_tick;

	//float deltaTime3 = ((floorf(((GetNetworkLatency(FLOW_INCOMING) + GetNetworkLatency(FLOW_OUTGOING)) * tickInterval) + 0.5)
	//	+ CurrentUserCmd.cmd->tick_count
	//	+ 1)
	//	* Interfaces::Globals->interval_per_tick)
	//	- TICKS_TO_TIME(tick);

	//if (deltaTime3 > 1.0f)
	//	deltaTime3 = 1.0f;

	////^aimware
	//AllocateConsole();
	//std::cout << deltaTime << " " << deltaTime2 << " " << deltaTime3 << std::endl;
	return fabsf(deltaTime) <= 0.2f;
#else
	//SDK 2013 VERSION
	float latency = GetNetworkLatency();
	const float SV_MAXUNLAG = 1.0f;
	float m_flLerpTime = cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat();
	int lerpTicks = TIME_TO_TICKS(m_flLerpTime);
	float correct = clamp(latency + TICKS_TO_TIME(lerpTicks), 0.0f, SV_MAXUNLAG);
	int targettick = tick - lerpTicks;
	float deltaTime = correct - TICKS_TO_TIME(GetEstimatedServerTickCount(latency) - targettick);
	return fabsf(deltaTime) <= 0.2f;
#endif
#endif
}

float LagCompensation::GetServerDeltaTimeForTick(int tick, bool bTakeFakeLatencyIntoAccount)
{
#ifdef PRINT_FUNCTION_NAMES
	printf("GetServerDeltaTimeForTick\n");
#endif
#define NEW_VERSION
#ifdef NEW_VERSION
	if (tick > 0)
	{

		float outlatency;
		float inlatency;
		INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
		if (nci)
		{
			inlatency = nci->GetLatency(FLOW_INCOMING);
			outlatency = nci->GetLatency(FLOW_OUTGOING);
		}
		else
		{
			inlatency = outlatency = 0.0f;
		}

		float totaloutlatency = outlatency;
		if (BacktrackExploitChk.Checked && bTakeFakeLatencyIntoAccount)
			totaloutlatency += EST_FAKE_LATENCY_ON_SERVER;

		//int servertickcount = TIME_TO_TICKS(latency) + 1 + Interfaces::Globals->tickcount;

		float servertime = TICKS_TO_TIME(GetEstimatedServerTickCount(outlatency + inlatency) + 1);  //GetEstimatedServerTime()

		float correct = clamp(totaloutlatency + GetLerpTime(), 0.0f, 1.0f);
		float flTargetTime = TICKS_TO_TIME(tick);
		float deltaTime = correct - (servertime - flTargetTime);
		return fabsf(deltaTime);
	}

	return 0.0f;


	//return (ReadInt((uintptr_t)&Interfaces::Globals->tickcount) - tick <= TIME_TO_TICKS(0.2f));
#else
#define ALIENSWARM_VERSION
#ifdef ALIENSWARM_VERSION
	float latency = GetNetworkLatency(FLOW_OUTGOING);
#define SV_MAXUNLAG 1.0f
	float m_flLerpTime = cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat();
	float correct = clamp(latency + m_flLerpTime, 0.0f, SV_MAXUNLAG);
	float flTargetTime = TICKS_TO_TIME(tick) - m_flLerpTime;
	float deltaTime = correct - (GetEstimatedServerTime() - flTargetTime);
	////^oRIGINAL

	//float latency2 = GetNetworkLatency(FLOW_INCOMING) + GetNetworkLatency(FLOW_OUTGOING);
	//float TimeUntilNextFrame = 0;// Interfaces::Globals->absoluteframetime * 0.5f;
	//float TimeItWillBeWhenPacketReachesServer = TICKS_TO_TIME(CurrentUserCmd.cmd->tick_count + TIME_TO_TICKS(latency2 + TimeUntilNextFrame) + 1);
	////Time we reach the server - time the player was simulated on the server , clamped to sv_maxunlag (1.0 seconds)
	//float deltaTime2 = fminf(TimeItWillBeWhenPacketReachesServer - TICKS_TO_TIME(tick), 1.0f);
	////^ fake lag fix


	//float tickInterval = 1.0 / Interfaces::Globals->interval_per_tick;

	//float deltaTime3 = ((floorf(((GetNetworkLatency(FLOW_INCOMING) + GetNetworkLatency(FLOW_OUTGOING)) * tickInterval) + 0.5)
	//	+ CurrentUserCmd.cmd->tick_count
	//	+ 1)
	//	* Interfaces::Globals->interval_per_tick)
	//	- TICKS_TO_TIME(tick);

	//if (deltaTime3 > 1.0f)
	//	deltaTime3 = 1.0f;

	////^aimware
	//AllocateConsole();
	//std::cout << deltaTime << " " << deltaTime2 << " " << deltaTime3 << std::endl;
	return fabsf(deltaTime) <= 0.2f;
#else
	//SDK 2013 VERSION
	float latency = GetNetworkLatency();
	const float SV_MAXUNLAG = 1.0f;
	float m_flLerpTime = cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat();
	int lerpTicks = TIME_TO_TICKS(m_flLerpTime);
	float correct = clamp(latency + TICKS_TO_TIME(lerpTicks), 0.0f, SV_MAXUNLAG);
	int targettick = tick - lerpTicks;
	float deltaTime = correct - TICKS_TO_TIME(GetEstimatedServerTickCount(latency) - targettick);
	return fabsf(deltaTime) <= 0.2f;
#endif
#endif
}

//Simulates player movement for x amount of ticks
void LagCompensation::EnginePredictPlayer(CBaseEntity* Entity, int ticks, bool CalledFromCreateMove, bool WasJumping, bool Ducking, bool ForwardVelocity, CustomPlayer*const pCPlayer)
{
	if (ForwardVelocity)
	{
		QAngle VelocityAngles;
		VectorAngles(Entity->GetVelocity(), VelocityAngles);
		VelocityAngles.Clamp();
		Entity->SetEyeAngles(VelocityAngles);
	}

	for (int i = 0; i < ticks; i++)
	{
		QAngle EyeAngles = Entity->GetEyeAngles();
		EyeAngles.Clamp();
		QAngle VelocityAngles;
		VectorAngles(Entity->GetVelocity(), VelocityAngles);
		float yawdelta = ClampYr(EyeAngles.y - VelocityAngles.y);
		CUserCmd newcmd;
		CMoveData newdata;
		memset(&newdata, 0, sizeof(CMoveData));
		memset(&newcmd, 0, sizeof(CUserCmd));
		newcmd.viewangles = Entity->GetEyeAngles();
		newcmd.viewangles.Clamp();
		newcmd.tick_count = CurrentUserCmd.cmd->tick_count;

		if (Ducking)
			newcmd.buttons = IN_DUCK;

		if ((WasJumping && Entity->GetFlags() & FL_ONGROUND) || Entity->IsInDuckJump())
			newcmd.buttons |= IN_JUMP;

		if (*(DWORD*)((DWORD)Entity + 0x389D))
			newcmd.buttons |= IN_SPEED;

		newcmd.command_number = CurrentUserCmd.cmd->command_number;
		newcmd.hasbeenpredicted = false;

		if (Entity->GetVelocity().Length() != 0.0f)
		{
			if (yawdelta < 0.0f)
				yawdelta += 360.0f;
			float yawdeltaabs = fabsf(yawdelta);

			if (yawdelta > 359.5f || yawdelta < 0.5f)
			{
				newcmd.buttons |= IN_FORWARD;
				newcmd.forwardmove = 450.0f;
				newcmd.sidemove = 0.0f;
			}
			else if (yawdelta > 179.5f && yawdelta < 180.5f)
			{
				newcmd.buttons |= IN_BACK;
				newcmd.forwardmove = -450.0f;
				newcmd.sidemove = 0.0f;
			}
			else if (yawdelta > 89.5f && yawdelta < 90.5f)
			{
				newcmd.buttons |= IN_MOVERIGHT;
				newcmd.forwardmove = 0.0f;
				newcmd.sidemove = 450.0f;
			}
			else if (yawdelta > 269.5f && yawdelta < 270.5f)
			{
				newcmd.buttons |= IN_MOVELEFT;
				newcmd.forwardmove = 0.0f;
				newcmd.sidemove = -450.0f;
			}
			else if (yawdelta > 0.0f && yawdelta < 90.0f)
			{
				newcmd.buttons |= IN_FORWARD;
				newcmd.buttons |= IN_MOVERIGHT;
				newcmd.forwardmove = 450.0f;
				newcmd.sidemove = 450.0f;
			}
			else if (yawdelta > 90.0f && yawdelta < 180.0f)
			{
				newcmd.buttons |= IN_BACK;
				newcmd.buttons |= IN_MOVERIGHT;
				newcmd.forwardmove = -450.0f;
				newcmd.sidemove = 450.0f;
			}
			else if (yawdelta > 180.0f && yawdelta < 270.0f)
			{
				newcmd.buttons |= IN_BACK;
				newcmd.buttons |= IN_MOVELEFT;
				newcmd.forwardmove = -450.0f;
				newcmd.sidemove = -450.0f;
			}
			else
			{
				//yawdelta > 270.0f && yawdelta > 0.0f
				newcmd.buttons |= IN_FORWARD;
				newcmd.buttons |= IN_MOVELEFT;
				newcmd.forwardmove = 450.0f;
				newcmd.sidemove = -450.0f;
			}
		}
		pMoveHelperServer = (CMoveHelperServer*)ReadInt(ReadInt(pMoveHelperServerPP));
		pMoveHelperServer->SetHost((CBasePlayer*)Entity);
		DWORD pOriginalPlayer = *(DWORD*)m_pPredictionPlayer;
		*(DWORD*)m_pPredictionPlayer = (DWORD)Entity;
		int flags = Entity->GetFlags();
		UINT rand = MD5_PseudoRandom(CurrentUserCmd.cmd->command_number) & 0x7fffffff;
		UINT originalrandomseed = *(UINT*)m_pPredictionRandomSeed;
		*(UINT*)m_pPredictionRandomSeed = rand;
		float frametime = Interfaces::Globals->frametime;
		float curtime = Interfaces::Globals->curtime;
		float tickinterval = Interfaces::Globals->interval_per_tick;
		Interfaces::Globals->frametime = tickinterval;
		int tickbase = Entity->GetTickBase();
		Interfaces::Globals->curtime = tickbase * tickinterval;
		CUserCmd *currentcommand = GetCurrentCommand(Entity);
		SetCurrentCommand(Entity, &newcmd);

		//Interfaces::GameMovement->StartTrackPredictionErrors((CBasePlayer*)Entity);

		Interfaces::Prediction->SetupMove((C_BasePlayer*)Entity, &newcmd, pMoveHelperServer, &newdata);
		Interfaces::GameMovement->ProcessMovement((CBasePlayer*)Entity, &newdata);
		//Interfaces::GameMovement->FullWalkMove();
		//Interfaces::Prediction->RunCommand((C_BasePlayer*)LocalPlayer.Entity, &newcmd, pMoveHelperServer);
		Interfaces::Prediction->FinishMove((C_BasePlayer*)Entity, &newcmd, &newdata);

		//Interfaces::GameMovement->FinishTrackPredictionErrors((CBasePlayer*)Entity);
		SetCurrentCommand(Entity, currentcommand);
		Interfaces::Globals->curtime = curtime;
		Interfaces::Globals->frametime = frametime;
		Entity->SetFlags(flags);
		*(DWORD*)m_pPredictionPlayer = pOriginalPlayer;
		pMoveHelperServer->SetHost(!CalledFromCreateMove ? nullptr : EnginePredictChk.Checked ? LocalPlayer.Entity : nullptr);
		*(UINT*)m_pPredictionRandomSeed = originalrandomseed;
	}
}

//Aimware Simulate player movement for fake lag. Don't allow players to be teleported into walls
void LagCompensation::SimulateMovement(CSimulationData& data, bool in_air) {
	if (!(data.m_nFlags & FL_ONGROUND))
		data.velocity.z -= (Interfaces::Globals->interval_per_tick * sv_gravity->GetFloat());
	else if (in_air)
		data.velocity.z = sqrt(91200.f);


	Vector mins = data.m_pEntity->GetMins();
	Vector maxs = data.m_pEntity->GetMaxs();

	Vector src = data.networkorigin;
	Vector end = src + (data.velocity * Interfaces::Globals->interval_per_tick);

	Ray_t ray;
	ray.Init(src, end, mins, maxs);

	trace_t trace;
	CTraceFilter filter;
	filter.m_icollisionGroup = COLLISION_GROUP_NONE;
	filter.pSkip = (IHandleEntity*)data.m_pEntity;
	Interfaces::EngineTrace->TraceRay(ray, CONTENTS_SOLID, &filter, &trace);

	if (trace.fraction != 1.f)
	{
		for (int i = 0; i < 2; ++i)
		{
			data.velocity -= trace.plane.normal * data.velocity.Dot(trace.plane.normal);

			float dot = data.velocity.Dot(trace.plane.normal);
			if (dot < 0.f)
			{
				data.velocity.x -= dot * trace.plane.normal.x;
				data.velocity.y -= dot * trace.plane.normal.y;
				data.velocity.z -= dot * trace.plane.normal.z;
			}

			end = trace.endpos + (data.velocity * (Interfaces::Globals->interval_per_tick * (1.f - trace.fraction)));
			ray.Init(trace.endpos, end, mins, maxs);
			Interfaces::EngineTrace->TraceRay(ray, CONTENTS_SOLID, &filter, &trace);

			if (trace.fraction == 1.f)
				break;
		}
	}

	data.networkorigin = trace.endpos;
	end = trace.endpos;
	end.z -= 2.f;

	ray.Init(data.networkorigin, end, mins, maxs);
	Interfaces::EngineTrace->TraceRay(ray, CONTENTS_SOLID, &filter, &trace);

	data.m_nFlags &= ~FL_ONGROUND;

	if (trace.fraction != 1.f && trace.plane.normal.z > 0.7f)
		data.m_nFlags |= FL_ONGROUND;
}

//MOVE ME
#if 0
inline float NormalizeFloat(float angle) {
	auto revolutions = angle / 360.f;
	if (angle > 180.f || angle < -180.f) {
		revolutions = round(abs(revolutions));

		if (angle < 0.f)
			angle = (angle + 360.f * revolutions);

		else
			angle = (angle - 360.f * revolutions);

		return angle;
	}

	return angle;
}
#endif

//Please make no attempt that I don't know csgo anywhere near as well as many people
//Nor do I understand how you are setting and obtaining values so this is probably wrong
//However, working off the real physics of life and maths and logic it should very 
//much work correctly

//So this is very simple, calculate new velocity for every chocked tick from static acceleration
//Add that updated velocity to netowrked origin for every chocked tick
//Will work for any case where linear acceleration is used, so that is always on the Z
//X and Y it will be more common then linear velocity

//Known issues, if player is falling and land on the ground within their chooked ticks it will fuck up
//Player may be extrapolated into walls and into the ground
//Doesn't take into account your ping
#include <iostream>

void LagCompensation::JakesSimpleMathLagComp(CustomPlayer*const pCPlayer, int ticksChoked, StoredNetvars* CurrentTick, StoredNetvars *PreviousTick)
{
	//accuracy check

#ifdef _DEBUG
	AllocateConsole();
#endif

	//I'm assuming that this is only being done when lag comp has been broken and extrapolation is actually needed
	//Probably bad assumption
	//velocity > (64.0f * 64.0f)

	//How much it accelerates per tick, possibly wrong logic. not sure
	//Vector Acceleration = ((PreviousTick->velocity - CurrentTick->velocity) / ticksChoked) / Interfaces::Globals->interval_per_tick;

	//If entity is in air apply acceleration due to gravity
	//if (!CurrentTick->flags & FL_ONGROUND)
	//	Acceleration.z = -sv_gravity->GetFloat();

	//Acceleration is now units/tick
	Vector Velocity = CurrentTick->velocity;
	Vector CurrentPos = CurrentTick->networkorigin;

	//Accuracy checks

#ifdef _DEBUG
	if (pCPlayer->pred_LastPos == vecZero)
	{
		pCPlayer->pred_LastPos = CurrentTick->networkorigin;
		pCPlayer->pred_HowClose = CurrentTick->networkorigin;
	}

	//if (ticksChoked <= 1)
	//{
	//	static Vector Average = Vector(0, 0, 0);
	//	howClose.x = abs(CurrentTick->networkorigin.x - lastPos.x);
	//	howClose.y = abs(CurrentTick->networkorigin.y - lastPos.y);
	//	howClose.z = abs(CurrentTick->networkorigin.z - lastPos.z);

	//	Average += howClose;
	//	static int times = 0;
	//	times++;
	//	std::cout << "apart: " << howClose.x << " " << howClose.y << " " << howClose.z << std::endl;

	//	std::cout << "Aware Avg: " << Average.x / times << " " << Average.y / times << " " << Average.z / times << std::endl;

	//}
#endif

	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	//static int counter = 1;
	//double additionvalue = abs(sin(counter / 30))+1;
	//counter++;
	CBaseEntity *Entity = pCPlayer->BaseEntity;
	//std::cout << additionvalue << std::endl;
	CSimulationData data;
	data.networkorigin = CurrentPos;
	data.velocity = Velocity;
	data.m_nFlags = CurrentTick->flags;
	data.m_pEntity = Entity;

#if 0
	for (int i = 0; i < ticksChoked; i++)
	{
		//Updates the players velocity for acceleration
		if (i > 0)//Don't update acceleration on first tick as it will be over predicting
		{
			//	data.velocity.x += Acceleration.x;
			//	data.velocity.y += Acceleration.y;
		}
		//Update player position 
		bool InAir = !(CurrentTick->flags & FL_ONGROUND);
		SimulateMovement(data, InAir);
	}
#else
	float latency = nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING);

	//Add 1 because CreateMove is 1 tick behind
	//FIXME: need to take into account psilent/fakelag packets!!!!!!!!!!! For now, just assume we choked ticksChoked is the amount of ticks the firing packet will be held back
	float TimeItWillBeWhenPacketReachesServer = TICKS_TO_TIME(Interfaces::Globals->tickcount + TIME_TO_TICKS(latency) + 1);

	//Time we reach the server - time the player was simulated on the server , clamped to sv_maxunlag (1.0 seconds)
	float deltaTime = fminf(TimeItWillBeWhenPacketReachesServer - CurrentTick->simulationtime, 1.0f);

	int deltaTicks = TIME_TO_TICKS(deltaTime);

	//float simulationTimeDelta = CurrentTick->simulationtime - PreviousTick->simulationtime;
	//int simulationTicks = clamp(TIME_TO_TICKS(simulationTimeDelta), 1, 15);

	//How many choked ticks to simulate.
	int ticks = CurrentTick->tickschoked;
#ifdef predictTicks
	//int CurrentChockedTics = TIME_TO_TICKS(abs(CurrentTick->simulationtime-PreviousTick->simulationtime));
	//if (CurrentChockedTics > 15)
	//	CurrentChockedTics = 15;
	// Number of ticks chocked since last tick was sent
	//not sure how to get or best way to get, I could make a counter but 
	//Think you store it somewhere
	//ticks = predictedChokedTicks(pCPlayer, CurrentChockedTics);
	//if (ticks > 1000)
	//	ticks = CurrentTick->tickschoked;
#endif
	int ChokedTicks = clamp(ticks + 1, 1, 15);

	int v20 = deltaTicks - ChokedTicks;
#ifdef acceleration
	Vector vecAcceleration = ((CurrentTick->velocity - PreviousTick->velocity) / TIME_TO_TICKS(CurrentTick->simulationtime - PreviousTick->simulationtime)) / Interfaces::Globals->interval_per_tick;
#endif
	//int predicted = 0;
	while (v20 >= 0)
	{
		for (int i = 0; i < ChokedTicks; i++)
		{
			//predicted++;
			//pCPlayer->LagCompensatedThisTick = true;
			bool InAir = !(CurrentTick->flags & FL_ONGROUND);
			SimulateMovement(data, InAir);
			Entity->SetOrigin(data.networkorigin);
			Entity->InvalidatePhysicsRecursive(POSITION_CHANGED);
			Entity->SetFlags(data.m_nFlags);

			C_CSGOPlayerAnimState *animstate = Entity->GetPlayerAnimState();
			if (animstate->m_iLastClientSideAnimationUpdateFramecount == Interfaces::Globals->framecount)
				animstate->m_iLastClientSideAnimationUpdateFramecount--;

			if (animstate->m_flLastClientSideAnimationUpdateTime == Interfaces::Globals->curtime)
				animstate->m_flLastClientSideAnimationUpdateTime -= Interfaces::Globals->interval_per_tick;
			Entity->UpdateClientSideAnimation();
#ifdef acceleration
			//data.velocity.x += vecAcceleration.x * Interfaces::Globals->interval_per_tick;
			//data.velocity.y += vecAcceleration.y * Interfaces::Globals->interval_per_tick;
#endif
		}
		v20 -= ChokedTicks;
	}
	//	AllocateConsole();
	//std::cout << predicted << std::endl;
#endif

	//Now set updated position
	//pCPlayer->pred_LastPos = CurrentPos;

#ifdef _DEBUG
	//Entity->DrawHitboxes(ColorRGBA(0, 0, 255, 255), 0.1f);
#endif

	if (Entity->GetOrigin() != data.networkorigin && data.networkorigin != vecZero)
	{
		Entity->SetOrigin(data.networkorigin);
		Entity->InvalidatePhysicsRecursive(POSITION_CHANGED);
		Entity->SetFlags(data.m_nFlags);
		pCPlayer->pBacktrackedTick->bCachedBones = false;
		Entity->InvalidateBoneCache();
	}
	//if (DrawResolveModeChk.Checked)
	//	Entity->DrawHitboxes(ColorRGBA(0, 0, 255, 100), 0.1f);

#ifdef _DEBUG
	//Entity->DrawHitboxes(ColorRGBA(0, 255, 0, 255), 0.1f);
#endif
};

void LagCompensation::JakesSimpleMathLagCompZ(CustomPlayer*const pCPlayer, int ticksChoked, StoredNetvars *CurrentTick, StoredNetvars* PreviousTick)
{
#ifdef _DEBUG
	AllocateConsole();
#endif

	auto Records = &pCPlayer->PersistentData.m_PlayerRecords;

	Vector Velocity = CurrentTick->velocity;
	Vector CurrentPos = CurrentTick->networkorigin;

	//Accuracy checks
	if (pCPlayer->pred_LastPos == vecZero)
	{
		pCPlayer->pred_LastPos = CurrentTick->networkorigin;
		pCPlayer->pred_LastConfirmedValue = CurrentPos;
		pCPlayer->pred_LastPredictedVelocity = vecZero;
	}

#ifdef _DEBUG
	if (ticksChoked <= 1)
	{
		int difference = (CurrentPos.z - pCPlayer->pred_LastPos.z);
		std::cout << difference << " " << CurrentPos.z - pCPlayer->pred_LastConfirmedValue.z << " " << pCPlayer->pred_LastPredictedVelocity.z << std::endl;
	}
#endif


	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();
	float ping = nci->GetAvgLatency(FLOW_OUTGOING) + nci->GetAvgLatency(FLOW_INCOMING);
	ticksChoked += TIME_TO_TICKS(ping);
	CBaseEntity *Entity = pCPlayer->BaseEntity;
	CSimulationData data;
	data.networkorigin = CurrentPos;
	data.velocity = Velocity;
	data.m_nFlags = CurrentTick->flags;
	data.m_pEntity = Entity;
	for (int i = 0; i < ticksChoked; i++)
	{
		if (i > 0)
		{
			Velocity.z -= sv_gravity->GetFloat() * Interfaces::Globals->interval_per_tick;
		}
		//float maxVel = 3500;
		//if (Velocity.z < -maxVel)
		//	Velocity.z = -maxVel;
		//if (Velocity.z > maxVel)
		//	Velocity.z = maxVel;
		CurrentPos.z += Velocity.z * Interfaces::Globals->interval_per_tick;
	}
	pCPlayer->pred_LastPredictedVelocity.z = Velocity.z * Interfaces::Globals->interval_per_tick;
	CM_RestoreNetvars(CurrentTick, Entity, &CurrentPos, &CurrentTick->absangles);
	pCPlayer->pred_LastPos = CurrentPos;
};

void LagCompensation::PredictPlayerPosition(CustomPlayer*const pCPlayer, StoredNetvars* CurrentTick, StoredNetvars* PreviousTick)
{
#define DYLANS_LAG_COMPENSATION
#ifdef DYLANS_LAG_COMPENSATION
	//INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();

	//int TicksSinceUpdate = pCPlayer->TicksChoked;//(Interfaces::Globals->tickcount - CurrentTick->TickReceivedNetUpdate); //TIME_TO_TICKS(Interfaces::Globals->curtime - LocalPlayer.Entity->GetSimulationTime()) + TIME_TO_TICKS(GetLerpTime());
	JakesSimpleMathLagComp(pCPlayer, 1, CurrentTick, PreviousTick);

#if 0
	int v20 = deltaTicks - simulationTicks;
	bool Jumping = CurrentTick->velocity.z > PreviousTick->velocity.z; // CurrentTick->jumptimemsecs != 0;
	bool Ducking = CurrentTick->duckamount > PreviousTick->duckamount;
	int runtimes = 0;
	if (v20 >= 0)
	{
		pCPlayer->LagCompensatedThisTick = true;
		QAngle OriginalEyeAngles = Entity->GetEyeAngles();
		while (v20 >= 0)
		{
			int i = simulationTicks;
			while (i)
			{

				//Ticks was 1 before I changed it.
				EnginePredictPlayer(Entity, 1, true, Jumping, Ducking, true, pCPlayer);
				i--;
				runtimes++;
			}
			v20 -= simulationTicks;
		}
		Entity->SetEyeAngles(OriginalEyeAngles);
#ifdef _DEBUG
		Entity->DrawHitboxes(ColorRGBA(255, 0, 0, 255), 0.1f);
	}
	std::cout << runtimes << std::endl;

	int v20 = deltaTicks - simulationTicks;
	if (v20 >= 0)
	{
		pCPlayer->LagCompensatedThisTick = true;
		for (int i = 0; i < simulationTicks; i++)
		{
			EnginePredictPlayer(Entity, 1, true);
		}
	}
	Entity->DrawHitboxes(ColorRGBA(255, 0, 0, 255), 0.1f);
#endif
#endif
#else
	//Aimware code. completely broken..
	Vector originalOrigin = CurrentTick->networkorigin;

	CSimulationData data;
	data.networkorigin = *Entity->GetAbsOrigin(); //CurrentTick->networkorigin;
	data.velocity = CurrentTick->velocity;
	data.m_nFlags = CurrentTick->flags;
	data.m_pEntity = Entity;

	INetChannelInfo *nci = Interfaces::Engine->GetNetChannelInfo();

	float tickInterval = 1.0 / Interfaces::Globals->interval_per_tick;

	float deltaTime = ((floorf(((nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING)) * tickInterval) + 0.5)
		+ CurrentUserCmd.cmd->tick_count
		+ 1)
		* Interfaces::Globals->interval_per_tick)
		- CurrentTick->simulationtime;

	if (deltaTime > 1.0f)
		deltaTime = 1.0f;

	float simulationTimeDelta = CurrentTick->simulationtime - PreviousTick->simulationtime;

	int simulationTicks = clamp(floorf((simulationTimeDelta * tickInterval) + 0.5f), 1, 15);

	float velocityDegree = RAD2DEG(atan2(CurrentTick->velocity.x, CurrentTick->velocity.y));
	int deltaTicks = floorf((tickInterval * deltaTime) + 0.5);
	float velocityAngle = velocityDegree - RAD2DEG(atan2(PreviousTick->velocity.x, PreviousTick->velocity.y));
	int times = 1;
	if (velocityAngle <= 180.0)
	{
		if (velocityAngle < -180.0)
			velocityAngle = velocityAngle + 360.0;
	}
	else
	{
		velocityAngle = velocityAngle - 360.0;
	}
	int v20 = deltaTicks - simulationTicks;
	float v21 = velocityAngle / simulationTimeDelta;
	float velocityLength2D = sqrtf((data.velocity.y * data.velocity.y) + (data.velocity.x * data.velocity.x)); //data.velocity.Length2D();

	if (v20 < 0)
	{
		CurrentTick->networkorigin = originalOrigin;
	}
	else
	{
		//pCPlayer->LagCompensatedThisTick = true;

		do
		{
			if (simulationTicks > 0)
			{
				int v22 = simulationTicks;
				do
				{
					float extrapolatedMovement = velocityDegree + (Interfaces::Globals->interval_per_tick * v21);
					data.velocity.x = cosf(DEG2RAD(extrapolatedMovement)) * velocityLength2D;
					data.velocity.y = sinf(DEG2RAD(extrapolatedMovement)) * velocityLength2D;
					bool InAir = !(CurrentTick->flags & FL_ONGROUND);
					SimulateMovement(data, InAir);
					times++;
					velocityDegree = extrapolatedMovement;
					--v22;
				} while (v22);
			}
			v20 -= simulationTicks;
		} while (v20 >= 0);
		//CurrentTick->networkorigin = data.networkorigin;
	}
	std::cout << times << " hell " << std::endl;
	CM_RestoreNetvars(CurrentTick, Entity, &data.networkorigin);
	Entity->DrawHitboxes(ColorRGBA(255, 0, 255, 255), 0.1f);
#endif
}
