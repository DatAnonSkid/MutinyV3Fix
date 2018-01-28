#pragma once
#include "VTHook.h"

#if 0
extern QAngle camAngles;
extern Vector camOrigin;
extern bool ThirdPerson;
extern Vector m_vecDesiredCameraOffset;
extern Vector m_vecCameraOffset;
Vector GetFinalCameraOffset(void);
#endif
void SetThirdPersonAngles();
void UpdateThirdPerson(CViewSetup *pSetup);
#if 0
inline void SetDesiredCameraOffset(const Vector& vecOffset) { m_vecDesiredCameraOffset = vecOffset; }
inline const Vector& GetDesiredCameraOffset(void) { return m_vecDesiredCameraOffset; }
inline void SetCameraOffsetAngles(const Vector& vecOffset) { m_vecCameraOffset = vecOffset; }
inline const Vector&	GetCameraOffsetAngles(void) { return m_vecCameraOffset; }
#endif