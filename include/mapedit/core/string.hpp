#pragma once
// STRING owner. Included in ABI order by mapedit/runtime.hpp.
class STRING {
public:
    char* m_buf;
    static char EMPTY[16];
    STRING();
    STRING(const char* str);
    STRING(const char* str1,const char* str2);
    STRING(const char* str,int len);
    STRING(const STRING& r);
    STRING(const STRING* r);
    ~STRING();
    void Copy(const char* str, int len);
    char* CharPtr();
    int Length();
    int HaveChar(int chr);
    int WriteToBuf(void* buffer);
    char LastChar();
    void SetChar(int index,int c);
    int Replace(const STRING& source,const STRING& dest);
    int Replace(const STRING* source,const STRING* dest);
    int Replace(const char* source,const char* dest);
    STRING ToLower();
    STRING ToUpper();
    STRING ToBase64(int add_code);
    void RemoveBeginChars(const char* chars);
    void RemoveEndChars(const char* chars);
    void Read(FILE* file);
    void Read(STREAM* file);
    void Write(STREAM* file);
    void Write(FILE* file);
    int IsDigit();
    int Int();
    float Float();
    unsigned int FourCC();
    STRING BeforeChar(const char* chars) const;
    char operator[](int index);
    int LoadFile(const STRING& filename);
    int SaveFile(const STRING& filename) const;
    STRING Before(const char* marker) const;
    STRING After(const char* marker) const;
    STRING BeforeLast(const char* marker) const;
    const STRING AfterLast(const char* marker) const;
    char FirstChar() const;
    const STRING Add(int arg) const;
    int WriteToClipboard(HWND__* wnd);
    int ReadFromClipboard(HWND__* wnd);
    int HaveFirst(const char* prefix) const;
    int HaveSubStr(const char* str);
    void ToWideChar(unsigned short* buffer,int size_buffer);
    const STRING SplitRegPath(HKEY__** root) const;
    STRING operator+(const char* rhs) const;
    STRING operator+(const STRING& rhs) const;
    const STRING* operator+=(const char* rhs);
    const STRING* operator+=(const STRING* r);
    int operator==(const char* rhs) const;
    int operator!=(const char* rhs);
    int operator!=(const STRING* r);
    const STRING* operator=(const STRING& r);
    const STRING* operator=(const STRING* r);
    const STRING* operator=(const char* str);
    int operator==(const STRING* r);
    int operator<(const STRING* r);
    int operator>(const STRING* r);
};

// Retail mylib.h source shape.  These tiny STRING primitives are header-visible
// in the original program.  VC6 /Ob1 therefore folds them into small callers
// (for example LOGICSTACK constructors) while still emitting COMDAT owners when
// a larger caller declines to inline them.
inline STRING::STRING() : m_buf(EMPTY) {}

inline STRING::STRING(const char* text)
{
    if (text && text[0])
        Copy(text, static_cast<int>(strlen(text)));
    else
        m_buf = EMPTY;
}

inline STRING::STRING(const STRING& other)
{
    if (other.m_buf[0])
        Copy(other.m_buf, static_cast<int>(strlen(other.m_buf)));
    else
        m_buf = EMPTY;
}

inline STRING::STRING(const STRING* other)
{
    if (other->m_buf[0])
        Copy(other->m_buf, static_cast<int>(strlen(other->m_buf)));
    else
        m_buf = EMPTY;
}

inline STRING::~STRING()
{
    if (m_buf != EMPTY)
        ::operator delete(m_buf);
}

// Allocate the same 16-byte size class used by the original STRING storage.
inline void STRING::Copy(const char* text, int length)
{
    const unsigned int bytes = (static_cast<unsigned int>(length) & ~0x0Fu) + 0x10u;
    m_buf = static_cast<char*>(::operator new(bytes));
    memcpy(m_buf, text, static_cast<unsigned int>(length));
    m_buf[length] = 0;
}

inline char* STRING::CharPtr() { return m_buf; }

// this exact copy/_strlwr/return body into PICTURE_BASE::Load. Keeping the
// definition header-visible restores the original mylib source shape.
inline STRING STRING::ToLower()
{
    STRING result(m_buf);
    _strlwr(result.m_buf);
    return result;
}

inline int STRING::Length() { return static_cast<int>(strlen(m_buf)); }
inline int STRING::HaveChar(int chr) { return strchr(m_buf,chr)!=0; }
inline int STRING::WriteToBuf(void* buffer)
{
    const int size=Length()+1;
    memcpy(buffer,m_buf,static_cast<unsigned int>(size));
    return size;
}
inline char STRING::LastChar() { return m_buf[0] ? m_buf[strlen(m_buf)-1] : 0; }
inline void STRING::SetChar(int index,int c) { m_buf[index]=static_cast<char>(c); }
inline int STRING::HaveFirst(const char* prefix) const
{
    const unsigned int length = strlen(prefix);
    return strncmp(m_buf, prefix, length) == 0;
}
inline char STRING::FirstChar() const { return m_buf[0]; }
inline int STRING::operator==(const char* text) const { return strcmp(m_buf, text) == 0; }
inline int STRING::operator==(const STRING* r) { return strcmp(m_buf,r->m_buf)==0; }
inline int STRING::operator!=(const STRING* r) { return strcmp(m_buf,r->m_buf)!=0; }
inline int STRING::operator<(const STRING* r) { return strcmp(m_buf,r->m_buf)<0; }
inline int STRING::operator>(const STRING* r) { return strcmp(m_buf,r->m_buf)>0; }
// Retail mylib keeps the STRING-reference Replace wrapper header-visible.
// LOGICSTACK::BinarOperator operation 9 proves this owner boundary: VC6 expands
// the wrapper in the caller and calls only Replace(const char*,const char*).
inline int STRING::Replace(const STRING& source,const STRING& dest)
{
    return Replace(source.m_buf,dest.m_buf);
}

inline int STRING::Replace(const STRING* source,const STRING* dest)
{
    return Replace(source->m_buf,dest->m_buf);
}

const STRING __cdecl operator+(const char* lhs,const STRING& rhs);
STRING __cdecl Printf(const char* format,...);

