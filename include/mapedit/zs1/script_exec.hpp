#pragma once

// Retail ZS1 global script opcode boundary @ 0x00444110.
int ScriptExecFunc(int command);
namespace zs1 {
const char* ScriptExecDispatch(int command,int arg1,int arg2,const char* str1,const char* str2,int* intResult,void** objectResult);
void InitializeScriptSubsystem();
void BindScriptSubsystemMainObject(void* object);
void ShutdownScriptSubsystem();
}
