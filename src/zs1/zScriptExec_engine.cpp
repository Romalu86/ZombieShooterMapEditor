#include "zScriptExec_engine.h"


namespace zs1 { namespace script_engine {

unsigned char g_toggleFlag = 0;
float g_pairX = 1000.0f;
float g_pairY = 1000.0f;
float g_angleRadians = 0.0f;
float g_angleSecond = 0.0f;
int g_moveQueryA = 0;
int g_moveQueryB = 0;
int g_moveQueryC = 0;
int g_moveQueryCount = 0;
unsigned char g_flag49C4AC = 1;
unsigned char g_flag5F0AC8 = 0;
int g_oneShotValue = 0;
void* g_mainObject = 0;
void* g_object1F = 0;

unsigned char GetFlagA()
{
    return g_flag49C4AC;
}

unsigned char GetFlagB()
{
    return g_flag5F0AC8;
}

void ClearFlagB()
{
    g_flag5F0AC8 = 0;
}

void SetOneShotValue(int value)
{
    g_oneShotValue = value;
}

void ToggleFlag()
{
    g_toggleFlag = static_cast<unsigned char>(!g_toggleFlag);
}

unsigned char GetToggleFlag()
{
    return g_toggleFlag;
}

void SetFloatPair(int a, int b)
{
    g_pairX = static_cast<float>(a);
    g_pairY = static_cast<float>(b);
}

void SetAngleState(float degrees, float second)
{
    // ZS1 constants: 0x00492C40 = pi, 0x00492C68 = 1/180.
    g_angleRadians = degrees * 3.1415927410125732421875f * 0.00555555569007992744446f;
    g_angleSecond = second;
}


} } // namespace zs1::script_engine
