#pragma once
#include "CSGO_HX.h"

/*
struct WeaponInfo_t
{
float m_flArmorRatio;
float m_flPenetration;
float m_iDamage;
float m_flRange;
float m_flRangeModifier;
};
*/

extern DWORD WeaponDataPtrUnknown;
extern DWORD WeaponDataPtrUnknownCall;
extern DWORD GetCSWeaponDataAdr;

class CBaseEntity;

enum CSWeaponType
{
	WEAPONTYPE_KNIFE = 0,
	WEAPONTYPE_PISTOL,
	WEAPONTYPE_SUBMACHINEGUN,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_SHOTGUN,
	WEAPONTYPE_SNIPER_RIFLE,
	WEAPONTYPE_MACHINEGUN,
	WEAPONTYPE_C4,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_UNKNOWN
};

struct CHudTexture
{
	char	szShortName[64];	//0x0000
	char	szTextureFile[64];	//0x0040
	bool	bRenderUsingFont;	//0x0080
	bool	bPrecached;			//0x0081
	char	cCharacterInFont;	//0x0082
	BYTE	pad_0x0083;			//0x0083
	int		hFont;				//0x0084
	int		iTextureId;			//0x0088
	float	afTexCoords[4];		//0x008C
	int		iPosX[4];			//0x009C
}; //Size=0x00AC

class WeaponInfo_t
{
public:

	virtual ~WeaponInfo_t() {};
	/*Parse(KeyValues *, char const*)
	RefreshDynamicParameters(void)
	GetPrimaryClipSize(C_EconItemView const*, int, float)const
	GetSecondaryClipSize(C_EconItemView const*, int, float)const
	GetDefaultPrimaryClipSize(C_EconItemView const*, int, float)const
	GetDefaultSecondaryClipSize(C_EconItemView const*, int, float)const
	GetPrimaryReserveAmmoMax(C_EconItemView const*, int, float)const
	GetSecondaryReserveAmmoMax(C_EconItemView const*, int, float)const*/

	char*		consoleName;			// 0x0004
	char		pad_0008[12];			// 0x0008
	int			iMaxClip1;				// 0x0014
	int			iMaxClip2;				// 0x0018
	int			iDefaultClip1;			// 0x001C
	int			iDefaultClip2;			// 0x0020
	char		pad_0024[8];			// 0x0024
	char*		szWorldModel;			// 0x002C
	char*		szViewModel;			// 0x0030
	char*		szDroppedModel;			// 0x0034
	char		pad_0038[4];			// 0x0038
	char*		N0000023E;				// 0x003C
	char		pad_0040[56];			// 0x0040
	char*		szEmptySound;			// 0x0078
	char		pad_007C[4];			// 0x007C
	char*		szBulletType;			// 0x0080
	char		pad_0084[4];			// 0x0084
	char*		szHudName;				// 0x0088
	char*		szWeaponName;			// 0x008C
	char		pad_0090[56];			// 0x0090
	int 		WeaponType;				// 0x00C8
	int			iWeaponPrice;			// 0x00CC
	int			iKillAward;				// 0x00D0
	char*		szAnimationPrefix;		// 0x00D4
	float		flCycleTime;			// 0x00D8
	float		flCycleTimeAlt;			// 0x00DC
	float		flTimeToIdle;			// 0x00E0
	float		flIdleInterval;			// 0x00E4
	bool		bFullAuto;				// 0x00E8
	char		pad_0x00E5[3];			// 0x00E9
	int			iDamage;				// 0x00EC
	float		flArmorRatio;			// 0x00F0
	int			iBullets;				// 0x00F4
	float		flPenetration;			// 0x00F8
	float		flFlinchVelocityModifierLarge;	// 0x00FC
	float		flFlinchVelocityModifierSmall;	// 0x0100
	float		flRange;				// 0x0104
	float		flRangeModifier;		// 0x0108
	float		flThrowVelocity;		// 0x010C
	char		pad_0x010C[12];			// 0x0110
	bool		bHasSilencer;			// 0x011C
	char		pad_0x0119[3];			// 0x011D
	char*		pSilencerModel;			// 0x0120
	int			iCrosshairMinDistance;	// 0x0124
	int			iCrosshairDeltaDistance;// 0x0128 - iTeam?
	float		flMaxPlayerSpeed;		// 0x012C
	float		flMaxPlayerSpeedAlt;	// 0x0130
	float		flSpread;				// 0x0134
	float		flSpreadAlt;			// 0x0138
	float		flInaccuracyCrouch;		// 0x013C
	float		flInaccuracyCrouchAlt;	// 0x0140
	float		flInaccuracyStand;		// 0x0144
	float		flInaccuracyStandAlt;	// 0x0148
	float		flInaccuracyJumpInitial;// 0x014C
	float		flInaccuracyJump;		// 0x0150
	float		flInaccuracyJumpAlt;	// 0x0154
	float		flInaccuracyLand;		// 0x0158
	float		flInaccuracyLandAlt;	// 0x015C
	float		flInaccuracyLadder;		// 0x0160
	float		flInaccuracyLadderAlt;	// 0x0164
	float		flInaccuracyFire;		// 0x0168
	float		flInaccuracyFireAlt;	// 0x016C
	float		flInaccuracyMove;		// 0x0170
	float		flInaccuracyMoveAlt;	// 0x0174
	float		flInaccuracyReload;		// 0x0178
	int			iRecoilSeed;			// 0x017C
	float		flRecoilAngle;			// 0x0180
	float		flRecoilAngleAlt;		// 0x0184
	float		flRecoilAngleVariance;	// 0x0188
	float		flRecoilAngleVarianceAlt;	// 0x018C
	float		flRecoilMagnitude;		// 0x0190
	float		flRecoilMagnitudeAlt;	// 0x0194
	float		flRecoilMagnitudeVariance;	// 0x0198
	float		flRecoilMagnitudeVarianceAlt;	// 0x019C
	float		flRecoveryTimeCrouch;	// 0x01A0
	float		flRecoveryTimeStand;	// 0x01A4
	float		flRecoveryTimeCrouchFinal;	// 0x01A8
	float		flRecoveryTimeStandFinal;	// 0x01AC
	int			iRecoveryTransitionStartBullet;// 0x01B0 
	int			iRecoveryTransitionEndBullet;	// 0x01B4
	bool		bUnzoomAfterShot;		// 0x01B8
	bool		bHideViewModelZoomed;	// 0x01B9
	char		pad_0x01B5[2];			// 0x01BA
	char		iZoomLevels[3];			// 0x01BC
	int			iZoomFOV[2];			// 0x01C0
	float		fZoomTime[3];			// 0x01C4
	char*		szWeaponClass;			// 0x01D4
	float		flAddonScale;			// 0x01D8
	char		pad_0x01DC[4];			// 0x01DC
	char*		szEjectBrassEffect;		// 0x01E0
	char*		szTracerEffect;			// 0x01E4
	int			iTracerFrequency;		// 0x01E8
	int			iTracerFrequencyAlt;	// 0x01EC
	char*		szMuzzleFlashEffect_1stPerson; // 0x01F0
	char		pad_0x01F4[4];			 // 0x01F4
	char*		szMuzzleFlashEffect_3rdPerson; // 0x01F8
	char		pad_0x01FC[4];			// 0x01FC
	char*		szMuzzleSmokeEffect;	// 0x0200
	float		flHeatPerShot;			// 0x0204
	char*		szZoomInSound;			// 0x0208
	char*		szZoomOutSound;			// 0x020C
	float		flInaccuracyPitchShift;	// 0x0210
	float		flInaccuracySoundThreshold;	// 0x0214
	float		flBotAudibleRange;		// 0x0218
	char		pad_0x0218[8];			// 0x0220
	char*		pWrongTeamMsg;			// 0x0224
	bool		bHasBurstMode;			// 0x0228
	char		pad_0x0225[3];			// 0x0229
	bool		bIsRevolver;			// 0x022C
	bool		bCannotShootUnderwater;	// 0x0230
};

/*
char* m_szWeaponName; //0x0004 
char pad_0x0008[0x8]; //0x0008
CCStrike15ItemDefinition* m_pItemDefinition; //0x0010 
int m_iMaxClip1; //0x0014 
char pad_0x0018[0xC]; //0x0018
int m_iMaxAmmo; //0x0024 
char pad_0x0028[0x4]; //0x0028
char* m_szWorldModel; //0x002C 
char* m_szViewModel; //0x0030 
char* m_szDroppedModel; //0x0034 
char pad_0x0038[0x4]; //0x0038
char* N0000054E; //0x003C 
char pad_0x0040[0x38]; //0x0040
char* N0000055D; //0x0078 
char pad_0x007C[0x4]; //0x007C
char* m_szAmmoType; //0x0080 
char pad_0x0084[0x4]; //0x0084
char* m_szLocalizedName; //0x0088 
char pad_0x008C[0x3C]; //0x008C
int m_eWeaponType; //0x00C8 
int m_iPrice; //0x00CC 
char pad_0x00D0[0x4]; //0x00D0
char* m_szAnimationPrefix; //0x00D4 
char pad_0x00D8[0x14]; //0x00D8
int m_iDamage; //0x00EC 
float m_flArmorRatio; //0x00F0 
char pad_0x00F4[0x4]; //0x00F4
float m_flPenetration; //0x00F8 
char pad_0x00FC[0x8]; //0x00FC
float m_flRange; //0x0104 
float m_flRangeModifier; //0x0108 
char pad_0x010C[0x10]; //0x010C
unsigned char m_bHasSilencer; //0x011C 
char pad_0x011D[0x10B]; //0x011D
unsigned char m_bHasBurstmode; //0x0228 
unsigned char m_bIsRevolver; //0x0229 
char pad_0x022A[0x1BE]; //0x022A

	};
	*/

class CCSWeaponInfo_t
{
public:
	void* vTable; //0x0000
	BYTE N0309F389; //0x0004
	BYTE N030AC4C1; //0x0005
	char name_class[93]; //0x0006
	char printname[80]; //0x00
	char viewmodel[80]; //0x00A6
	char playermodel[80]; //0x00F6
	char bullettype[32]; //0x0146
	char anim_prefix[58]; //0x0166
	char N03122066[32]; //0x01A0
	char contentpack[32]; //0x01C0
	char unknowndata00[128]; //0x01E0
	char name_real[32]; //0x0260
	char unknowndata01[1184]; //0x0280
	CHudTexture* hudTexture_silenced; //0x0720
	CHudTexture* hudTexture; //0x0724
	CHudTexture* hudTexture_ammo; //0x0728
	char unknowndata02[4]; //0x072C
	CHudTexture* hudTexture_crosshair_01; //0x0730
	CHudTexture* hudTexture_crosshair_02; //0x0734
	CHudTexture* hudTexture_crosshair_03; //0x0738
	CHudTexture* hudTexture_crosshair_04; //0x073C
	char unknowndata03[8]; //0x0740
	__int32 WeaponType; //0x0748
	__int32 Team; //0x074C
	__int32 WeaponID; //0x0750
	BYTE FullAuto; //0x0754
	char unknowndata04[7]; //0x0755
	__int32 WeaponPrice; //0x075C
	float WeaponArmorRatio; //0x0760
	float MaxPlayerSpeed; //0x0764
	float MaxPlayerSpeedAlt; //0x0768
	__int32 CrosshairMinDistance; //0x076C
	__int32 CrosshairDeltaDistance; //0x0770
	float flPenetrationPower; //0x0774
	__int32 iDamage; //0x0778
	float flRange; //0x077C
	float flRangeModifier; //0x0780
	__int32 iBullets; //0x0784
	float CycleTime; //0x0788

};//Size=0x078C

class CBaseCombatWeapon
{
public:
	char			__pad[0x64];
	int				index;
	int			GetWeaponID();
	float			GetNextPrimaryAttack();
	float GetNextSecondaryAttack();
	float GetPostPoneFireReadyTime();
	float			GetAccuracyPenalty();
	int			GetXUIDLow();
	int			GetXUIDHigh();
	int			GetEntityQuality();
	int			GetAccountID();
	int			GetItemIDHigh();
	int 			GetItemDefinitionIndex();
	int			GetFallbackPaintKit();
	int			GetFallbackStatTrak();
	float			GetFallbackWear();
	BOOL			IsEmpty();
	BOOLEAN			IsReloading();
	int GetClipOne();
	void			UpdateAccuracyPenalty();
	float			GetWeaponSpread();
	float			GetWeaponCone();
	WeaponInfo_t*	GetCSWpnData();
	bool			IsGun();
	std::string		GetWeaponName();
	bool IsPistol(int OptionalItemDefinitionIndex);
	bool IsShotgun(int OptionalItemDefinitionIndex);
	bool IsKnife(int OptionalItemDefinitionIndex);
	bool IsSniper(bool IncludeAutoSnipers, int OptionalItemDefinitionIndex);
	bool IsGrenade(int OptionalItemDefinitionIndex);
};