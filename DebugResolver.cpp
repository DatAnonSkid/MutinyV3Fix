#include "player_lagcompensation.h"
#include "VMProtectDefs.h"


#ifdef _DEBUG
#include <fstream>
//#define DEBUG_FILE
#ifdef FEET_YAW_DEBUG_FILE
std::ofstream feetyawdebugfile;
#endif

void LagCompensation::OutputFeetYawInformation(CustomPlayer* const pCPlayer, CBaseEntity* const Entity)
{
	static float sCurrentFeetYaw[34];
	static float sGoalFeetYaw[34];
	static float sLowerBodyYaw[34];

	AllocateConsole();

	float CurrentFeetYaw = Entity->GetCurrentFeetYaw();
	float GoalFeetYaw = Entity->GetGoalFeetYaw();
	float LowerBodyYaw = pCPlayer->CurrentNetvarsFromProxy.lowerbodyyaw;
	int index = pCPlayer->Index;

	//If the current does match the saved values then print the changes
	if (CurrentFeetYaw != sCurrentFeetYaw[index]
		|| GoalFeetYaw != sGoalFeetYaw[index]
		|| LowerBodyYaw != sLowerBodyYaw[index])
	{
		wchar_t PlayerName[32];
		GetPlayerName(GetRadar(), index, PlayerName);

		char debuginfo[1024];
		sprintf(debuginfo, "%S : CFY %.3f : GFY %.3f : LBY %.3f : LMB %i\n", PlayerName, ClampYr(CurrentFeetYaw), ClampYr(GoalFeetYaw), LowerBodyYaw, (BOOL)(GetAsyncKeyState(VK_LBUTTON) & (1 << 16)) > 0);

		printf(debuginfo);

#ifdef FEET_YAW_DEBUG_FILE
		if (!feetyawdebugfile.is_open())
		{
			feetyawdebugfile.open("G:\\debug_ft.txt", std::ios::out);
		}

		if (feetyawdebugfile.is_open())
		{
			feetyawdebugfile << debuginfo << "\n";
		}
#endif

		//Store current values
		sCurrentFeetYaw[index] = CurrentFeetYaw;
		sGoalFeetYaw[index] = GoalFeetYaw;
		sLowerBodyYaw[index] = LowerBodyYaw;
	}
}


#ifdef LOWERBODY_YAW_DEBUG_FILE
std::ofstream lowerbodyyawdebugfile;
#endif

void LagCompensation::OutputLowerBodyInformation(CustomPlayer* const pCPlayer, CBaseEntity* const Entity)
{
	if (!AllocedConsole)
	{
		AllocateConsole();
	}

	wchar_t PlayerName[32];
	GetPlayerName(GetRadar(), pCPlayer->Index, PlayerName);

	char debuginfo[1024];
	sprintf(debuginfo, "%S : LBY %.3f\n", PlayerName, pCPlayer->CurrentNetvarsFromProxy.lowerbodyyaw);

	printf(debuginfo);

#ifdef LOWERBODY_YAW_DEBUG_FILE
	if (!lowerbodyyawdebugfile.is_open())
	{
		lowerbodyyawdebugfile.open("G:\\debug_lb.txt", std::ios::out);
	}

	if (lowerbodyyawdebugfile.is_open())
	{
		lowerbodyyawdebugfile << debuginfo << "\n";
	}
#endif
}

std::ofstream serveryawsfile;
void LagCompensation::OutputServerYaws(float eyeyaw, float goalfeet, float curfeet)
{
	if (!serveryawsfile.is_open())
	{
		serveryawsfile.open("G:\\debug_sy.txt", std::ios::out);
	}

	if (serveryawsfile.is_open())
	{
		serveryawsfile << "EyeYaw " << eyeyaw << " GFY " << goalfeet << " CFY " << curfeet << "\n";
		serveryawsfile.flush();
	}
}


#ifdef ANIMATIONS_DEBUG
std::ofstream animationsfile;
C_AnimationLayer AnimLayer[MAX_OVERLAYS];

void LagCompensation::OutputAnimations(CustomPlayer* const pCPlayer, CBaseEntity* const Entity)
{
	if (!animationsfile.is_open())
	{
		animationsfile.open("G:\\debug_an.txt", std::ios::out);
	}

	if (animationsfile.is_open())
	{
#if 1
		static float lastnextupdatetime = 0.0f;
		DWORD cl = GetServerClientEntity(2);
		CBaseEntity *pEnt = ServerClientToEntity(cl);
		float nextupdatetime = pEnt->GetNextLowerBodyyawUpdateTimeServer();
		bool update = false;

		if (nextupdatetime != lastnextupdatetime)
		{
			animationsfile << "Server LBY Update\n";
			lastnextupdatetime = nextupdatetime;
			update = true;
		}
#endif

		int numoverlays = Entity->GetNumAnimOverlays();

		for (int i = 0; i < numoverlays; i++)
		{
			C_AnimationLayer *layer = Entity->GetAnimOverlay(i);

#if 1
			if (layer->_m_nSequence != AnimLayer[i]._m_nSequence)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Sequence: " << AnimLayer[i]._m_nSequence << " -> " << layer->_m_nSequence << "\n";
				update = true;
			}
#endif

			if (layer->_m_flCycle != AnimLayer[i]._m_flCycle)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Cycle: " << AnimLayer[i]._m_flCycle << " -> " << layer->_m_flCycle << "\n";
				update = true;
			}

			if (layer->_m_flPlaybackRate != AnimLayer[i]._m_flPlaybackRate)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Playback: " << AnimLayer[i]._m_flPlaybackRate << " -> " << layer->_m_flPlaybackRate << "\n";
				update = true;
			}

			if (layer->m_nOrder != AnimLayer[i].m_nOrder)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Order: " << AnimLayer[i].m_nOrder << " -> " << layer->m_nOrder << "\n";
				update = true;
			}

			if (layer->m_nInvalidatePhysicsBits != AnimLayer[i].m_nInvalidatePhysicsBits)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Physics: " << AnimLayer[i].m_nInvalidatePhysicsBits << " -> " << layer->m_nInvalidatePhysicsBits << "\n";
				update = true;
			}

			if (layer->m_flWeight != AnimLayer[i].m_flWeight)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " Weight: " << AnimLayer[i].m_flWeight << " -> " << layer->m_flWeight << "\n";
				update = true;
			}

			if (layer->m_flLayerAnimtime != AnimLayer[i].m_flLayerAnimtime)
			{
				animationsfile << "#" << Entity->index << " OL: " << i << " Tick: " << Interfaces::Globals->tickcount << " AnimTime: " << AnimLayer[i].m_flLayerAnimtime << " -> " << layer->m_flLayerAnimtime << "\n";
				update = true;
			}

			if (update)
				AnimLayer[i] = *layer;
		}

		if (update)
			animationsfile << "\n" << std::endl;

		//animationsfile.flush();
	}
}
#endif

#endif