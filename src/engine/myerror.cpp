#include "mapedit/runtime.hpp"
#include <stdarg.h>

extern "C" int __cdecl vsprintf(char*, const char*, va_list);
extern "C" int __cdecl remove(const char*);
extern "C" int __cdecl rename(const char*,const char*);
extern "C" char* _pgmptr;

namespace {
inline void* ErrorFile(const MYERROR* self)
{
    return self->file;
}

inline HWND__*& ErrorWindow(MYERROR* self)
{
    return self->hwnd;
}
}

// Writes a temporary diagnostic line and restores the previous file position.
void __cdecl MYERROR::LogTmp(const MYERROR* self,const char* text,...)
{
    if (!text || !ErrorFile(self))
        return;

    char buffer[1024];
    va_list args;
    va_start(args,text);
    vsprintf(buffer,text,args);
    va_end(args);

    void* file=ErrorFile(self);
    const long pos=ftell(file);
    fputs(buffer,file);
    fputs("\n",file);
    fflush(file);
    fseek(file,pos,0);
}

// ZS1 retail 0x00411DB0..0x00411E0F: vsprintf into 1024 bytes, fputs text/newline, fflush.
void __cdecl MYERROR::Log(const MYERROR* self,const char* text,...)
{
    if (!text || !ErrorFile(self))
        return;

    char buffer[1024];
    va_list args;
    va_start(args,text);
    vsprintf(buffer,text,args);
    va_end(args);

    void* file=ErrorFile(self);
    fputs(buffer,file);
    fputs("\n",file);
    fflush(file);
}

int __cdecl MYERROR::Window(const char* text,...)
{
    if (!text)
        return 0;

    char buffer[1024];
    va_list args;
    va_start(args,text);
    vsprintf(buffer,text,args);
    va_end(args);

    void* file=ErrorFile(this);
    if (file) {
        fputs(buffer,file);
        fputs("\n",file);
        fflush(file);
    }

    if (!ErrorWindow(this))
        ErrorWindow(this)=GetForegroundWindow();
    return MessageBoxA(ErrorWindow(this),buffer,"Error",0);
}

extern "C" char* __cdecl strcat(char*,const char*);

// Retail always returns EXCEPTION_EXECUTE_HANDLER (1). Unknown exception codes
// are accepted silently; known codes emit the exact retail diagnostic text.
long MYERROR::FilterExcept(_EXCEPTION_RECORD* info)
{
    const unsigned long code=info->ExceptionCode;
    const unsigned long address=reinterpret_cast<unsigned long>(info->ExceptionAddress);

    switch (code) {
    case 0xC0000005u: // EXCEPTION_ACCESS_VIOLATION
        if (info->ExceptionInformation[0]!=0) {
            Log(this,"!!!ERROR EXCEPTION 0x%X!!!: Access violation write to 0x%X",
                address,info->ExceptionInformation[1]);
        } else {
            Log(this,"!!!ERROR EXCEPTION 0x%X!!!: Access violation read from 0x%X",
                address,info->ExceptionInformation[1]);
        }
        break;
    case 0xC000008Cu:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_ARRAY_BOUNDS_EXCEEDED",address); break;
    case 0x80000003u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_BREAKPOINT",address); break;
    case 0x80000002u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_DATATYPE_MISALIGNMENT",address); break;
    case 0xC000008Du:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_DENORMAL_OPERAND",address); break;
    case 0xC000008Eu:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!:FLT divide by zero",address); break;
    case 0xC000008Fu:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_INEXACT_RESULT",address); break;
    case 0xC0000090u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_INVALID_OPERATION",address); break;
    case 0xC0000091u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_OVERFLOW",address); break;
    case 0xC0000092u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_STACK_CHECK",address); break;
    case 0xC0000093u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_FLT_UNDERFLOW",address); break;
    case 0xC000001Du:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_ILLEGAL_INSTRUCTION",address); break;
    case 0xC0000006u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_IN_PAGE_ERROR",address); break;
    case 0xC0000094u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!:INT divide by zero",address); break;
    case 0xC0000095u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_INT_OVERFLOW",address); break;
    case 0xC0000026u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_INVALID_DISPOSITION",address); break;
    case 0xC0000025u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_NONCONTINUABLE_EXCEPTION",address); break;
    case 0xC0000096u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_PRIV_INSTRUCTION",address); break;
    case 0x80000004u:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_SINGLE_STEP",address); break;
    case 0xC00000FDu:
        Log(this,"!!!ERROR EXCEPTION 0x%X!!!: EXCEPTION_STACK_OVERFLOW",address); break;
    default:
        break;
    }
    return 1;
}

// Canonical MapEditor formats this prefix through GetTimeString().
// ZS1 retail whole-owner proof: timestamp prefix, module varargs, 15-way error-format switch, Log tail.
void __cdecl MYERROR::Error(const MYERROR* self,const char* module,int type,
                            const char* text,unsigned long err,...)
{
    char str[1024];
    STRING timeText=GetTimeString();
    sprintf(str,"!!!ERROR %s!!!",timeText.CharPtr());

    va_list args;
    va_start(args,err);
    vsprintf(str+strlen(str),module,args);
    va_end(args);

    strcat(str,": ");
    // Retail does not index the format strings in source-order.  The VC6
    // switch jump-table at MapEdit.exe 0x004145BF/0x0041474E maps the
    // TYPE_ERROR numeric values to these cases in this exact order.
    static const char* const formats[15]={
        "0x%X Couldn't lock %s",             // 0  E_LOCK
        "0x%X Couldn't copy %s",             // 1  E_COPY
        "%i There was not enough memory for %s", // 2  E_MEMORY
        "0x%X Couldn't create the %s",       // 3  E_CREATE
        "0x%X Invalid %s",                   // 4  E_INVALID
        "0x%X Load %s",                      // 5  E_LOAD
        "0x%X Save %s",                      // 6  E_SAVE
        "0x%X Couldn't open '%s'",           // 7  E_OPEN
        "0x%X Couldn't set the %s",          // 8  E_SET
        "0x%X Couldn't get the %s",          // 9  E_GET
        "%i %s",                             // 10 E_ERROR
        "0x%X Section can't found (%s)",     // 11 E_SECTION
        "0x%X Unable initialize %s",         // 12 E_INIT
        "%i Missing %s",                     // 13 E_MISSING
        "%i Unknownn %s"                     // 14 E_UNKNOWN
    };
    if (type>=0 && type<15)
        strcat(str,formats[type]);

    Log(self,str,err,text);
}

void __cdecl MYERROR::LogExit(const MYERROR* self,const char* text,...)
{
    char buffer[1024];
    va_list args;
    va_start(args,text);
    vsprintf(buffer,text,args);
    va_end(args);
    Log(self,buffer);

    MYERROR* mutableSelf=const_cast<MYERROR*>(self);
    if (mutableSelf->file)
        fclose(mutableSelf->file);
    mutableSelf->file=0;
    exit(1);
}

// ZS1 allocates 0x80C bytes: hwnd +4, FILE* +8, active filename +0x0C,
// caller-supplied log base path +0x40C.  Preserve those offsets exactly; the
// active file still falls back to the current directory when the Logs path fails.
MYERROR::MYERROR(const char* log_path,int clear_log_file)
{
    // ZS1 0x00411590 allocates 0x80C bytes and stores the caller-supplied
    // base path verbatim at +0x40C.  The active filename remains at +0x0C.
    hwnd=0;
    const char* base=log_path ? log_path : "";
    strcpy(logPath,base);

    STRING logName(base);
    if (clear_log_file) {
        logName+="\\error";
        STRING date=GetDateString();
        STRING time=GetTimeString();
        logName+=&date;
        logName+=" ";
        logName+=&time;
        logName+=".log";
        logName.Replace(":","h");
        logName.Replace(":","m");
    } else {
        logName+="\\error.log";
    }

    const char* mode=clear_log_file ? "wt" : "at";
    file=FOpen(&logName,mode);
    if (!file) {
        // Retail fallback strips the last Logs\ component and retries in CWD.
        STRING fallback=logName.After("Logs\\");
        file=FOpen(&fallback,mode);
        logName=fallback;
    }
    strcpy(filename,logName.CharPtr());

    STRING executable(_pgmptr ? _pgmptr : "");
    STRING executableTime=FGetTime(&executable);
    STRING date=GetDateString();
    STRING time=GetTimeString();
    Log(this,"----< %s %s >----< %s (%s) >----",
        date.CharPtr(),time.CharPtr(),executable.CharPtr(),executableTime.CharPtr());
}

// Retail closes the active timestamped file and promotes it to error.log.
// If the log was already opened as logs\\error.log/error.log (append mode),
// no rename is performed.
MYERROR::~MYERROR()
{
    if (file)
        fclose(file);
    file=0;

    // ZS1 0x00411950 checks whether the active file is already error.log.
    STRING active(filename);
    STRING afterLogs=active.After("Logs\\");
    STRING basename=active.AfterLast("\\");
    if (afterLogs=="error.log" || basename=="error.log")
        return;

    STRING finalName(logPath);
    finalName+="\\error.log";
    FRemove(&finalName);
    if (FRename(&active,&finalName)!=0) {
        STRING fallback("error.log");
        FRemove(&fallback);
        FRename(&active,&fallback);
    }
}

void __cdecl MYERROR::WindowExit(const char* text,...)
{
    char buffer[1024];
    va_list args;
    va_start(args,text);
    vsprintf(buffer,text,args);
    va_end(args);
    Window(buffer);
    exit(1);
}
