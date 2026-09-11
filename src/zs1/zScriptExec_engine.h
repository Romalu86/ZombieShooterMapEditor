#pragma once


namespace zs1 { namespace script_engine {

// Recovery globals corresponding to original fixed-address state used by zScriptExec.
// Names are descriptive; addresses are retained in comments for reccmp ownership.
extern unsigned char g_toggleFlag;       // ZS1 0x005F0AB0
extern float g_pairX;                   // ZS1 0x0049C418
extern float g_pairY;                   // ZS1 0x0049C41C
extern float g_angleRadians;            // ZS1 0x005F0AA8
extern float g_angleSecond;             // ZS1 0x005F0AAC

extern int g_moveQueryA;                // ZS1 0x005F0AB8
extern int g_moveQueryB;                // ZS1 0x005F0ABC
extern int g_moveQueryC;                // ZS1 0x005F0AC0
extern int g_moveQueryCount;            // ZS1 0x005F0AC4
extern unsigned char g_flag49C4AC;       // ZS1 0x0049C4AC
extern unsigned char g_flag5F0AC8;       // ZS1 0x005F0AC8
extern int g_oneShotValue;              // ZS1 0x005F0ACC

extern void* g_mainObject;              // ZS1 global pointer 0x004A6B58
extern void* g_object1F;                // ZS1 global pointer 0x004C90CC

unsigned char GetFlagA();
unsigned char GetFlagB();
void ClearFlagB();
void SetOneShotValue(int value);

void ToggleFlag();
unsigned char GetToggleFlag();
void SetFloatPair(int a, int b);
void SetAngleState(float degrees, float second);


} } // namespace zs1::script_engine
