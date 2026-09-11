#include "mapedit/runtime.hpp"
#include "zCommon.h"
#include "zDebugLog.h"
#include "zHelpParser.h"
#include "zHighScores.h"
#include "zScriptExec_engine.h"
#include "zUserMngr.h"

#if defined(_WIN32)
extern "C" int __cdecl _mkdir(const char*);
#endif

namespace zs1 {
// Retail static initializer ZS1 0x00412070 stores STRING::EMPTY (0x004C9100)
// into the process-global Windows user path before MAP construction.
char* g_WindowsUserPath = STRING::EMPTY;

STRING zUserMngr::BuildUserFileName(int slot,bool hash,int suffixIndex) const
{
    STRING base=BuildUserDataPath();
    STRING result=Printf("%s/user%03d%s",base.m_buf,slot+1,hash ? "_hash" : "");
    if(suffixIndex!=-1) { STRING suffix=Printf("_%02d",suffixIndex); result+=&suffix; }
    result+=".dat";
    return result;
}

STRING zUserMngr::BuildUserDataPath() const
{
    STRING result(g_WindowsUserPath);
    result+=m_rootPath;
#if defined(_WIN32)
    _mkdir(result.m_buf);
#endif
    return result;
}

void BuildUserFileNameBuffer(const zUserMngr* self,char* out,unsigned int outSize,int slot,bool hash,int suffixIndex)
{
    STRING result=self->BuildUserFileName(slot,hash,suffixIndex);
    const unsigned int n=static_cast<unsigned int>(result.Length());
    const unsigned int copy=(outSize && n>=outSize) ? outSize-1u : n;
    if(copy) memcpy(out,result.m_buf,copy);
    if(outSize) out[copy]=0;
}

void BuildUserDataPathBuffer(const zUserMngr* self,char* out,unsigned int outSize)
{
    STRING result=self->BuildUserDataPath();
    const unsigned int n=static_cast<unsigned int>(result.Length());
    const unsigned int copy=(outSize && n>=outSize) ? outSize-1u : n;
    if(copy) memcpy(out,result.m_buf,copy);
    if(outSize) out[copy]=0;
}

STRING zHighScores::BuildRecordsFileName(bool defaultFile) const
{
    if(defaultFile) return STRING("Maps\\_records.dat_default");
    STRING result=g_UserMngr->BuildUserDataPath();
    result+="\\_records.dat";
    return result;
}

void BuildRecordsFileNameBuffer(const zHighScores* self,char* out,unsigned int outSize,bool defaultFile)
{
    STRING result=self->BuildRecordsFileName(defaultFile);
    const unsigned int n=static_cast<unsigned int>(result.Length());
    const unsigned int copy=(outSize && n>=outSize) ? outSize-1u : n;
    if(copy) memcpy(out,result.m_buf,copy);
    if(outSize) out[copy]=0;
}

int ResolveHelpVidMetric(int vidIndex)
{
    return static_cast<int>(Map->Vid(vidIndex)->m_regionTileStepY);
}

// ZS1 MAP startup creates zDebugLog (0x004125D3) and zUserMngr
// (0x004125FD) before the MAP_EDIT constructor returns.  The process-global
// main object (0x004A6B58) is assigned only after MAP_EDIT construction returns
// (WinMain 0x00404CCE), so object creation and main-object binding are kept as
// separate operations here.  High-score/help owners are retained from the V25
// recovered zScript cluster because ScriptExec directly depends on them.
void InitializeScriptSubsystem()
{
    if(!g_DebugLog) g_DebugLog=new zDebugLog();
    if(!g_UserMngr) g_UserMngr=new zUserMngr("userdata");
    if(!g_HighScores) g_HighScores=new zHighScores();
    if(!g_HelpParser) g_HelpParser=new zHelpParser("help");
}

void BindScriptSubsystemMainObject(void* object)
{
    script_engine::g_mainObject=object;
}

void ShutdownScriptSubsystem()
{
    delete g_HelpParser; g_HelpParser=0;
    delete g_HighScores; g_HighScores=0;
    delete g_UserMngr; g_UserMngr=0;
    delete g_DebugLog; g_DebugLog=0;
    script_engine::g_mainObject=0;
}
}
