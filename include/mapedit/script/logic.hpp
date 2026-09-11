#pragma once
// Zombie Shooter 1 LOGIC owner.  PORT5 replaces the former A53 compatibility
// bridge with the target-proven x86 layout recovered from MapEditZS1.exe.
// Canonical offsets: LOGIC @ MAP+0x1A8, sizeof 0x870.

namespace zs1 { namespace logic { extern char g_RetailLogicErrorText[]; } }
extern MYERROR* Error;

struct LOGICSTACK {
    uint8_t type;                 // +0x00
    uint8_t pad01[3];
    int number;                   // +0x04
    STRING string;                // +0x08

    LOGICSTACK();
    LOGICSTACK(const LOGICSTACK& that);
    // ZS1 0x00439550: the emitted ctor has two explicit 32-bit arguments
    // (value + diagnostic/source context) and returns with ret 8.  The context
    // is intentionally unused by the retail body.
    LOGICSTACK(int value,const STRING* context);
    // Source-family string construction carries the same context at call sites;
    // large retail owners inline this shape.
    LOGICSTACK(const STRING* value,const STRING* context);
    LOGICSTACK(const void* object,const STRING* context=0);
    ~LOGICSTACK();
    LOGICSTACK& operator=(const LOGICSTACK& that);

    int IsArray() const;
    int IsInt() const;
    int IsString() const;
    int IsObject() const;
    int IsPointer() const;
    int IsInit() const;
    int Int();
    const STRING* String();
    void Inc();
    void Dec();
    void BinarOperator(int operation,const LOGICSTACK& r);
    void Read(STREAM* file);
};
// Retail script.h inline value constructors.  Their context is ABI-significant
// even where the body does not read it: the 0x00439550 body returns with ret 8.
inline LOGICSTACK::LOGICSTACK(const LOGICSTACK& r)
    : type(r.type),number(r.number),string(r.string) {}
inline LOGICSTACK::LOGICSTACK(int value,const STRING*)
    : type(2),number(value),string() {}
inline LOGICSTACK::LOGICSTACK(const STRING* value,const STRING*)
    : type(1),string(*value) {}
inline LOGICSTACK::LOGICSTACK(const void* object,const STRING*)
    : type(object?0x12:0x02),
      number(static_cast<int>(reinterpret_cast<unsigned int>(object))),string() {}

// Retail script.h/mylib.h inline primitives.  Direct target CallFunction ASM
// calls Int/BinarOperator, but not these tiny accessors/mutators nor String();
// small callers therefore fold them while larger owners may still emit COMDATs.
inline LOGICSTACK& LOGICSTACK::operator=(const LOGICSTACK& r)
{
    if(this!=&r) {
        type=static_cast<uint8_t>(r.type&0x7Fu);
        number=r.number;
        string=r.string;
    }
    return *this;
}
inline int LOGICSTACK::IsArray() const { return type&4; }
inline int LOGICSTACK::IsInt() const { return type&2; }
inline int LOGICSTACK::IsString() const { return type&1; }
inline int LOGICSTACK::IsObject() const { return type&0x10; }
inline int LOGICSTACK::IsPointer() const { return type&0x20; }
inline int LOGICSTACK::IsInit() const { return type&0x08; }
inline const STRING* LOGICSTACK::String()
{
    if(type&2) {
        char buffer[128];
        string=STRING(_itoa(number,buffer,10));
    }
    return &string;
}
inline void LOGICSTACK::Inc()
{
    type=static_cast<uint8_t>(type&~0x10u);
    if(type&0x22) ++number;
}
inline void LOGICSTACK::Dec()
{
    type=static_cast<uint8_t>(type&~0x10u);
    if(type&0x22) --number;
}

// Retail keeps LIST<LOGICSTACK>::Insert as an out-of-line owner at 0x0043A040.
// Declare the explicit specialization before inline Push/ExpandForInsert use so
// old/strict compilers do not instantiate the primary template first.
template<> void LIST<LOGICSTACK>::Insert(LOGICSTACK item);

// ZS1 LIST<LOGICSTACK> trivial/template bodies were visible at the call site.
// Evidence: target 0x43A040 LIST::Insert contains ExpandForInsert in-line,
// target CallFunction uses the standalone Push owner 0x439B30, while the much
// smaller LOGIC/MAP Push wrappers inline Push and call Insert directly.
template<> inline int LIST<LOGICSTACK>::No() { return m_no; }
template<> inline LOGICSTACK* LIST<LOGICSTACK>::First() { return m_data; }
template<> inline LOGICSTACK* LIST<LOGICSTACK>::operator[](int index) { return m_data+index; }
template<> inline const LOGICSTACK* LIST<LOGICSTACK>::operator[](int index) const { return m_data+index; }
template<> inline LOGICSTACK* LIST<LOGICSTACK>::Last() { return m_data+m_no-1; }
template<> inline LOGICSTACK* LIST<LOGICSTACK>::Pop() { --m_no; return m_data+m_no; }
// Target 0x0043A160 is an emitted COMDAT, while target Insert 0x0043A040
// inlines this complete body.  This requires the definition to be visible here.
template<> inline void LIST<LOGICSTACK>::Expand(int newAllocation)
{
    if(newAllocation<=m_max)
        return;
    LOGICSTACK* oldData=m_data;
    LOGICSTACK* fresh=new LOGICSTACK[newAllocation];
    m_data=fresh;
    if(!fresh)
        MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",newAllocation);
    if(oldData) {
        for(int i=0;i<m_max;++i)
            fresh[i]=oldData[i];
        delete[] oldData;
    }
    m_max=newAllocation;
}
template<> inline void LIST<LOGICSTACK>::ExpandForInsert()
{
    if(m_no>=m_max) Expand(m_max*2+4);
}
template<> inline void LIST<LOGICSTACK>::Push(const LOGICSTACK* item)
{
    Insert(*item);
}
template<> inline void LIST<LOGICSTACK>::SetNo(int newNo)
{
    m_no=newNo;
    if(m_no>m_max) Expand(m_no);
}

struct LOGICVAR {
    uint8_t flag;                 // +0x00
    uint8_t auxFlags;             // +0x01
    uint8_t pad02[2];
    STRING value;                 // +0x04
    int a;                        // +0x08 (function bytecode address / value metadata)
    int type;                     // +0x0C (function parameter base for flag==3)
    int extra;                    // +0x10 (parameter count / array metadata)

    LOGICVAR();
    LOGICVAR(int kind,int v);
    ~LOGICVAR();
    LOGICVAR& operator=(const LOGICVAR& r);
};

// Original mylib.h source shape: LOGIC compiler/parser calls Location() directly
// on the named lists.  Target owners fold this backwards name search at /Ob1;
// no standalone LOGICVAR/STRING Location owner is present in the retail map.
template<> inline int NAMED_LIST<LOGICVAR>::Location(const STRING* name)
{
    for(int i=this->m_no;i>0;--i)
        if(this->m_data[i-1].name==name)
            return i-1;
    return -1;
}
template<> inline int NAMED_LIST<STRING>::Location(const STRING* name)
{
    for(int i=this->m_no;i>0;--i)
        if(this->m_data[i-1].name==name)
            return i-1;
    return -1;
}

class LOGIC {
public:
    LIST<LOGICSTACK> stack;       // +0x000
    NAMED_LIST<LOGICVAR> variables;// +0x010
    NAMED_LIST<STRING> strings;   // +0x020
    LOGICVAR* currentSymbol;      // +0x030
    STRING name;                  // +0x034
    void* ownedBuffer38;          // +0x038
    unsigned char* byteCode;      // +0x03C
    int byteCodeSize;             // +0x040
    char* pos;                    // +0x044
    char* end;                    // +0x048
    char* sourceBuffer;           // +0x04C
    int line;                     // +0x050
    int parserFlags54;            // +0x054
    int mainFunction;             // +0x058
    int parserMode;               // +0x05C
    int actionN[256];             // +0x060
    int scriptEventFunction[256]; // +0x460
    void* runtimeScratch860;      // +0x860
    void* runtimeScratch864;      // +0x864
    int compileError;             // +0x868
    int parseContext;             // +0x86C

    LOGIC();
    ~LOGIC();
    int Load(const STRING* scriptName);
    int LoadLGC(const STRING* scriptName);
    int Save();
    void SaveVar(STREAM* file);
    void LoadVar(STREAM* file);
    void Error(int type,const STRING* text,int err);
    // ZS1 parser/compiler owners (0x00431CD0..0x00437B10).  These are real
    // LOGIC members in retail; keeping them on LOGIC removes the former
    // reconstruction-only LOGIC_LAYOUT32 owner names while preserving the
    // exact 0x870 object layout above.
    void EmitByteIfNoError(uint8_t opcode);
    void EmitByteInt32(uint8_t opcode,int value);
    int skipempty2();
    int skipempty();
    int GetLine(STRING* out);
    int GetName(STRING* out);
    int Word(const char* word);
    int WordEnd(const char* word);
    int GetInt();
    STRING GetConstantString();
    int GetString(char* out);
    int SetNoElement(int value);
    void IntVar(int declarationMode);
    void StringVar(int declarationMode);
    void mnog();
    void SetOperation(int byteCodePos,int operation);
    void slag();
    void cmpslag();
    void logicslag();
    void vyragAnd();
    void vyragXor();
    void vyragOr();
    void vyragCmpAnd();
    void CompileExpression(int context);
    int vyrag_oper();
    void oper(int* breakFixups);
    int func();
    int CallFunction(int function,const char* signature,const void* a,const void* b,int c);
    // Source-family compatibility overload retained only for callers not yet carrying
    // an explicit target signature; PORT5 canonical MAP routes use ppi/iii.
    int CallFunction(int function,const void* a,const void* b,int c);
    int DeletePointerToObject(void* object);
    STRING GetVariableStr(const STRING* varName);
    const NAMED_LIST<LOGICVAR>* Var();
    void PushInt(int value);
    void PushStr(const STRING* value);
    int PopInt();
    const STRING* PopStr();
    int PopObject();
    void PushObject(const void* object,const STRING* context=0);
    int IsLastStackString();
    int GetActionN(int actionNo);
    void SetActionN(int actionNo,int value);
    void Release();
};


// Retail script.h source shape: these tiny owners are inline.  VC6 /Ob1 emits
// standalone COMDAT copies when needed and also folds the bodies into large
// callers such as LOGIC::CallFunction and the MAP script-stack wrappers.
inline void LOGIC::PushInt(int value)
{
    STRING context(zs1::logic::g_RetailLogicErrorText);
    LOGICSTACK item(value,&context);
    stack.Push(&item);
}

inline void LOGIC::PushStr(const STRING* value)
{
    STRING context(zs1::logic::g_RetailLogicErrorText);
    LOGICSTACK item(value,&context);
    stack.Push(&item);
}

inline void LOGIC::PushObject(const void* object,const STRING* context)
{
    LOGICSTACK item(object,context);
    stack.Push(&item);
}

