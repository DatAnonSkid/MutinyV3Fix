#ifdef MUTINY
#ifndef NO_MUTINY_HEARTBEAT
#include "C:/DeveloperFramework/Framework/Include/Mutiny-Frame.h"
//#pragma comment(lib,"cryptlib.lib")
#include "EncryptString.h"
extern MutinyFrame::Backend* Framework;
#endif
#endif

#include "MutinyInject.h"
#include "CSGO_HX.h"
#include "VMProtectDefs.h"

bool MutinyInjection::StopThread;
int MutinyInjection::InvalidGame;
HINSTANCE MutinyInjection::hfInstance;

void MutinyInjection::OnDLLInjected(HMODULE hInstance)
{
	DWORD ThreadID;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&MutinyInjection::ThreadLoop, NULL, 0, &ThreadID);
}

void MutinyInjection::ThreadLoop(HMODULE hInstance)
{
	void* Think = Wrap_NVInit(hInstance, &InvalidGame, (void*)&MutinyInjection::ReadShort, (void*)&MutinyInjection::WriteShort, (void*)&MutinyInjection::WriteDouble, (void*)&MutinyInjection::ReadDouble, (void*)&MutinyInjection::WriteFloat, (void*)&MutinyInjection::ReadFloat, (void*)&MutinyInjection::WriteInt, (void*)&MutinyInjection::ReadInt, &MutinyInjection::WriteByte, (void*)&MutinyInjection::ReadByte);
	
	if (Think)
	{
		while (!StopThread)
		{
			((void(*)(void))Think)();

#if 0
#ifdef MUTINY
#ifndef NO_MUTINY_HEARTBEAT
			static auto HasHeldValue = -1;
			static auto TimerLastIt = std::chrono::high_resolution_clock::now();

			if (std::chrono::duration_cast<std::chrono::seconds>
				(std::chrono::high_resolution_clock::now() - TimerLastIt).count() > 21)
			{
				auto int_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Framework->Heartbeat->Current - Framework->Heartbeat->Start);
				if (int_ns.count() != HasHeldValue)
				{
					HasHeldValue = int_ns.count();
					TimerLastIt = std::chrono::high_resolution_clock::now();
				}
				else
				{
					THROW_ERROR(VMP_ENCDECSTRINGA("Fatal Security Error in TL"));
					//LogMessage(VMP_ENCDECSTRINGA("Fatal Sec Error in TL"));
					abort();
				}
			}
#endif
#endif
#endif
			Sleep(1);
		}
	}

	StopThread = false;
}

//Replace with driver functions
char __cdecl MutinyInjection::ReadByte(uintptr_t adr) { return *(char*)(adr); }
void __cdecl MutinyInjection::WriteByte(uintptr_t adr, char val) { *(char*)(adr) = val; }
short __cdecl MutinyInjection::ReadShort(uintptr_t adr) { return *(short*)(adr); }
void __cdecl MutinyInjection::WriteShort(uintptr_t adr, short val) { *(short*)(adr) = val; }
int __cdecl MutinyInjection::ReadInt(uintptr_t adr) { return *(int*)(adr); }
void __cdecl MutinyInjection::WriteInt(uintptr_t adr, int val) { *(int*)(adr) = val; }
float __cdecl MutinyInjection::ReadFloat(uintptr_t adr) { return *(float*)(adr); }
/*
Exception thrown at 0x801DB861 (nvollib.dll) in csgo.exe: 0xC0000005: Access violation reading location 0x00000264.
three times
at adr 612.
ReadFloat((DWORD)this + m_flSimulationTime);
From GetSimulationTime() was called just before.
*/
void __cdecl MutinyInjection::WriteFloat(uintptr_t adr, float val) { *(float*)(adr) = val; }
double __cdecl MutinyInjection::ReadDouble(uintptr_t adr) { return *(double*)(adr); }
void __cdecl MutinyInjection::WriteDouble(uintptr_t adr, double val) { *(double*)(adr) = val; }
void __cdecl MutinyInjection::ReadAngle(uintptr_t adr, QAngle& dest) { dest = *(QAngle*)(adr); }
void __cdecl MutinyInjection::WriteAngle(uintptr_t adr, QAngle val) { *(QAngle*)(adr) = val; }
void __cdecl MutinyInjection::ReadVector(uintptr_t adr, Vector& dest) { dest = *(Vector*)(adr); }
void __cdecl MutinyInjection::WriteVector(uintptr_t adr, Vector val) { *(Vector*)(adr) = val; }