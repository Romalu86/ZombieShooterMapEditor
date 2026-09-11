#pragma once



namespace zs1 { namespace engine {
// them into the retail parser/compiler owners instead of emitting fake calls.
inline void* RetailOperatorNew(size_t bytes) { return ::operator new(bytes); }
inline void RetailOperatorDelete(void* p) { ::operator delete(p); }
inline void ReportListOutOfMemory(int requestedCapacity) {
    MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",requestedCapacity);
}
} }

inline void* operator new(size_t, void* p) { return p; }
inline void operator delete(void*, void*) {}

namespace zs1 { namespace logic {
typedef unsigned int uintptr_t;
typedef int int32_t;


// Reconstruction relocation bridge: retail used absolute 0x004C9100 for the
// canonical empty STRING.  The rebuilt EXE uses STRING::EMPTY at a linker-chosen
// address; all recovered owners call this helper instead of baking the retail VA.
inline uint32_t RetailEmptyStringAddress() {
    return static_cast<uint32_t>(reinterpret_cast<unsigned int>(STRING::EMPTY));
}
#pragma pack(push, 4)
struct STRING_LAYOUT32 { uint32_t ptr; };

struct LOGICSTACK_LAYOUT32 {
    uint8_t type;       // +0x00
    uint8_t pad01[3];
    int32_t number;     // +0x04
    uint32_t stringPtr; // +0x08

    LOGICSTACK_LAYOUT32();
    LOGICSTACK_LAYOUT32(const LOGICSTACK_LAYOUT32& other);
    LOGICSTACK_LAYOUT32& operator=(const LOGICSTACK_LAYOUT32& other);
    // Recovery-level value constructor for retail 0x00439550 family.
    explicit LOGICSTACK_LAYOUT32(int value);
    LOGICSTACK_LAYOUT32(const void* object, const STRING_LAYOUT32& context);
    // Source-family string-value constructor; target PushStr routes inline this shape.
    explicit LOGICSTACK_LAYOUT32(const STRING_LAYOUT32& value);
    void Read(void* stream);
    void BinarOperator(int operation, const LOGICSTACK_LAYOUT32& other);
    int Int();
    STRING_LAYOUT32* String();
    void Inc();
    void Dec();
};

struct LOGICVAR_LAYOUT32 {
    uint8_t flag;       // +0x00
    uint8_t auxFlags;   // +0x01, ZS1 function/return-state flags
    uint8_t pad02[2];
    STRING_LAYOUT32 value;   // +0x04
    int32_t a;          // +0x08
    int32_t type;       // +0x0C
    int32_t extra;      // +0x10
};

struct NAMED_LOGICVAR_ENTRY_LAYOUT32 {
    STRING_LAYOUT32 name;    // +0x00
    LOGICVAR_LAYOUT32 var;   // +0x04

    NAMED_LOGICVAR_ENTRY_LAYOUT32();
    NAMED_LOGICVAR_ENTRY_LAYOUT32& operator=(const NAMED_LOGICVAR_ENTRY_LAYOUT32& other);
};

struct NAMED_STRING_ENTRY_LAYOUT32 {
    STRING_LAYOUT32 name;    // +0x00
    STRING_LAYOUT32 value;   // +0x04
};

struct LIST_LOGICSTACK_LAYOUT32 {
    uint32_t vptr; // ZS1 0x0049246C
    int count;
    int capacity;
    uint32_t data;

    int GetNo() const;
    void Push(const LOGICSTACK_LAYOUT32& value);
    void Insert(LOGICSTACK_LAYOUT32 value);
    void Expand(int newCapacity);
    int IsLastString() const;
    int DeletePointerToObject(void* object);
};

struct NAMED_LIST_LOGICVAR_LAYOUT32 {
    uint32_t vptr; // derived 0x00492468; base destructor vptr 0x00492470
    int count;
    int capacity;
    uint32_t data;

    void Insert(STRING_LAYOUT32 name, LOGICVAR_LAYOUT32 value);
};

struct NAMED_LIST_STRING_LAYOUT32 {
    uint32_t vptr; // derived 0x00492464; base destructor vptr 0x00492474
    int count;
    int capacity;
    uint32_t data;

    void Insert(STRING_LAYOUT32 name, STRING_LAYOUT32 value);
    void Release();
};

// Exact ZS1 LOGIC x86 layout, proven by LOGIC::Release (0x00431620),
// parser/compiler xrefs, GetActionN/SetActionN and MAP::MAP construction.
// It occupies MAP+0x1A8..+0xA17: sizeof(LOGIC)==0x870 in this EXE.
struct LOGIC_LAYOUT32 {
    LIST_LOGICSTACK_LAYOUT32 stack;           // +0x000
    NAMED_LIST_LOGICVAR_LAYOUT32 variables;   // +0x010
    NAMED_LIST_STRING_LAYOUT32 strings;       // +0x020

    uint32_t currentSymbol;              // +0x030, LOGICVAR_LAYOUT32* for the function currently compiled
    STRING_LAYOUT32 name;                     // +0x034
    uint32_t ownedBuffer38;              // +0x038, freed by Release; exact owner name pending
    uint32_t byteCode;                   // +0x03C, owned compiled bytecode buffer
    int byteCodeSize;                         // +0x040, write cursor/final bytecode size
    uint32_t pos;                        // +0x044, source parser cursor
    uint32_t end;                        // +0x048, source end
    uint32_t sourceBuffer;               // +0x04C, owned source/parser allocation
    int line;                                 // +0x050
    int parserFlags54;                        // +0x054, preprocessor conditional/include state; saved/restored across #include
    int mainFunction;                         // +0x058, function/symbol index named "main"
    int parserMode;                           // +0x05C, directive-name parsing suppression mode used around #define/#undef names

    int actionN[256];                         // +0x060..+0x45F
    int scriptEventFunction[256];             // +0x460..+0x85F, ScriptEvent00..255 function indices

    uint32_t runtimeScratch860;           // +0x860
    uint32_t runtimeScratch864;           // +0x864
    int compileError;                         // +0x868, set by LOGIC::Error
    int parseContext;                         // +0x86C

    void Release();
    void PushStr(const STRING_LAYOUT32& value);
    void PushInt(int value);
    // The current body does not read the context, but the ret 8 ABI is real and call sites pass it.
    void PushObject(const void* object, const STRING_LAYOUT32& context);
    int LoadLGC(const STRING_LAYOUT32& name);
    int Load(const STRING_LAYOUT32& name);
    int LoadVar(void* stream);
    char** GetVariableStr(char** out, const STRING_LAYOUT32& name);
    int GetActionN(int actionNo);
    void SetActionN(int actionNo, int value);
    // Current ZS1 ABI has an explicit parameter-signature string as arg2.
    int CallFunction(int function, const char* signature,
                     const void* a, const void* b, int c);
};
#pragma pack(pop)


// Exact external-owner calls used by GetActionN error/int paths. They are kept
// explicit so this scaffold does not invent replacement runtime logging.

// Exact owner boundaries used by LOGIC::LoadLGC.  These are declarations of
// still-separate recovered subsystems, not replacement diagnostics.
inline void AssignRetailString(STRING_LAYOUT32* dst, const STRING_LAYOUT32* src) {
    STRING* d=reinterpret_cast<STRING*>(dst);
    const STRING* s=reinterpret_cast<const STRING*>(src);
    *d=*s;
} // STRING::operator=, retail 0x0044A9D0
int RetailFileLength(void* file); // MSVC6 filelength/fileno path
inline void ReportLogicError(LOGIC_LAYOUT32* logic, int type, const char* word, int line) {
    STRING text(word ? word : "");
    reinterpret_cast<LOGIC*>(logic)->Error(type,&text,line);
} // LOGIC::Error 0x00432130

// Runtime owner boundaries used by the target ZS1 bytecode interpreter.
STRING_LAYOUT32* RetailInt2Str(STRING_LAYOUT32* out, int value); // retail 0x004394C0 hidden return
void InvokeScriptOpcode(unsigned opcode); // implemented wrapper for retail 0x00444110
void RetailStreamRead(void* stream, void* dst, int bytes); // virtual STREAM::Read boundary used by 0x00439630
void RetailStringReadRes(STRING_LAYOUT32* value, void* stream); // STRING::Read_res 0x0044AD30
extern char g_RetailLogicErrorText[]; // GLOBAL: ZS1 0x004A6880

// Address anchors:
// 0x00431620 LOGIC::Release() -- body recovered in logic_runtime.cpp
// 0x00431780 LOGIC::LoadLGC(const STRING&) family
// 0x00437D90 LOGIC::GetActionN(int)
// 0x00437E00 LOGIC::SetActionN(int,int)
// 0x0041C2D0 LIST_LOGICSTACK::LIST_LOGICSTACK()
// 0x004144E0 NAMED_LIST_LOGICVAR::NAMED_LIST_LOGICVAR()
// 0x00414500 NAMED_LIST_STRING::NAMED_LIST_STRING()

} } // namespace zs1::logic
