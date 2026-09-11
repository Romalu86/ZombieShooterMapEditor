#include "mapedit/runtime.hpp"
#include "mapedit/script/logic_layout.hpp"


namespace zs1 { namespace logic {
namespace {

LOGICSTACK_LAYOUT32* StackData(LOGIC_LAYOUT32* logic)
{
    return reinterpret_cast<LOGICSTACK_LAYOUT32*>(static_cast<uintptr_t>(logic->stack.data));
}


static 
void DestroyOwnedString(uint32_t& value)
{
    if (value && value != RetailEmptyStringAddress())
        zs1::engine::RetailOperatorDelete(reinterpret_cast<void*>(static_cast<uintptr_t>(value)));
    value = RetailEmptyStringAddress();
}

void DestroyStackArray(LIST_LOGICSTACK_LAYOUT32* list)
{
    if (!list->data)
        return;
    LOGICSTACK_LAYOUT32* data = reinterpret_cast<LOGICSTACK_LAYOUT32*>(static_cast<uintptr_t>(list->data));
    const uint32_t rawAddress = list->data - 4u;
    const int allocated = *reinterpret_cast<const int*>(static_cast<uintptr_t>(rawAddress));
    for (int i = allocated - 1; i >= 0; --i)
        DestroyOwnedString(data[i].stringPtr);
    zs1::engine::RetailOperatorDelete(reinterpret_cast<void*>(static_cast<uintptr_t>(rawAddress)));
}

void DestroyVariableArray(NAMED_LIST_LOGICVAR_LAYOUT32* list)
{
    if (!list->data)
        return;
    NAMED_LOGICVAR_ENTRY_LAYOUT32* data = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(static_cast<uintptr_t>(list->data));
    const uint32_t rawAddress = list->data - 4u;
    const int allocated = *reinterpret_cast<const int*>(static_cast<uintptr_t>(rawAddress));
    for (int i = allocated - 1; i >= 0; --i) {
        DestroyOwnedString(data[i].var.value.ptr);
        DestroyOwnedString(data[i].name.ptr);
    }
    zs1::engine::RetailOperatorDelete(reinterpret_cast<void*>(static_cast<uintptr_t>(rawAddress)));
}

void DestroyNamedStringArray(NAMED_LIST_STRING_LAYOUT32* list)
{
    if (!list->data)
        return;
    NAMED_STRING_ENTRY_LAYOUT32* data = reinterpret_cast<NAMED_STRING_ENTRY_LAYOUT32*>(static_cast<uintptr_t>(list->data));
    const uint32_t rawAddress = list->data - 4u;
    const int allocated = *reinterpret_cast<const int*>(static_cast<uintptr_t>(rawAddress));
    for (int i = allocated - 1; i >= 0; --i) {
        DestroyOwnedString(data[i].value.ptr);
        DestroyOwnedString(data[i].name.ptr);
    }
    zs1::engine::RetailOperatorDelete(reinterpret_cast<void*>(static_cast<uintptr_t>(rawAddress)));
}

void DeleteRaw(uint32_t& address)
{
    if (address)
        zs1::engine::RetailOperatorDelete(reinterpret_cast<void*>(static_cast<uintptr_t>(address)));
    address = 0;
}

const char* StringChars(const STRING_LAYOUT32& value)
{
    return reinterpret_cast<const char*>(static_cast<uintptr_t>(value.ptr));
}

void InitStackValue(LOGICSTACK_LAYOUT32* value)
{
    new (value) LOGICSTACK_LAYOUT32();
}

void CopyStackValue(LOGICSTACK_LAYOUT32* dst, const LOGICSTACK_LAYOUT32* src)
{
    *dst = *src;
}

LOGICSTACK_LAYOUT32* AllocateStackArray(int count)
{
    const size_t bytes = 4u + static_cast<size_t>(count) * sizeof(LOGICSTACK_LAYOUT32);
    uint8_t* raw = static_cast<uint8_t*>(zs1::engine::RetailOperatorNew(bytes));
    if (!raw)
        return nullptr;
    *reinterpret_cast<int*>(raw) = count;
    LOGICSTACK_LAYOUT32* data = reinterpret_cast<LOGICSTACK_LAYOUT32*>(raw + 4);
    for (int i = 0; i < count; ++i)
        InitStackValue(data + i);
    return data;
}

void InitVariableEntry(NAMED_LOGICVAR_ENTRY_LAYOUT32* entry)
{
    new (entry) NAMED_LOGICVAR_ENTRY_LAYOUT32();
}

void CopyVariableEntry(NAMED_LOGICVAR_ENTRY_LAYOUT32* dst, const NAMED_LOGICVAR_ENTRY_LAYOUT32* src)
{
    *dst = *src;
}

NAMED_LOGICVAR_ENTRY_LAYOUT32* AllocateVariableArray(int count)
{
    const size_t bytes = 4u + static_cast<size_t>(count) * sizeof(NAMED_LOGICVAR_ENTRY_LAYOUT32);
    uint8_t* raw = static_cast<uint8_t*>(zs1::engine::RetailOperatorNew(bytes));
    if (!raw)
        return nullptr;
    *reinterpret_cast<int*>(raw) = count;
    NAMED_LOGICVAR_ENTRY_LAYOUT32* data = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(raw + 4);
    for (int i = 0; i < count; ++i)
        InitVariableEntry(data + i);
    return data;
}

void EnsureStackCapacity128(LOGIC_LAYOUT32* logic)
{
    if (logic->stack.capacity >= 128)
        return;
    LOGICSTACK_LAYOUT32* oldData = StackData(logic);
    const int oldCapacity = logic->stack.capacity;
    LOGICSTACK_LAYOUT32* newData = AllocateStackArray(128);
    logic->stack.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(newData));
    if (!newData)
        zs1::engine::ReportListOutOfMemory(128);
    if (oldData) {
        for (int i = 0; i < oldCapacity; ++i)
            CopyStackValue(newData + i, oldData + i);
        LIST_LOGICSTACK_LAYOUT32 oldList = { 0 };
        oldList.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(oldData));
        DestroyStackArray(&oldList);
    }
    logic->stack.capacity = 128;
}

void EnsureVariableCapacity128(LOGIC_LAYOUT32* logic)
{
    if (logic->variables.capacity >= 128)
        return;
    NAMED_LOGICVAR_ENTRY_LAYOUT32* oldData = reinterpret_cast<NAMED_LOGICVAR_ENTRY_LAYOUT32*>(
        static_cast<uintptr_t>(logic->variables.data));
    const int oldCapacity = logic->variables.capacity;
    NAMED_LOGICVAR_ENTRY_LAYOUT32* newData = AllocateVariableArray(128);
    logic->variables.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(newData));
    if (!newData)
        zs1::engine::ReportListOutOfMemory(128);
    if (oldData) {
        for (int i = 0; i < oldCapacity; ++i)
            CopyVariableEntry(newData + i, oldData + i);
        NAMED_LIST_LOGICVAR_LAYOUT32 oldList = { 0 };
        oldList.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(oldData));
        DestroyVariableArray(&oldList);
    }
    logic->variables.capacity = 128;
}

void ReleaseNamedStrings(LOGIC_LAYOUT32* logic)
{
    logic->strings.capacity = 0;
    logic->strings.count = 0;
    DestroyNamedStringArray(&logic->strings);
    logic->strings.data = 0;
}

bool ResolveActionSlot(LOGIC_LAYOUT32* logic, int actionNo, int* stackIndex)
{
    // Retail performs an unsigned compare against 0x100, so negative values fail too.
    if (static_cast<unsigned int>(actionNo) >= 0x100u)
        return false;
    const int index = logic->actionN[actionNo];
    if (static_cast<unsigned int>(index) >= static_cast<unsigned int>(logic->stack.count))
        return false;
    *stackIndex = index;
    return true;
}

} // namespace


// End of internal ZS1 layout helpers.  The large runtime owners below must be
// emitted under the real retail C++ symbols, not LOGIC_LAYOUT32 trampolines.
} } // namespace zs1::logic

void LOGIC::Release()
{
    currentSymbol=0;
    if(ownedBuffer38) { ::operator delete(ownedBuffer38); ownedBuffer38=0; }
    if(byteCode) { ::operator delete(byteCode); byteCode=0; }
    if(sourceBuffer) { ::operator delete(sourceBuffer); sourceBuffer=0; }

    // Keep the source shape that makes VC6 emit the retail array-cookie teardown
    // directly in this owner instead of routing through reconstruction helpers.
    stack.m_max=0;
    stack.m_no=0;
    if(stack.m_data) delete[] stack.m_data;
    stack.m_data=0;

    variables.m_max=0;
    variables.m_no=0;
    if(variables.m_data) delete[] variables.m_data;
    variables.m_data=0;

    strings.m_max=0;
    strings.m_no=0;
    if(strings.m_data) delete[] strings.m_data;
    strings.m_data=0;

    byteCodeSize=0;
    parserFlags54=0;
    pos=0;
    parserMode=0;
    compileError=0;
    parseContext=0;
    mainFunction=-1;

    for(int i=0;i<256;++i) actionN[i]=-1;
    { /* VC6 for-scope */ for(int i=0;i<256;++i) scriptEventFunction[i]=-1; } /* VC6 for-scope */

    // Target intentionally leaves end (+0x48), line (+0x50), and runtime scratch
    // untouched here.
}

int LOGIC::LoadLGC(const STRING* inputName)
{
    const char* filename=inputName->m_buf;
    FILE* file=*filename ? static_cast<FILE*>(fopen(filename,"rb")) : 0;

    Release();
    name=inputName;

    if(!file) {
        STRING empty;
        Error(7,&empty,0);
        return 1;
    }

#if defined(_MSC_VER) && _MSC_VER <= 1200
    // Retail reads FILE::_file directly at +0x10 before _filelength.
    const int fileSize=static_cast<int>(_filelength(reinterpret_cast<int*>(file)[4]));
#else
    const int fileSize=static_cast<int>(_filelength(_fileno(file)));
#endif

    byteCode=static_cast<unsigned char*>(::operator new(256000u));
    if(!byteCode) {
        STRING data("data");
        Error(2,&data,0);
        exit(1);
    }

    sourceBuffer=static_cast<char*>(::operator new(static_cast<unsigned int>(fileSize+0x1014)));
    if(!sourceBuffer) {
        STRING ini("ini");
        Error(2,&ini,0);
        exit(1);
    }

    pos=sourceBuffer+0x0FE2;
    end=pos+fileSize;
    fread(pos,1,static_cast<size_t>(fileSize),file);

    // Target 0x43193F..0x431A78: these two fixed-capacity expansions are
    // template bodies inlined by VC6 in LOGIC::LoadLGC.  Keep them physically
    // in this owner; an out-of-line Expand call shortens .text and changes COMDAT order.
    if(stack.m_max<128) {
        LOGICSTACK* oldData=stack.m_data;
        LOGICSTACK* fresh=new LOGICSTACK[128];
        stack.m_data=fresh;
        if(!fresh)
            MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",128);
        if(oldData) {
            for(int i=0;i<stack.m_max;++i) fresh[i]=oldData[i];
            delete[] oldData;
        }
        stack.m_max=128;
    }
    if(variables.m_max<128) {
        typedef NAMED_LIST_STRUCT<LOGICVAR> VAR_ITEM;
        VAR_ITEM* oldData=variables.m_data;
        VAR_ITEM* fresh=new VAR_ITEM[128];
        variables.m_data=fresh;
        if(!fresh)
            MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",128);
        if(oldData) {
            for(int i=0;i<variables.m_max;++i) fresh[i]=oldData[i];
            delete[] oldData;
        }
        variables.m_max=128;
    }

    line=0;
    // Retail calls the compiler owner once and loops only while that result is
    // zero and compileError remains clear.
    int compileResult=func();
    while(!compileResult && !compileError)
        compileResult=func();

    MYERROR::Log(::Error,"LoadScript::ByteCode=%i varNo=%i DefineNo=%i stackNo=%i",byteCodeSize,variables.m_no,strings.m_no,stack.m_no);

    strings.m_max=0;
    strings.m_no=0;
    if(strings.m_data) delete[] strings.m_data;
    strings.m_data=0;

    if(byteCodeSize) {
        if(byteCodeSize>256000) {
            STRING sizeText("byte code size");
            Error(2,&sizeText,byteCodeSize);
        }
        unsigned char* compact=static_cast<unsigned char*>(
            ::operator new(static_cast<unsigned int>(byteCodeSize)));
        if(!compact) {
            STRING tmp("tmp");
            Error(2,&tmp,0);
            exit(1);
        }
        memcpy(compact,byteCode,static_cast<size_t>(byteCodeSize));
        ::operator delete(byteCode);
        byteCode=compact;
    } else {
        Release();
    }

    if(sourceBuffer) ::operator delete(sourceBuffer);
    sourceBuffer=0;

    if(compileError) {
        Release();
        fclose(file);
        return 1;
    }

    for(int i=0;i<variables.m_no;++i) {
        const LOGICVAR& var=variables.m_data[i].val;
        if(var.flag==3 && (var.auxFlags&1) && !(var.auxFlags&2))
            MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: function %s() not return value",variables.m_data[i].name.m_buf);
    }

    fclose(file);
    return 0;
}

// script runtime translation unit as Release/LoadLGC.  Keep this physical
// ownership because VC6 object/COMDAT order is part of whole-EXE identity.

// Binary script loader.  Keep target ordering: Release/name assignment happens
// before the open-error path, and only the high bits of the first dword select
// the textual compiler.
int LOGIC::Load(const STRING* scriptName) {
    int stackCount=stack.No();
    FSTREAM file(scriptName,"rb");

    Release();
    name=scriptName;

    if(!file.IsOpen()) {
        STRING empty;
        Error(7,&empty,0);
        return 1;
    }

    file.Read(&stackCount,4u);
    if(stackCount & static_cast<int>(0xFF000000u))
        return LoadLGC(&name);

    // Target 0x431E3B..0x431ED7 inlines LIST<LOGICSTACK>::Expand(128).
    if(stack.m_max<128) {
        LOGICSTACK* oldData=stack.m_data;
        LOGICSTACK* fresh=new LOGICSTACK[128];
        stack.m_data=fresh;
        if(!fresh)
            MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",128);
        if(oldData) {
            for(int i=0;i<stack.m_max;++i) fresh[i]=oldData[i];
            delete[] oldData;
        }
        stack.m_max=128;
    }
    // SetNo itself is also inlined here in retail; only the dynamic overflow
    // Expand remains an out-of-line call.
    stack.m_no=stackCount;
    if(stack.m_no>stack.m_max)
        stack.Expand(stack.m_no);
    LoadVar(&file);

    file.Read(&byteCodeSize,4u);
    byteCode=static_cast<unsigned char*>(::operator new(static_cast<unsigned int>(byteCodeSize)));
    if(!byteCode) {
        STRING data2("data2");
        Error(2,&data2,0);
        exit(1);
    }
    file.Read(byteCode,static_cast<unsigned int>(byteCodeSize));

    for(int i=0;i<variables.No();++i)
        if(*variables.Name(i)=="main") mainFunction=i;
    return 0;
}

void LOGIC::LoadVar(STREAM* file) {
    // each name retail reads the complete 0x14-byte LOGICVAR, including the
    // stale persisted STRING pointer.  The second pass replaces that pointer
    // with STRING::EMPTY before reading the actual string payload.
    for(int i=0;i<stack.m_no;++i) stack.m_data[i].Read(file);
    int count=0;
    file->Read(&count,4u);
    variables.Expand(count);
    variables.m_no=count;
    { /* VC6 for-scope */ for(int i=0;i<count;++i) {
        variables.m_data[i].name.Read(file);
        file->Read(&variables.m_data[i].val,0x14u);
    } } /* VC6 for-scope */
    { /* VC6 for-scope */ for(int i=0;i<count;++i) {
        variables.m_data[i].val.value.m_buf=STRING::EMPTY;
        variables.m_data[i].val.value.Read(file);
    } } /* VC6 for-scope */
}

// Retail LOGIC::Error logs the primary diagnostic unconditionally, but sets
// compileError and emits the two 60-byte context lines only when source pos exists.
void LOGIC::Error(int type,const STRING* text,int err) {
    MYERROR::Error(::Error,"SCRIPT '%s' line %i",type,
                   const_cast<char*>(text->m_buf),
                   static_cast<unsigned long>(err),name.m_buf,line+1);
    if(!pos) return;

    compileError=1;
    char context[61];
    for(int i=0;i<60;++i) {
        const char c=pos[i-30];
        context[i]=(c=='\n' || c=='\r' || c=='\t') ? '?' : c;
    }
    context[60]=0;
    MYERROR::Error(::Error,"SCRIPT",10,context,0);

    { /* VC6 for-scope */ for(int i=0;i<60;++i) context[i]=(i==30) ? '^' : ' '; } /* VC6 for-scope */
    context[60]=0;
    MYERROR::Error(::Error,"SCRIPT",10,context,0);
}


namespace zs1 { namespace logic {

// Synthetic LOGIC_LAYOUT32 action owners retired in P86; real LOGIC owns 0x437D90/0x437E00.

} } // namespace zs1::logic
