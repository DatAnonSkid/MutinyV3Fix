#include "player_lagcompensation.h"
#include "Logging.h"

void Normalise(double &value)
{

	if (!std::isfinite(value))
	{
		value = 0;
	}

	while (value <= -180) value += 360;
	while (value > 180) value -= 360;
	if (value < -180) value = -179.999;
	if (value > 180) value = 179.999;

}
float Vec2Ang(Vector Velocity)
{
	if (Velocity.x == 0 || Velocity.y == 0)
		return 0;
	float rise = Velocity.x;
	float run = Velocity.y;
	float value = rise / run;
	float theta = atan(value);
	theta = RAD2DEG(theta) + 90;
	if (Velocity.y < 0 && Velocity.x > 0 || Velocity.y < 0 && Velocity.x < 0)
		theta *= -1;
	else
		theta = 180 - theta;
	return theta;
}

double difference(double first, double second)
{
	Normalise(first);
	Normalise(second);
	double returnval = first - second;
	if (first < -91 && second> 91 || first > 91 && second < -91)
	{
		double negativedifY = 180 - abs(first);
		double posdiffenceY = 180 - abs(second);
		returnval = negativedifY + posdiffenceY;
	}
	return returnval;
}

//Probably double handling with these value storing
//Call this every single tick that the player's information updates (ie they send a tick again).
int usingAimwareAdaptive(CustomPlayer*const pCPlayer)
{

	float VelocityY = pCPlayer->BaseEntity->GetVelocity().y;
	float VelocityX = pCPlayer->BaseEntity->GetVelocity().x;
	int WishTicks_1 = 0;
	int AdaptTicks = 2;
	double ExtrapolatedSpeed = sqrt((VelocityX * VelocityX) + (VelocityY * VelocityY))
		* Interfaces::Globals->interval_per_tick;
	while ((WishTicks_1 * ExtrapolatedSpeed) <= 68.0)
	{
		if (((AdaptTicks - 1) * ExtrapolatedSpeed) > 68.0)
		{
			++WishTicks_1;
			break;
		}
		if ((AdaptTicks * ExtrapolatedSpeed) > 68.0)
		{
			WishTicks_1 += 2;
			break;
		}
		if (((AdaptTicks + 1) * ExtrapolatedSpeed) > 68.0)
		{
			WishTicks_1 += 3;
			break;
		}
		if (((AdaptTicks + 2) * ExtrapolatedSpeed) > 68.0)
		{
			WishTicks_1 += 4;
			break;
		}
		AdaptTicks += 5;
		WishTicks_1 += 5;
		if (AdaptTicks > 16)
			break;
	}
	if (WishTicks_1 > 16)
		WishTicks_1 = 15;
	return WishTicks_1;
}

void recordChokedTicks(CustomPlayer*const pCPlayer, int ChockedTics)
{
	//inacurate first 20 logs but meh
	pCPlayer->PersistentData.fakeLagHistoryCounter++;
	if (pCPlayer->PersistentData.fakeLagHistoryCounter > 19)
		pCPlayer->PersistentData.fakeLagHistoryCounter = 0;
	pCPlayer->PersistentData.historyPacketsChoked[pCPlayer->PersistentData.fakeLagHistoryCounter] = ChockedTics;

	if (usingAimwareAdaptive(pCPlayer) == ChockedTics)
		pCPlayer->PersistentData.UsingAwareAdaptive = true;
	else
		pCPlayer->PersistentData.UsingAwareAdaptive = false;


}
//int ChockedTics = number of ticks chocked since last sending a packet
int predictedChokedTicks(CustomPlayer*const pCPlayer, int CurrentChockedTics)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("predictChokedTicks\n");
#endif
	//Assuming polnomial expansions, lets just keep differentialting the equation until we get something which works ;)
	int total = 0;
	for (int i = 0; i < 20; i++)
	{
		total += pCPlayer->PersistentData.historyPacketsChoked[i];
	}
	int average = total / 20;
	double SD = 0;
	for (int i = 0; i < 20; i++)
	{
		SD += ((pCPlayer->PersistentData.historyPacketsChoked[i] - average)*(pCPlayer->PersistentData.historyPacketsChoked[i] - average));
	}
	SD /= (SD - 1);
	SD = sqrt(SD);

	if (SD <= 2 && CurrentChockedTics <= average)
		return average; // Probably average if SD is so low

	if (pCPlayer->PersistentData.UsingAwareAdaptive)
		return usingAimwareAdaptive(pCPlayer);

	//See if its a linear step
	int totalStep = 0;
	//2nd degree step
	int difference2[19];
	int totalstep2 = 0;
	//3rd degree step
	int difference3[18];
	int totalstep3 = 0;
	for (int i = 0; i < 19; i++)
	{
		difference2[i] += pCPlayer->PersistentData.historyPacketsChoked[i + 1];
		difference2[i] -= pCPlayer->PersistentData.historyPacketsChoked[i];
		totalStep += difference2[i];
	}
	int averageStep = totalStep / 19;
	double SDStep = 0;
	for (int i = 0; i < 19; i++)
	{
		SDStep += ((difference2[i] - averageStep)*(difference2[i] - averageStep));
	}
	SDStep /= (SDStep - 1);
	SDStep = sqrt(SDStep);

	if (SDStep <= 1 && CurrentChockedTics <= averageStep)
		return averageStep; // Probably average if SD is so l



	for (int i = 0; i < 18; i++)
	{
		difference3[i] += difference2[i + 1];
		difference3[i] -= difference2[i];
		totalstep2 += difference3[i];
	}
	int averageStep2 = totalstep2 / 18;
	double SDStep2 = 0;
	for (int i = 0; i < 18; i++)
	{
		SDStep2 += ((difference2[i] - averageStep2)*(difference2[i] - averageStep2));
	}
	SDStep2 /= (SDStep2 - 1);
	SDStep2 = sqrt(SDStep2);

	if (SDStep2 <= 1 && CurrentChockedTics <= averageStep2)
		return averageStep2; // Probably average if SD is so l


	int difference4[17];
	for (int i = 0; i < 17; i++)
	{
		difference4[i] += difference3[i + 1];
		difference4[i] -= difference3[i];
		totalstep3 += difference3[i];
	}
	int averageStep3 = totalstep3 / 17;
	double SDStep3 = 0;
	for (int i = 0; i < 17; i++)
	{
		SDStep3 += ((difference3[i] - averageStep3)*(difference3[i] - averageStep3));
	}
	SDStep3 /= (SDStep3 - 1);
	SDStep3 = sqrt(SDStep3);

	if (SDStep3 <= 1 && CurrentChockedTics <= averageStep3)
		return averageStep3; // Probably average if SD is so l

							 //We have now checked if they are following a polynomial up to degree 4
							 //See here for complete complex algorthym http://www.johansens.us/sane/education/formula.htm
	int lastamount = pCPlayer->PersistentData.historyPacketsChoked[pCPlayer->PersistentData.fakeLagHistoryCounter];
	if (CurrentChockedTics <= lastamount)
		return lastamount;
	return CurrentChockedTics;

	//Check for aimware adaptive

}


float accuracy = 10;

void clamp(float &value)
{
	while (value > 180)
		value -= 360;
	while (value < -180)
		value += 360;
}
float clamp2(float value)
{
	while (value > 180)
		value -= 360;
	while (value < -180)
		value += 360;
	return value;
}
float difference(float first, float second)
{
	clamp(first);
	clamp(second);
	float returnval = first - second;
	if (first < -91 && second> 91 || first > 91 && second < -91)
	{
		double negativedifY = 180 - abs(first);
		double posdiffenceY = 180 - abs(second);
		returnval = negativedifY + posdiffenceY;
	}
	return returnval;
}

float mean(float real[scanstrength], int slots)
{
	float total = 0;
	for (int i = 0; i < slots; i++)
	{
		total += real[i];
	}
	return total / slots;
}
float SD(float real[scanstrength], float mean, int slots)
{
	float total = 0;
	for (int i = 0; i < slots; i++)
	{
		total += (real[i] - mean)*(real[i] - mean);
	}
	total /= slots;
	return sqrt(total);
}
bool checkSpin(float real[scanstrength], int ticks[scanstrength], float &meanvalue, int &level, int slots)
{
	float dif[scanstrength - 1];

	int ticks2[scanstrength - 1];
	float dif2[scanstrength - 2];

	for (int i = 0; i < slots - 1; i++)
	{
		ticks2[i] = abs(ticks[i + 1] - ticks[i]);
		if (ticks2[i] == 1)
			ticks2[i] = 1;
		dif[i] = clamp2(difference(real[i], real[i + 1]) / (ticks2[i]));
	}
	meanvalue = mean(dif, slots - 1);
	if (SD(dif, meanvalue, slots - 1) < accuracy)
	{
		level = 1;
		return true;
	}


	for (int i = 0; i < slots - 2; i++)
	{
		int ticks = abs(ticks2[i + 1] - ticks2[i]);
		if (ticks == 0)
			ticks = 1;

		dif2[i] = clamp2(difference(dif[i], dif[i + 1]) / (ticks));
	}
	meanvalue = mean(dif2, slots - 2);
	if (SD(dif2, meanvalue, slots - 2) < accuracy)
	{
		level = 2;
		return true;
	}

	return false;
}
bool checkJitter(float real[scanstrength], int ticks[scanstrength], float &meanvalue, int &level, int slots)
{
	float dif[scanstrength - 1] = { 0 };


	int ticks2[scanstrength - 1] = { 0 };
	float dif2[scanstrength - 2] = { 0 };

	for (int i = 0; i < slots - 1; i++)
	{
		ticks2[i] = abs(ticks[i + 1] - ticks[i]);
		if (ticks2[i] == 1)
			ticks2[i] = 1;

		dif[i] = clamp2(abs(difference(real[i], real[i + 1]) / (ticks2[i])));
	}
	meanvalue = mean(dif, slots - 1);
	if (SD(dif, meanvalue, slots - 1) < accuracy)
	{
		level = 1;
		return true;
	}


	for (int i = 0; i < slots - 2; i++)
	{
		int ticks = abs(ticks2[i + 1] - ticks2[i]);
		if (ticks == 0)
			ticks = 1;

		dif2[i] = clamp2(abs(difference(dif[i], dif[i + 1]) / (ticks)));
	}
	meanvalue = mean(dif2, slots - 2);
	if (SD(dif2, meanvalue, slots - 2) < accuracy)
	{
		level = 2;
		return true;
	}

	return false;
}
bool checkStatic(float real[scanstrength], float meanvalue, int slots)
{
	meanvalue = mean(real, slots);
	if (SD(real, meanvalue, slots) < accuracy)
	{
		return true;
	}
	return false;
}
//TOFIX: level 2 jitter's and spins are not dont right
//Jitter's arent done right
//Add history to which of the options worked
void LogicResolver2_Log(QAngle angles, QAngle real, bool knowReal, CustomPlayer* pCPlayer, CBaseEntity* pLocalEntity)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("LogicResolver2_Log\n");
#endif
	if (knowReal)
	{
		int history = WhaleDongTxt.iValue;
		CBaseEntity * Target = pCPlayer->BaseEntity;

		int counter = pCPlayer->PersistentData.counter;
		pCPlayer->PersistentData.counter++;
		if (pCPlayer->PersistentData.counter >= history)
			pCPlayer->PersistentData.counter = 0;
		pCPlayer->PersistentData.realValues[counter] = real.y;
		pCPlayer->PersistentData.fakeValues[counter] = angles.y;
		pCPlayer->PersistentData.targetDValues[counter] = CalcAngle(Target->GetEyePosition(), pLocalEntity->GetEyePosition()).y;
		pCPlayer->PersistentData.velocityDValues[counter] = Vec2Ang(Target->GetVelocity());
		pCPlayer->PersistentData.ticksReal[counter] = TIME_TO_TICKS(Interfaces::Globals->curtime);
		pCPlayer->PersistentData.lbyUpdate = Interfaces::Globals->curtime;
		pCPlayer->PersistentData.curTimeUpdatedLast = Interfaces::Globals->curtime;
		pCPlayer->PersistentData.lastlby = real.y;

	}
}


void LogicResolver2_Predict(QAngle &oAngles, bool predict, CustomPlayer* pCPlayer, CBaseEntity* pLocalEntity, int ticks)
{
#ifdef PRINT_FUNCTION_NAMES
	LogMessage("LogicResolver2_Predict\n");
#endif
	int history = WhaleDongTxt.iValue;
	CBaseEntity * Target = pCPlayer->BaseEntity;

	float realValues[scanstrength];
	float fakeValues[scanstrength];
	float targetDValues[scanstrength];
	float velocityDValues[scanstrength];
	int ticksReal[scanstrength];

	for (int i = 0; i < history; i++)
	{
		realValues[i] = pCPlayer->PersistentData.realValues[i];
		fakeValues[i] = pCPlayer->PersistentData.fakeValues[i];
		targetDValues[i] = pCPlayer->PersistentData.targetDValues[i];
		velocityDValues[i] = pCPlayer->PersistentData.velocityDValues[i];
		ticksReal[i] = pCPlayer->PersistentData.ticksReal[i];
	}



	float lastUpdate = pCPlayer->PersistentData.curTimeUpdatedLast;
	float currentTime = Interfaces::Globals->curtime;

	float lastReal = pCPlayer->PersistentData.lastlby;
	float currentFake = oAngles.y;
	float currentTargetD = CalcAngle(Target->GetEyePosition(), pLocalEntity->GetEyePosition()).y;
	float currentVelocityD = Vec2Ang(Target->GetVelocity());
	bool onGround = (Target->GetFlags() & FL_ONGROUND);

	int shotsMissed = pCPlayer->PersistentData.ShotsnotHit;
	float ResolvedAngle = lastReal;
	float lastLBY = lastReal;
	StoredNetvars *CurrentNetVars = pCPlayer->GetCurrentRecord();
	if (CurrentNetVars)
	{
		auto LowerBodyRecords = &pCPlayer->PersistentData.m_PlayerLowerBodyRecords;
		lastReal = CurrentNetVars->lowerbodyyaw;
		lastLBY = CurrentNetVars->lowerbodyyaw;
	}

	float testValues[scanstrength];



	int tick2predict = 1;

	//Do prediction shit
	//Real vs Fakes
	for (int i = 0; i < history; i++)
	{
		testValues[i] = clamp2(difference(realValues[i], fakeValues[i]));
	}

	float RFstaticMean = 0;;
	bool RFisStatic = checkStatic(testValues, RFstaticMean, history);
	int RFlevelJitter = 0;
	float RFlevelJitterMean;
	bool RFisJitter = checkJitter(testValues, ticksReal, RFlevelJitterMean, RFlevelJitter, history);
	int RFlevelSpin = 0;
	float RFlevelCheckMean;
	bool RFisSpin = checkSpin(testValues, ticksReal, RFlevelCheckMean, RFlevelSpin, history);

	//Real vs Target
	for (int i = 0; i < history; i++)
	{
		testValues[i] = clamp2(difference(realValues[i], targetDValues[i]));
	}

	float RTstaticMean = 0;;
	bool RTisStatic = checkStatic(testValues, RTstaticMean, history);
	int RTlevelJitter = 0;
	float RTlevelJitterMean;
	bool RTisJitter = checkJitter(testValues, ticksReal, RTlevelJitterMean, RTlevelJitter, history);
	int RTlevelSpin = 0;
	float RTlevelCheckMean;
	bool RTisSpin = checkSpin(testValues, ticksReal, RTlevelCheckMean, RTlevelSpin, history);

	//Delta Checks
	for (int i = 0; i < history; i++)
	{
		testValues[i] = clamp2(difference(realValues[i], 0));
	}

	float staticMean = 0;
	bool isStatic = checkStatic(testValues, staticMean, history);
	int levelJitter = 0;
	float levelJitterMean;
	bool isJitter = checkJitter(testValues, ticksReal, levelJitterMean, levelJitter, history);
	int levelSpin = 0;
	float levelCheckMean;
	bool isSpin = checkSpin(testValues, ticksReal, levelCheckMean, levelSpin, history);


	//Real vs Velocity

	for (int i = 0; i < history; i++)
	{
		testValues[i] = clamp2(difference(realValues[i], velocityDValues[i]));
	}

	float RVstaticMean = 0;;
	bool RVisStatic = checkStatic(testValues, RVstaticMean, history);
	int RVlevelJitter = 0;
	float RVlevelJitterMean;
	bool RVisJitter = checkJitter(testValues, ticksReal, RVlevelJitterMean, RVlevelJitter, history);
	int RVlevelSpin = 0;
	float RVlevelCheckMean;
	bool RVisSpin = checkSpin(testValues, ticksReal, RVlevelCheckMean, RVlevelSpin, history);


	//Let us predict!
	std::vector <float> angles;
	std::vector <int> resolveMode;
	float angle = 0;

	//Real vs fake
	if (RFisStatic)
	{
		angle = RFstaticMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_StaticFake);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_StaticFake);
		}
	}
	if (RFisJitter)
	{
		if (RFlevelJitter == 2)
			angle = lastReal + RFlevelJitterMean * 1.5;
		else
			angle = lastReal + RFlevelJitterMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_StaticFake);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_StaticFake);
		}
	}
	if (RFisSpin)
	{
		if (RFlevelSpin == 2)
			angle = lastReal + RFlevelCheckMean * 1.5;
		else
			angle = lastReal + RFlevelCheckMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_StaticFake);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_StaticFake);
		}
	}

	//Real vs target
	if (RTisStatic)
	{
		angle = RTstaticMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Target);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Target);
		}
	}
	if (RTisJitter)
	{
		if (RTlevelJitter == 2)
			angle = lastReal + RTlevelJitterMean * 1.5;
		else
			angle = lastReal + RTlevelJitterMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Target);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Target);
		}
	}
	if (RTisSpin)
	{
		if (RTlevelSpin == 2)
			angle = lastReal + RTlevelCheckMean * 1.5;
		else
			angle = lastReal + RTlevelCheckMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Target);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Target);
		}
	}


	//Real delta
	if (isStatic)
	{
		angle = staticMean;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Static);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Static);
		}

	}
	if (isJitter)
	{
		//Needs to be right
		if (levelJitter == 2)
			angle = lastReal + levelJitterMean * 1.5;
		else
		{
			if (tick2predict % 2)
				angle = lastReal + levelJitterMean;
			else
				angle = lastReal;
		}
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Jitter);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Jitter);
		}
	}
	if (isSpin)
	{
		if (levelSpin == 2)
			angle = lastReal + levelCheckMean * tick2predict * 1.5;
		else
			angle = lastReal + levelCheckMean * tick2predict;
		if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Spin);
		}
		else if (!onGround || (currentTime - lastUpdate < 1.1f))
		{
			angles.push_back(angle);
			resolveMode.push_back(WD_Spin);
		}
	}

	//Real vs Velocity
	if (abs(currentVelocityD) > 0)
	{
		if (RVisStatic)
		{
			angle = RVstaticMean;
			if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
			{
				angles.push_back(angle);
				resolveMode.push_back(WD_Velocity);
			}
			else if (!onGround || (currentTime - lastUpdate < 1.1f))
			{
				angles.push_back(angle);
				resolveMode.push_back(WD_Velocity);
			}
		}
		if (RVisJitter)
		{
			if (RVlevelJitter == 2)
				angle = lastReal + RVlevelJitterMean * 1.5;
			else
				angle = lastReal + RVlevelJitterMean;
			if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
			{
				angles.push_back(angle);
				resolveMode.push_back(WD_Velocity);
			}
			else if (!onGround || (currentTime - lastUpdate < 1.1f))
			{
				angles.push_back(angle);
				resolveMode.push_back(WD_Velocity);
			}
		}
		if (RVisSpin)
		{
			if (RVlevelSpin == 2)
				angle = lastReal + RVlevelCheckMean * 1.5;
			else
				angle = lastReal + RVlevelCheckMean;
			if (onGround && (currentTime - lastUpdate > 1.1f) && (difference(angle, lastLBY) < 35))
			{
				angles.push_back(angle);
				resolveMode.push_back(WD_Velocity);
			}
			else if (!onGround || (currentTime - lastUpdate < 1.1f))
			{
				angles.push_back(angle);
				resolveMode.push_back(WD_Velocity);
			}
		}

	}
	//Angles have been predicted
	//Let us brute force and set what we want most :)

	//Lets check if any of our algorthyms worked
	if (angles.size() > 0)
	{
		int resolveOption = shotsMissed % angles.size();
		ResolvedAngle = angles[resolveOption];
		pCPlayer->PersistentData.LastResolveModeUsed = (ResolveYawModes)resolveMode[resolveOption];

	}
	else if (onGround && (currentTime - lastUpdate > 1.1f))
	{
		pCPlayer->PersistentData.LastResolveModeUsed = WD_FuckIt;

		ResolvedAngle = lastLBY;

		switch ((int)shotsMissed % 9)
		{
		case 1:
			ResolvedAngle += 35;
			break;
		case 3:
			ResolvedAngle -= 35;
			break;
		case 5:
			ResolvedAngle += 20;
			break;
		case 7:
			ResolvedAngle -= 20;
			break;
		}
	}
	int resolverNumber = 0;
	resolverNumber = 30 * (shotsMissed % 13) - 180;
	if (abs(pCPlayer->PersistentData.ShotsMissed) > 2 || angles.size() == 0)
	{

		if (onGround && (currentTime - lastUpdate > 1.1f))
		{
			ResolvedAngle = lastLBY;
			pCPlayer->PersistentData.LastResolveModeUsed = WD_FuckIt;

			switch ((int)shotsMissed % 9)
			{

			case 1:
				ResolvedAngle += 35;
				break;
			case 3:
				ResolvedAngle -= 35;
				break;
			case 5:
				ResolvedAngle += 20;
				break;
			case 7:
				ResolvedAngle -= 20;
				break;
			}
		}
		else
		{
			if (angles.size() == 0)
			{
				pCPlayer->PersistentData.LastResolveModeUsed = WD_FuckIt;

				switch ((int)shotsMissed % 12)
				{
				case 1:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 2:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 3:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 4:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 5:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 6:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 7:
					ResolvedAngle = oAngles.y - resolverNumber;
					break;
				case 8:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 9:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 10:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 11:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;

				}
			}
			else
			{
				if (shotsMissed % 1)
					pCPlayer->PersistentData.LastResolveModeUsed = WD_FuckIt;

				switch ((int)shotsMissed % 12)
				{
				case 1:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 3:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 5:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 7:
					ResolvedAngle = oAngles.y + resolverNumber;
					break;
				case 9:
					ResolvedAngle = currentTargetD + resolverNumber;
					break;
				case 11:
					ResolvedAngle += 35;
					break;
				case 13:
					ResolvedAngle -= 35;
					break;

				}
			}
		}
	}
	oAngles.y = ResolvedAngle;
}