#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"
#include "mapedit/zs1/script_exec.hpp"

namespace {
#if defined(_MSC_VER) && _MSC_VER <= 1200
// VC6 FILE::_file is at +0x10.  Do not include <stdio.h> here: this project
// intentionally carries raw CRT declarations in win32_abi.hpp, and mixing the
// two declaration surfaces produces duplicate-C-linkage diagnostics in VC6.
// The overlay below models only the prefix the retail instruction actually reads.
struct RETAIL_FILE_PREFIX32 {
    uint32_t ptr;      // +0x00 FILE::_ptr
    int cnt;           // +0x04 FILE::_cnt
    uint32_t base;     // +0x08 FILE::_base
    int flag;          // +0x0C FILE::_flag
    int file;          // +0x10 FILE::_file
};
#endif
inline zs1::logic::LOGIC_LAYOUT32* Z(LOGIC* p) {
    return reinterpret_cast<zs1::logic::LOGIC_LAYOUT32*>(p);
}
inline const zs1::logic::LOGIC_LAYOUT32* Z(const LOGIC* p) {
    return reinterpret_cast<const zs1::logic::LOGIC_LAYOUT32*>(p);
}
inline zs1::logic::LOGICSTACK_LAYOUT32* Z(LOGICSTACK* p) {
    return reinterpret_cast<zs1::logic::LOGICSTACK_LAYOUT32*>(p);
}
inline const zs1::logic::LOGICSTACK_LAYOUT32* Z(const LOGICSTACK* p) {
    return reinterpret_cast<const zs1::logic::LOGICSTACK_LAYOUT32*>(p);
}
}

namespace zs1 { namespace engine {
} }

namespace zs1 { namespace logic {
int RetailFileLength(void* stream) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
    // ZS1 0x0043182E: mov edx,[edi+10h] / push edx / call _filelength.
    const RETAIL_FILE_PREFIX32* f=reinterpret_cast<const RETAIL_FILE_PREFIX32*>(stream);
    return static_cast<int>(_filelength(f->file));
#else
    // Modern CRT hides FILE internals; this lane is build-only and preserves the
    // same descriptor operation through the public CRT accessor.
    FILE* f=static_cast<FILE*>(stream);
    return static_cast<int>(_filelength(_fileno(f)));
#endif
}

void InvokeScriptOpcode(unsigned opcode) {
    ScriptExecFunc(static_cast<int>(opcode & 0xFFu));
}
void RetailStreamRead(void* stream,void* dst,int bytes) {
    static_cast<STREAM*>(stream)->Read(dst,static_cast<unsigned int>(bytes));
}
void RetailStringReadRes(STRING_LAYOUT32* value,void* stream) {
    reinterpret_cast<STRING*>(value)->Read(static_cast<STREAM*>(stream));
}
char g_RetailLogicErrorText[1024]={0};
} }

// Active ABI wrappers ---------------------------------------------------------
// Target 0x00431C80 / 0x00439550 / 0x00439570 / 0x004497E0.
// The three bytes after type are compiler padding: retail never initializes or copies them.
LOGICSTACK::LOGICSTACK():type(0),number(0),string() {}
// Target PushStr @ 0x004498C0 materializes this value with type=1 and leaves
// the inactive numeric payload untouched.  The input is a reference in retail.
LOGICSTACK::~LOGICSTACK() {}
// LOGICSTACK assignment/accessor/String/Inc/Dec source bodies are header-inline
// in script/logic.hpp.  This is required for the mixed target codegen where
// small callers inline them while larger loops retain standalone COMDAT owners.

// Retail keeps Int() as a real out-of-line owner.  CallFunction, ExecFunc and
// MAP script wrappers call this symbol directly; unlike the tiny Is*/String
// accessors it must therefore have one guaranteed emitted definition.
int LOGICSTACK::Int()
{
    if(type&1) {
        const char* text=string.m_buf;
        if(!isdigit(text[0]) && (text[0]!='-' || !isdigit(text[1])))
            MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: \xEF\xF0\xE8\xF1\xE2\xE0\xE8\xE2\xE0\xED\xE8\xE5 \xEF\xE5\xF0\xE5\xEC\xE5\xED\xED\xEE\xE9 int \xF1\xF2\xF0\xEE\xEA\xE8 '%s'",text);
        if(text[1]!='x')
            return atoi(text);
        int value;
        sscanf(text,"%i",&value);
        return value;
    }
    return number;
}

void LOGICSTACK::BinarOperator(int operation,const LOGICSTACK& r) {
    if((type&1) && (r.type&1)) {
        // Retail 0x004396AF uses a dense 8..20 switch table here.  Keeping
        // these string-special operations as if/else changes VC6 control-flow
        // ownership and also misses the direct STRING::operator+=(STRING*) call.
        switch(operation) {
        case 8:
            string+=&r.string;
            type=1;
            return;
        case 9: {
            // Retail string subtraction removes/replaces the RHS substring
            // using the shared script text buffer as the replacement value.
            // The temporary STRING lifetime and inline Replace wrapper are
            // visible directly in target 0x004396DF..0x00439764.
            STRING replacement(zs1::logic::g_RetailLogicErrorText);
            string.Replace(r.string,replacement);
            type=1;
            return;
        }
        case 13:
            number=!strcmp(string.m_buf,r.string.m_buf);
            type=2;
            return;
        case 20:
            number=strcmp(string.m_buf,r.string.m_buf)!=0;
            type=2;
            return;
        }
    }

    // Target 0x00439815 calls LOGICSTACK::Int() on the RHS before converting
    // the left operand.  Besides the shorter source shape this preserves the
    // retail invalid-int diagnostic path owned by Int().
    int rhs=const_cast<LOGICSTACK&>(r).Int();

    if(type&1) {
        if(string.m_buf[1]!='x') number=atoi(string.m_buf);
        else { number=0; sscanf(string.m_buf,"%i",&number); }
    }

    switch(operation) {
    case 8:  number+=rhs; break;
    case 9:  number-=rhs; break;
    case 19: number*=rhs; break;
    case 6:  number=rhs ? number/rhs : 0x0FFFFFFF; break;
    case 7:  number%=rhs; break;
    case 11: number|=rhs; break;
    case 10: number^=rhs; break;
    case 12: number&=rhs; break;
    case 23: number<<=rhs; break;
    case 22: number>>=rhs; break;
    case 21: number=(number&&rhs)?1:0; break;
    case 14: number=(number||rhs)?1:0; break;
    case 16: number=number<rhs; break;
    case 18: number=number<=rhs; break;
    case 15: number=number>rhs; break;
    case 17: number=number>=rhs; break;
    case 13: number=number==rhs; break;
    case 20: number=number!=rhs; break;
    default: MYERROR::Log(::Error,"!!!ERROE!!!SCRIPT::Unknown Binary command %i",operation); break;
    }
    type=2;
}

void LOGICSTACK::Read(STREAM* file) {
    file->Read(&type,1u);
    if(!(type&8)) return;
    if(type&1) {
        string.Read(file);
        for(int i=string.Length();i!=0;--i) string.m_buf[i-1]^=0x17;
    } else file->Read(&number,4u);
}

LOGICVAR::LOGICVAR():flag(0),auxFlags(0),value(),a(0),type(0),extra(0) { pad02[0]=pad02[1]=0; }
LOGICVAR::LOGICVAR(int kind,int v):flag(static_cast<uint8_t>(kind)),auxFlags(0),value(),a(v),type(0),extra(0) { pad02[0]=pad02[1]=0; }
LOGICVAR::~LOGICVAR() {}
LOGICVAR& LOGICVAR::operator=(const LOGICVAR& r) { if(this!=&r){flag=r.flag;auxFlags=r.auxFlags;pad02[0]=r.pad02[0];pad02[1]=r.pad02[1];value=r.value;a=r.a;type=r.type;extra=r.extra;}return *this; }

LOGIC::LOGIC():stack(),variables(),strings(),currentSymbol(0),name(),ownedBuffer38(0),byteCode(0),byteCodeSize(0),pos(0),end(0),sourceBuffer(0),line(0),parserFlags54(0),mainFunction(-1),parserMode(0),runtimeScratch860(0),runtimeScratch864(0),compileError(0),parseContext(0) {
    for(int i=0;i<256;++i){actionN[i]=-1;scriptEventFunction[i]=-1;}
}
LOGIC::~LOGIC() { Release(); }
int LOGIC::CallFunction(int function,const void* a,const void* b,int c) { return CallFunction(function,"ppi",a,b,c); }
// ZS1 0x004498C0 / 0x004499E0 / 0x00449AD0 are header-inline owners.
// Their canonical source bodies live in script/logic.hpp so VC6 can reproduce
// both the standalone COMDAT and the target inlined call-site shapes.
int LOGIC::PopInt() {
    if(stack.m_no<=0) return 0;
    return stack.m_data[--stack.m_no].Int();
}
const STRING* LOGIC::PopStr() {
    if(stack.m_no<=0) return &name;
    return stack.m_data[--stack.m_no].String();
}
int LOGIC::PopObject() {
    if(stack.m_no<=0) return 0;
    LOGICSTACK& v=stack.m_data[--stack.m_no];
    return v.number;
}
int LOGIC::IsLastStackString() {
    return stack.m_data[stack.m_no-1].type&1;
}
int LOGIC::DeletePointerToObject(void* object) {
    int result=0;
    const int target=static_cast<int>(reinterpret_cast<unsigned int>(object));
    for(int i=0;i<stack.m_no;++i) {
        LOGICSTACK& value=stack.m_data[i];
        if((value.type&0x10) && value.number==target) {
            value.number=0;
            value.type=static_cast<uint8_t>(value.type&~0x10u);
            ++result;
        }
    }
    return result;
}
int LOGIC::GetActionN(int actionNo) {
    if(compileError) return 0;
    if(static_cast<unsigned int>(actionNo)<0x100u) {
        const int index=actionN[actionNo];
        if(static_cast<unsigned int>(index)<static_cast<unsigned int>(stack.m_no))
            return stack.m_data[index].Int();
    }
    STRING text=Printf("variable for Get Action%i",actionNo);
    Error(13,&text,0);
    return 0;
}
void LOGIC::SetActionN(int actionNo,int value) {
    if(compileError) return;
    if(static_cast<unsigned int>(actionNo)<0x100u) {
        const int index=actionN[actionNo];
        if(static_cast<unsigned int>(index)<static_cast<unsigned int>(stack.m_no)) {
            stack.m_data[index].number=value;
            stack.m_data[index].type=2;
            return;
        }
    }
    STRING text=Printf("variable for Set Action%i",actionNo);
    Error(13,&text,0);
}

STRING LOGIC::GetVariableStr(const STRING* varName) {
    STRING baseName=varName->Before("[");
    int variable=-1;
    for(int i=variables.m_no-1;i>=0;--i) {
        if(variables.m_data[i].name==&baseName) { variable=i; break; }
    }

    if(variable<0 || variables.m_data[variable].val.flag!=1) {
        MYERROR::Log(::Error,"!!!ERROR!!! SCRIPT Can't find variable '%s' in GetVariableString",varName->m_buf);
        return STRING(zs1::logic::g_RetailLogicErrorText);
    }

    STRING indexText=varName->After("[");
    const int index=indexText.Int();
    LOGICVAR& entry=variables.m_data[variable].val;
    if(index>=entry.extra)
        return STRING(zs1::logic::g_RetailLogicErrorText);

    return STRING(stack.m_data[entry.a+index].String());
}
const NAMED_LIST<LOGICVAR>* LOGIC::Var() { return &variables; }
void LOGIC::SaveVar(STREAM* file) {
    for(int i=0;i<stack.m_no;++i) {
        // Persist the same tag/value representation as target Read.
        const LOGICSTACK& v=stack.m_data[i];
        file->Write(&v.type,1u);
        if(v.type&0x08) {
            if(v.type&1) {
                STRING encrypted(v.string);
                for(int n=0;n<encrypted.Length();++n) encrypted.m_buf[n]^=0x17;
                encrypted.Write(file);
            } else file->Write(&v.number,4u);
        }
    }
    file->Write(&variables.m_no,4u);
    { /* VC6 for-scope */ for(int i=0;i<variables.m_no;++i) {
        variables.m_data[i].name.Write(file);
        const LOGICVAR& v=variables.m_data[i].val;
        uint8_t head[4]={v.flag,v.auxFlags,v.pad02[0],v.pad02[1]};
        file->Write(head,4u); int stale=0; file->Write(&stale,4u);
        file->Write(&v.a,4u);file->Write(&v.type,4u);file->Write(&v.extra,4u);
    } } /* VC6 for-scope */
    { /* VC6 for-scope */ for(int i=0;i<variables.m_no;++i) variables.m_data[i].val.value.Write(file); } /* VC6 for-scope */
}
int LOGIC::Save() {
    STRING path=name.BeforeLast(".")+".lgd";
    FSTREAM file(&path,"wb");
    if(!file.IsOpen()) return 1;
    const int count=stack.m_no; file.Write(&count,4u); SaveVar(&file);
    file.Write(&byteCodeSize,4u); if(byteCodeSize) file.Write(byteCode,static_cast<unsigned int>(byteCodeSize));
    return 0;
}
